#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/debian_buster_gcc_meson .
