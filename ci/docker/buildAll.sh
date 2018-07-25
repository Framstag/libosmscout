#!/bin/bash
cd `dirname $0`
set -xe

./archlinux_clang_cmake/build.sh
./archlinux_gcc_cmake/build.sh
./archlinux_gcc_meson/build.sh
./debian_stretch_gcc_cmake/build.sh
./debian_buster_gcc_meson/build.sh
./ubuntu_14.04_gcc_cmake/build.sh
./ubuntu_16.04_gcc_cmake/build.sh
./ubuntu_18.04_gcc_cmake/build.sh
