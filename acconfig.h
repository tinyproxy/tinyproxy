/*
 * Define if you want to have the peer's IP address to be included in a
 * XTinyproxy header sent to the server.
 */
#undef XTINYPROXY_ENABLE

/*
 * This is the default location of the configuration file
 */
#define DEFAULT_CONF_FILE "/etc/tinyproxy/tinyproxy.conf"

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
#undef TUNNEL_SUPPORT

/*
 * NOTE: for DEFAULT_STATHOST:  this controls remote proxy stats display.
 * for example, the default DEFAULT_STATHOST of "tinyproxy.stats" will
 * mean that when you use the proxy to access http://tinyproxy.stats/",
 * you will be shown the proxy stats.  Set this to something obscure
 * if you don't want random people to be able to see them, or set it to
 * "" to disable.  In the future, I figure maybe some sort of auth
 * might be desirable, but that would involve a major simplicity
 * sacrifice.
 *
 *
 * The "hostname" for getting tinyproxy stats. "" = disabled by default
 */
#define DEFAULT_STATHOST "tinyproxy.stats"

/*
 * Define the following for the appropriate datatype, if necessary
 */
#undef uint8_t
#undef int16_t
#undef uint16_t
#undef int32_t
#undef uint32_t
#undef in_addr_t 
#undef size_t
#undef ssize_t
#undef socklen_t
