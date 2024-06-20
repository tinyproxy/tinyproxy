/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1998 Steven Young <sdyoung@miranda.org>
 * Copyright (C) 1999 Robert James Kaes <rjkaes@users.sourceforge.net>
 * Copyright (C) 2009 Michael Adam <obnox@samba.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* Logs the various messages which tinyproxy produces to either a log file
 * or the syslog daemon. Not much to it...
 */

#include "main.h"

#include "heap.h"
#include "log.h"
#include "utils.h"
#include "sblist.h"
#include "conf.h"
#include <pthread.h>

static const char *syslog_level[] = {
        NULL,
        NULL,
        "CRITICAL",
        "ERROR",
        "WARNING",
        "NOTICE",
        "INFO",
        "DEBUG",
        "CONNECT"
};

#define TIME_LENGTH 16
#define STRING_LENGTH 800

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

/*
 * Global file descriptor for the log file
 */
int log_file_fd = -1;

/*
 * Store the log level setting.
 */
static int log_level = LOG_INFO;

/*
 * Hold a listing of log messages which need to be sent once the log
 * file has been established.
 * The key is the actual messages (already filled in full), and the value
 * is the log level.
 */
static sblist *log_message_storage;

static unsigned int logging_initialized = FALSE;     /* boolean */

/*
 * Open the log file and store the file descriptor in a global location.
 */
int open_log_file (const char *log_file_name)
{
        if (log_file_name == NULL) {
                log_file_fd = fileno(stdout);
        } else {
                log_file_fd = create_file_safely (log_file_name, FALSE);
        }
        return log_file_fd;
}

/*
 * Close the log file
 */
void close_log_file (void)
{
        if (log_file_fd < 0 || log_file_fd == fileno(stdout)) {
                return;
        }

        close (log_file_fd);
        log_file_fd = -1;
}

/*
 * Set the log level for writing to the log file.
 */
void set_log_level (int level)
{
        log_level = level;
}

/*
 * This routine logs messages to either the log file or the syslog function.
 */
void log_message (int level, const char *fmt, ...)
{
        va_list args;
        struct timespec nowtime;
        struct tm tm_buf;

        char time_string[TIME_LENGTH];
        char str[STRING_LENGTH];

        ssize_t ret;

#ifdef NDEBUG
        /*
         * Figure out if we should write the message or not.
         */
        if (log_level == LOG_CONN) {
                if (level == LOG_INFO)
                        return;
        } else if (log_level == LOG_INFO) {
                if (level > LOG_INFO && level != LOG_CONN)
                        return;
        } else if (level > log_level)
                return;
#endif

        if (config && config->syslog && level == LOG_CONN)
                level = LOG_INFO;

        va_start (args, fmt);

        /*
         * If the config file hasn't been processed, then we need to store
         * the messages for later processing.
         */
        if (!logging_initialized) {
                char *entry_buffer;

                if (!log_message_storage) {
                        log_message_storage = sblist_new (sizeof(char*), 64);
                        if (!log_message_storage)
                                goto out;
                }

                vsnprintf (str, STRING_LENGTH, fmt, args);

                entry_buffer = (char *) safemalloc (strlen (str) + 6);
                if (!entry_buffer)
                        goto out;

                sprintf (entry_buffer, "%d %s", level, str);
                if(!sblist_add (log_message_storage, &entry_buffer))
                        safefree (entry_buffer);
                goto out;
        }

        if(!config->syslog && log_file_fd == -1)
                goto out;

        if (config->syslog) {
                pthread_mutex_lock(&log_mutex);
#ifdef HAVE_VSYSLOG_H
                vsyslog (level, fmt, args);
#else
                vsnprintf (str, STRING_LENGTH, fmt, args);
                syslog (level, "%s", str);
#endif
                pthread_mutex_unlock(&log_mutex);
        } else {
                char *p;

                clock_gettime(CLOCK_REALTIME, &nowtime);
                /* Format is month day hour:minute:second (24 time) */
                strftime (time_string, TIME_LENGTH, "%b %d %H:%M:%S",
                          localtime_r (&nowtime.tv_sec, &tm_buf));

                snprintf (str, STRING_LENGTH, "%-9s %s.%03lu [%ld]: ",
                          syslog_level[level], time_string,
                          (unsigned long) nowtime.tv_nsec/1000000ul,
                          (long int) getpid ());

                /*
                 * Overwrite the '\0' and leave room for a trailing '\n'
                 * be added next.
                 */
                p = str + strlen(str);
                vsnprintf (p, STRING_LENGTH - strlen(str) - 1, fmt, args);

                p = str + strlen(str);
                *p = '\n';
                *(p+1) = '\0';

                assert (log_file_fd >= 0);

                pthread_mutex_lock(&log_mutex);
                ret = write (log_file_fd, str, strlen (str));
                pthread_mutex_unlock(&log_mutex);

                if (ret == -1) {
                        config->syslog = TRUE;

                        log_message(LOG_CRIT, "ERROR: Could not write to log "
                                    "file %s: %s.",
                                    config->logf_name, strerror(errno));
                        log_message(LOG_CRIT,
                                    "Falling back to syslog logging");
                }

                pthread_mutex_lock(&log_mutex);
                fsync (log_file_fd);
                pthread_mutex_unlock(&log_mutex);

        }

out:
        va_end (args);
}

/*
 * This needs to send any stored log messages.
 */
static void send_stored_logs (void)
{
        char **string;
        char *ptr;
        int level;
        size_t i;

        if (log_message_storage == NULL)
                return;

        log_message(LOG_DEBUG, "sending stored logs");

        for (i = 0; i < sblist_getsize (log_message_storage); ++i) {
                string = sblist_get (log_message_storage, i);
                if (!string || !*string) continue;

                ptr = strchr (*string, ' ') + 1;
                level = atoi (*string);

#ifdef NDEBUG
                if (log_level == LOG_CONN && level == LOG_INFO)
                        continue;
                else if (log_level == LOG_INFO) {
                        if (level > LOG_INFO && level != LOG_CONN)
                                continue;
                } else if (level > log_level)
                        continue;
#endif

                log_message (level, "%s", ptr);
                safefree(*string);
        }

        sblist_free (log_message_storage);
        log_message_storage = NULL;

        log_message(LOG_DEBUG, "done sending stored logs");
}

/**
 * Initialize the logging subsystem, based on the configuration.
 * Returns 0 upon success, -1 upon failure.
 */
int setup_logging (void)
{
        if (!config->syslog) {
                if (open_log_file (config->logf_name) < 0) {
                        /*
                         * If opening the log file fails, we try
                         * to fall back to syslog logging...
                         */
                        config->syslog = TRUE;

                        log_message (LOG_CRIT, "ERROR: Could not create log "
                                     "file %s: %s.",
                                     config->logf_name, strerror (errno));
                        log_message (LOG_CRIT,
                                     "Falling back to syslog logging.");
                }
        }

        if (config->syslog) {
                openlog ("tinyproxy", LOG_PID, LOG_USER);
        }

        logging_initialized = TRUE;
        send_stored_logs ();

        return 0;
}

/**
 * Stop the logging subsystem.
 */
void shutdown_logging (void)
{
        if (!logging_initialized) {
                return;
        }

        if (config->syslog) {
                closelog ();
        } else {
                close_log_file ();
        }

        logging_initialized = FALSE;
}
