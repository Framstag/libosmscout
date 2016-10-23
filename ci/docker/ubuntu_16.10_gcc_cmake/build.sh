#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/ubuntu_16.10_gcc_cmake .
