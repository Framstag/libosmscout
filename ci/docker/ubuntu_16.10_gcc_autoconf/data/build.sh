#!/bin/sh

git clone git://git.code.sf.net/p/libosmscout/code libosmscout

cd libosmscout
. ./setupAutoconf.sh
env
make full

