#!/bin/sh

docker run --rm=true -it libosmscout/debian_stretch_gcc_cmake ./build.sh "$@"
