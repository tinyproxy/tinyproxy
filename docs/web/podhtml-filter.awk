BEGIN {i=0}
/<\/{0,1}h1/ {if(!i)i=1; gsub("h1", "h4", $0);}
#/<\/body>/ {i=0;}
/BUGS/ {i=-1}
{if(i==1) print;}
