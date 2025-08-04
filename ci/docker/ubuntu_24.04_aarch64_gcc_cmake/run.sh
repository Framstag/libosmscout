#!/bin/sh

docker run --rm=true -it libosmscout/ubuntu_24.04_aarch64_gcc_cmake ./build.sh "$@"
