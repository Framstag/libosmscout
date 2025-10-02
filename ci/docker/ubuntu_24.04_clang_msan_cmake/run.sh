#!/bin/sh

# msan requires privileged mode to disable ASLR (address space layout randomization)
docker run --privileged --rm=true -it libosmscout/ubuntu_24.04_clang_msan_cmake ./build.sh "$@"
