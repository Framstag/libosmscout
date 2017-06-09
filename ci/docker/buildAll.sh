#!/bin/bash
cd `dirname $0`
set -xe

./archlinux_clang_autoconf/build.sh
./archlinux_gcc_autoconf/build.sh
./archlinux_gcc_cmake/build.sh
./debian_jessie_gcc_autoconf/build.sh
./debian_jessie_gcc_cmake/build.sh
./ubuntu_14.04_gcc_autoconf/build.sh
./ubuntu_14.04_gcc_cmake/build.sh
./ubuntu_15.10_gcc_autoconf/build.sh
./ubuntu_15.10_gcc_cmake/build.sh
./ubuntu_16.04_gcc_autoconf/build.sh
./ubuntu_16.04_gcc_cmake/build.sh
./ubuntu_16.10_gcc_autoconf/build.sh
./ubuntu_16.10_gcc_cmake/build.sh
./ubuntu_17.04_gcc_autoconf/build.sh
./ubuntu_17.04_gcc_cmake/build.sh
