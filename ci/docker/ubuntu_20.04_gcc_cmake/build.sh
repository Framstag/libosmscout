#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/ubuntu_20.04_gcc_cmake .
