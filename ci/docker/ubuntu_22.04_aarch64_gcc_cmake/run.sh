#!/bin/sh

docker run --rm=true -it libosmscout/ubuntu_22.04_aarch64_gcc_cmake ./build.sh "$@"
