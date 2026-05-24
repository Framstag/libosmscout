#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/ubuntu_24.04_clang_msan_cmake .
