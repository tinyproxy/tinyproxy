# test webpage with `python -m SimpleHTTPServer`

all: index.html

tp.html.conf: ../man5/tinyproxy.conf.txt
	pod2html --noindex < $^ | awk -f podhtml-filter.awk > $@

index.html: tp.html.head tp.html.conf tp.html.foot
	cat $^ > $@

clean:
	rm tp.html.conf index.html *.tmp

.PHONY: all clean

