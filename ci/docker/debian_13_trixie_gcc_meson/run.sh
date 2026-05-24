#!/bin/sh

docker run --rm=true -it libosmscout/debian_13_trixie_gcc_meson ./build.sh "$@"
