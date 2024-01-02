#!/bin/sh

docker run --rm=true -it libosmscout/debian_bullseye_gcc_meson ./build.sh "$@"
