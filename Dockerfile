# tinyproxy - A fast light-weight HTTP proxy
# Copyright (C) 2019 Kaito Yamada <kaitoy@pcap4j.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

FROM alpine:3.10 as build

COPY / /src
WORKDIR /src

RUN apk update \
    && \
    apk add --no-cache \
        make \
        automake \
        gcc \
        musl-dev \
        asciidoc \
        autoconf \
    && \
    ./autogen.sh \
    && \
    ./configure \
        --prefix=/tmp \
        --enable-xtinyproxy \
        --enable-filter \
        --enable-upstream \
        --enable-transparent \
        --enable-reverse \
    && \
    make \
    && \
    make install \
    && \
    rm -rf /tmp/share/doc /tmp/share/man

FROM alpine:3.10

COPY --from=build /tmp/share /tinyproxy/share
COPY --from=build /tmp/etc /tinyproxy/etc
COPY --from=build /tmp/bin /tinyproxy/bin

RUN apk update \
    && \
    apk add --no-cache \
        tini \
        ca-certificates

ENTRYPOINT ["tini", "--", "/tinyproxy/bin/tinyproxy"]
CMD ["-d", "-c", "/tinyproxy/etc/tinyproxy/tinyproxy.conf"]
