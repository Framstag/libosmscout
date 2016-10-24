#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/debian_jessie_gcc_cmake .
