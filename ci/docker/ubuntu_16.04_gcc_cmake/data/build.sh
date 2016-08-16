#!/bin/sh

git clone git://git.code.sf.net/p/libosmscout/code libosmscout

env

cd libosmscout
mkdir build
cd build
cmake ..
make

