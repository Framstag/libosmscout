#!/bin/sh

docker run --rm=true -it libosmscout/debian_jessie_gcc_autoconf ./build.sh "$@"
