#!/bin/sh

docker run --rm=true -it libosmscout/ubuntu_24.04_gcc_meson_qt6 ./build.sh "$@"
