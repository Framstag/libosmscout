#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/debian_stretch_gcc_cmake .
