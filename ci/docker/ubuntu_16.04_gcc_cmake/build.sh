#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/ubuntu_16.04_gcc_cmake .
