typedef union {
	unsigned int num;
	char *cptr;
	void *ptr;
} YYSTYPE;
#define	KW_PORT	257
#define	KW_LISTEN	258
#define	KW_LOGFILE	259
#define	KW_PIDFILE	260
#define	KW_SYSLOG	261
#define	KW_MAXCLIENTS	262
#define	KW_MAXSPARESERVERS	263
#define	KW_MINSPARESERVERS	264
#define	KW_STARTSERVERS	265
#define	KW_MAXREQUESTSPERCHILD	266
#define	KW_TIMEOUT	267
#define	KW_USER	268
#define	KW_GROUP	269
#define	KW_ANONYMOUS	270
#define	KW_FILTER	271
#define	KW_XTINYPROXY	272
#define	KW_TUNNEL	273
#define	KW_ALLOW	274
#define	KW_DENY	275
#define	KW_YES	276
#define	KW_NO	277
#define	IDENTIFIER	278
#define	NUMBER	279
#define	STRING	280
#define	NUMERIC_ADDRESS	281
#define	STRING_ADDRESS	282
#define	NETMASK_ADDRESS	283


extern YYSTYPE yylval;
