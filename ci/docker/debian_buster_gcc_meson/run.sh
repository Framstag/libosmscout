#!/bin/sh

docker run --rm=true -it libosmscout/debian_buster_gcc_meson ./build.sh "$@"
