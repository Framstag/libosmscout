#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/ubuntu_21.10_aarch64_gcc_cmake .
