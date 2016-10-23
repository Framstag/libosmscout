#!/bin/sh
cd `dirname $0`
docker build -t libosmscout/ubuntu_15.10_gcc_autoconf .
