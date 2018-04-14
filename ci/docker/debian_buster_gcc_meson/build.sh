#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/debian_sid_gcc_meson .
