/*
 * Define if you want to have the peer's IP address to be included in a
 * XTinyproxy header sent to the server.
 */
#undef XTINYPROXY

/*
 * These are the defaults, but they can be changed by configure at compile
 * time, or on the command line at runtime.
 */
#define DEFAULT_LOG "/var/log/tinyproxy"
#define DEFAULT_PORT 8888
#define DEFAULT_USER ""

/*
 * Define if you would like to include filtering code.
 */
#undef FILTER_ENABLE

/*
 * Define if you want to use the included GNU regex routine
 */
#undef USE_GNU_REGEX

/*
 * Define if you want to include upstream proxy support
 */
#undef UPSTREAM_PROXY
