/* automatically generated with re2r by rofl0r */
%%{
machine logfile;
action A1 { matches[1].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
main := '"'([^"]+) >A1 %E1 '"' ;
}%%

RE2R_EXPORT int re2r_match_logfile(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

RE2R_EXPORT int re2r_match_pidfile(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_anonymous(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_viaproxyname(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_defaulterrorfile(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_statfile(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_stathost(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

%%{
machine xtinyproxy;
action A1 { matches[1].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
main := ([Yy][Ee][Ss]|[Oo][Nn]|[Nn][Oo]|[Oo][Ff][Ff]) >A1 %E1  ;
}%%

RE2R_EXPORT int re2r_match_xtinyproxy(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

RE2R_EXPORT int re2r_match_syslog(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_bindsame(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_disableviaheader(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

%%{
machine port;
action A1 { matches[1].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
main := ([0-9]+) >A1 %E1  ;
}%%

RE2R_EXPORT int re2r_match_port(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

RE2R_EXPORT int re2r_match_maxclients(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_maxspareservers(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_minspareservers(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_startservers(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_maxrequestsperchild(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_timeout(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_connectport(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_port(p, pe, nmatch, matches);
}

%%{
machine user;
action A1 { matches[1].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
main := (('-'|[A-Za-z0-9._])+) >A1 %E1  ;
}%%

RE2R_EXPORT int re2r_match_user(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

RE2R_EXPORT int re2r_match_group(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_user(p, pe, nmatch, matches);
}

%%{
machine listen;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action A3 { matches[3].rm_so = p-start; }
action A4 { matches[4].rm_so = p-start; }
action A5 { matches[5].rm_so = p-start; }
action A6 { matches[6].rm_so = p-start; }
action A7 { matches[7].rm_so = p-start; }
action A8 { matches[8].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
action E3 { matches[3].rm_eo = p-start; }
action E4 { matches[4].rm_eo = p-start; }
action E5 { matches[5].rm_eo = p-start; }
action E6 { matches[6].rm_eo = p-start; }
action E7 { matches[7].rm_eo = p-start; }
action E8 { matches[8].rm_eo = p-start; }
main := (([0-9]+[.][0-9]+[.][0-9]+[.][0-9]+) >A2 %E2 |((([0-9a-fA-F:]{2,39}) >A5 %E5 ) >A4 %E4 |(([0-9a-fA-F:]{0,29} ":" ([0-9]+[.][0-9]+[.][0-9]+[.][0-9]+) >A8 %E8 ) >A7 %E7 ) >A6 %E6 ) >A3 %E3 ) >A1 %E1  ;
}%%

RE2R_EXPORT int re2r_match_listen(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=1,[3]=1,[4]=3,[5]=4,[6]=3,[7]=6,[8]=7,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

%%{
machine allow;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action A3 { matches[3].rm_so = p-start; }
action A4 { matches[4].rm_so = p-start; }
action A5 { matches[5].rm_so = p-start; }
action A6 { matches[6].rm_so = p-start; }
action A7 { matches[7].rm_so = p-start; }
action A8 { matches[8].rm_so = p-start; }
action A9 { matches[9].rm_so = p-start; }
action A10 { matches[10].rm_so = p-start; }
action A11 { matches[11].rm_so = p-start; }
action A12 { matches[12].rm_so = p-start; }
action A13 { matches[13].rm_so = p-start; }
action A14 { matches[14].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
action E3 { matches[3].rm_eo = p-start; }
action E4 { matches[4].rm_eo = p-start; }
action E5 { matches[5].rm_eo = p-start; }
action E6 { matches[6].rm_eo = p-start; }
action E7 { matches[7].rm_eo = p-start; }
action E8 { matches[8].rm_eo = p-start; }
action E9 { matches[9].rm_eo = p-start; }
action E10 { matches[10].rm_eo = p-start; }
action E11 { matches[11].rm_eo = p-start; }
action E12 { matches[12].rm_eo = p-start; }
action E13 { matches[13].rm_eo = p-start; }
action E14 { matches[14].rm_eo = p-start; }
main := (((([0-9]+[.][0-9]+[.][0-9]+[.][0-9]+) >A4 %E4 ( "/" [0-9]+)? >A5 %E5 ) >A3 %E3 |(((([0-9a-fA-F:]{2,39}) >A9 %E9 ) >A8 %E8 |(([0-9a-fA-F:]{0,29} ":" ([0-9]+[.][0-9]+[.][0-9]+[.][0-9]+) >A12 %E12 ) >A11 %E11 ) >A10 %E10 ) >A7 %E7 ( "/" [0-9]+)? >A13 %E13 ) >A6 %E6 ) >A2 %E2 |(('-'|[A-Za-z0-9._])+) >A14 %E14 ) >A1 %E1  ;
}%%

RE2R_EXPORT int re2r_match_allow(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=1,[3]=2,[4]=3,[5]=3,[6]=2,[7]=6,[8]=7,[9]=8,[10]=7,[11]=10,[12]=11,[13]=6,[14]=1,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

RE2R_EXPORT int re2r_match_deny(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_allow(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_bind(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_listen(p, pe, nmatch, matches);
}

%%{
machine basicauth;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
main := (('-'|[A-Za-z0-9._])+) >A1 %E1 [ \t]+(('-'|[A-Za-z0-9._])+) >A2 %E2  ;
}%%

RE2R_EXPORT int re2r_match_basicauth(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

%%{
machine errorfile;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
main := ([0-9]+) >A1 %E1 [ \t]+'"'([^"]+) >A2 %E2 '"' ;
}%%

RE2R_EXPORT int re2r_match_errorfile(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

%%{
machine addheader;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
main := '"'([^"]+) >A1 %E1 '"'[ \t]+'"'([^"]+) >A2 %E2 '"' ;
}%%

RE2R_EXPORT int re2r_match_addheader(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

RE2R_EXPORT int re2r_match_filter(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_filterurls(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_filterextended(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_filterdefaultdeny(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_filtercasesensitive(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_reversebaseurl(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_logfile(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_reverseonly(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

RE2R_EXPORT int re2r_match_reversemagic(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	return re2r_match_xtinyproxy(p, pe, nmatch, matches);
}

%%{
machine reversepath;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action A3 { matches[3].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
action E3 { matches[3].rm_eo = p-start; }
main := '"'([^"]+) >A1 %E1 '"'([ \t]+'"'([^"]+) >A3 %E3 '"')? >A2 %E2  ;
}%%

RE2R_EXPORT int re2r_match_reversepath(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=0,[3]=2,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

%%{
machine upstream;
action A1 { matches[1].rm_so = p-start; }
action A2 { matches[2].rm_so = p-start; }
action A3 { matches[3].rm_so = p-start; }
action A4 { matches[4].rm_so = p-start; }
action A5 { matches[5].rm_so = p-start; }
action A6 { matches[6].rm_so = p-start; }
action A7 { matches[7].rm_so = p-start; }
action A8 { matches[8].rm_so = p-start; }
action A9 { matches[9].rm_so = p-start; }
action A10 { matches[10].rm_so = p-start; }
action A11 { matches[11].rm_so = p-start; }
action A12 { matches[12].rm_so = p-start; }
action A13 { matches[13].rm_so = p-start; }
action A14 { matches[14].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
action E2 { matches[2].rm_eo = p-start; }
action E3 { matches[3].rm_eo = p-start; }
action E4 { matches[4].rm_eo = p-start; }
action E5 { matches[5].rm_eo = p-start; }
action E6 { matches[6].rm_eo = p-start; }
action E7 { matches[7].rm_eo = p-start; }
action E8 { matches[8].rm_eo = p-start; }
action E9 { matches[9].rm_eo = p-start; }
action E10 { matches[10].rm_eo = p-start; }
action E11 { matches[11].rm_eo = p-start; }
action E12 { matches[12].rm_eo = p-start; }
action E13 { matches[13].rm_eo = p-start; }
action E14 { matches[14].rm_eo = p-start; }
main := (( "none" ) >A2 %E2 [ \t]+'"'([^"]+) >A3 %E3 '"') >A1 %E1 |(( "http" | "socks4" | "socks5" ) >A5 %E5 [ \t]+(([^:]*) >A7 %E7  ":" ([^@]*) >A8 %E8  "@" )? >A6 %E6 (([0-9]+[.][0-9]+[.][0-9]+[.][0-9]+) >A10 %E10 |(('-'|[A-Za-z0-9._])+) >A11 %E11 ) >A9 %E9  ":" ([0-9]+) >A12 %E12 ([ \t]+'"'([^"]+) >A14 %E14 '"')? >A13 %E13 ) >A4 %E4  ;
}%%

RE2R_EXPORT int re2r_match_upstream(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,[2]=1,[3]=1,[4]=0,[5]=4,[6]=4,[7]=6,[8]=6,[9]=4,[10]=9,[11]=9,[12]=4,[13]=4,[14]=13,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

%%{
machine loglevel;
action A1 { matches[1].rm_so = p-start; }
action E1 { matches[1].rm_eo = p-start; }
main := ([Cc] "ritical" |[Ee] "rror" |[Ww] "arning" |[Nn] "otice" |[Cc] "onnect" |[Ii] "nfo" ) >A1 %E1  ;
}%%

RE2R_EXPORT int re2r_match_loglevel(const char *p, const char* pe, size_t nmatch, regmatch_t matches[])
{
	size_t i, cs;
	int par;
	static const unsigned char parents[] = {[0]=0,[1]=0,};
	const char *start = p, *eof = pe;
	%% write data nofinal noerror noentry;
	for(i=0;i<nmatch;++i) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	%% write init;
	%% write exec;
	if(cs < %%{ write first_final; }%% ) return -1;
	matches[0] = (regmatch_t){.rm_so = 0, .rm_eo = eof-start};
	for(i=1;i<nmatch;++i) if(matches[i].rm_eo == -1) matches[i].rm_so = -1;
	else if(matches[i].rm_so == matches[i].rm_eo) matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1};
	else { par = i; while((par = parents[par])) if(matches[par].rm_eo == -1) { matches[i] = (regmatch_t){.rm_so = -1, .rm_eo = -1}; break; }}
	return 0;
}

