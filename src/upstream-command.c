/* TODO(tkluck): this is needed for fdopen. Does it have portability
 * downsides? */
#define _POSIX_C_SOURCE 1
#include <stdio.h>

#include "upstream-command.h"
#include "upstream.h"

#define MAX_RESULT_MESSAGE_SIZE 1024

/*
 * ensure that st->in is writable (according to select() ). Use select()
 * to wait at most UPSTREAM_COMMAND_WRITABLE_TIMOUT_MS milliseconds,
 * and respawn otherwise.
 * Returns a negative error code if we do not succeed in respawning.
 */
static int ensure_upstream_command_writable(const char *cmd, struct upstream_command_state *st)
{
        int err;
        int parent2child[2];
        int child2parent[2];

        assert(st != NULL);

        if (0 == st->pid) {
                err = pipe(parent2child);
                if (0 != err)
                        return -1;
                err = pipe(child2parent);
                if (0 != err)
                        return -1;

                st->pid = fork();
                if (0 > std->pid)
                        return -1;
                if (0 == st->pid) {
                        /* I'm the child; connect stdin/stdout to the pipes */
                        if (0 > close(parent2child[1]))
                                exit(1);
                        if (0 > close(child2parent[0]))
                                exit(1);
                        if (0 > dup2(parent2child[0], 0))
                                exit(1);
                        if (0 > dup2(child2parent[1], 1))
                                exit(1);

                        /* TODO(tkluck): do we need to worry about installed
                         * signal handlers (e.g. for SIGCHLD)?
                         */

                        err = execl("/bin/sh", "sh", "-c", cmd, (char *) 0);

                        /* unreachable unless something is wrong with the command */
                        exit(1);
                } else {
                        /* I'm the parent */
                        sleep(1);
                        close(parent2child[0]);
                        close(child2parent[1]);
                        st->in  = fdopen(parent2child[1], "w");
                        st->out = fdopen(child2parent[0], "r");
                }
        }

        /* TODO(tkluck): use select(2) to ensure that writing to
         * the process stdin doesn't block (for sufficiently small
         * message). Otherwise, fork a new copy and kill the old one. */

        return 0;
}

/*
 * Communicate with the configured command to obtain a proxy
 * specification, and _replace_ the fields in *up with the appropriate
 * spec.
 *
 * In case of success, returns one of the following positive constants:
 *     UPSTREAM_DIRECT - no proxy
 *     UPSTREAM_FALLTHROUGH - use subsequent rules
 *     UPSTREAM_PROXY - use upstream as configured in *up
 *
 * Returns a negative error code if we do not succeed in communicating
 * with the subprocess.
 */
int upstream_get_from_command(const char *host, struct upstream *up)
{
        int err;
        char msg[MAX_RESULT_MESSAGE_SIZE];
        char *msgres;

        assert(up != NULL);
        assert(up->command != NULL);
        if (NULL != strchr(host, '\n'))
                return -1;

        err = ensure_upstream_command_writable(up->command, &up->cmdstate);
        if (err < 0)
                return -2;

        /* TODO(tkluck): replace by version with timeout. */
        err = fprintf(up->cmdstate.in, "HOST %s\n", host);
        if (err < 0)
                return -3;
        err = fflush(up->cmdstate.in);
        if (err != 0)
                return -4;

        /* TODO(tkluck): replace by version with timeout */
        msgres = fgets(msg, MAX_RESULT_MESSAGE_SIZE, up->cmdstate.out);
        if (NULL == msgres)
                return -5;

        if (strncmp(msg, "DIRECT",       6)) {
                return UPSTREAM_DIRECT;
        } else if
           (strncmp(msg, "FALLTHROUGH", 11)) {
                return UPSTREAM_FALLTHROUGH;
        } else if
           (strncmp(msg, "HTTP ",        5)) {
                /* TODO(tkluck): parse and fill the struct here.
                 * use same/similar format as the configuration file */
                return UPSTREAM_PROXY;
        } else if
           (strncmp(msg, "SOCKS4 ",      7)) {
                /* TODO(tkluck): parse and fill the struct here.
                 * use same/similar format as the configuration file */
                return UPSTREAM_PROXY;
        } else if
           (strncmp(msg, "SOCKS5 ",      7)) {
                /* TODO(tkluck): parse and fill the struct here.
                 * use same/similar format as the configuration file */
                return UPSTREAM_PROXY;
        } else {
                goto parsefail;
        }

parsefail:
        return -6;

}
