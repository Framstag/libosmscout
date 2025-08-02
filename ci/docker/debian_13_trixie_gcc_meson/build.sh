#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/debian_13_trixie_gcc_meson .
