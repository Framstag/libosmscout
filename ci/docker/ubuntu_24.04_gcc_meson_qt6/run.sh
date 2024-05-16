#!/bin/sh

docker run --rm=true -it libosmscout/ubuntu_22.04_gcc_meson_qt6 ./build.sh "$@"
