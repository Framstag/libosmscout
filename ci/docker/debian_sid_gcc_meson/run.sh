#!/bin/sh

docker run --rm=true -it libosmscout/debian_sid_gcc_meson ./build.sh "$@"
