#!/bin/sh

git clone git://git.code.sf.net/p/libosmscout/code libosmscout

cd libosmscout
. ./setupAutoconf.sh
export CC=clang
export CXX=clang++
env
make full

