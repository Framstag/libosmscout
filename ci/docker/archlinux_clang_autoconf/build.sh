#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/archlinux_clang_autoconf .
