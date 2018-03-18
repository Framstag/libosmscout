#!/bin/sh

docker run --rm=true -it libosmscout/archlinux_gcc_meson ./build.sh "$@"
