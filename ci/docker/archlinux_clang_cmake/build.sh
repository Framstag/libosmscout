#!/bin/sh
cd `dirname $0`
docker build --pull -t libosmscout/archlinux_clang_cmake .
