#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/debian_bullseye_gcc_meson .
