#!/bin/bash
cd `dirname $0`
set -xe

./archlinux_clang_autoconf/run.sh   "$@"
./archlinux_gcc_autoconf/run.sh     "$@"
./archlinux_gcc_cmake/run.sh        "$@"
./debian_jessie_gcc_autoconf/run.sh "$@"
./debian_jessie_gcc_cmake/run.sh    "$@"
./ubuntu_14.04_gcc_autoconf/run.sh  "$@"
./ubuntu_14.04_gcc_cmake/run.sh     "$@"
./ubuntu_15.10_gcc_autoconf/run.sh  "$@"
./ubuntu_15.10_gcc_cmake/run.sh     "$@"
./ubuntu_16.04_gcc_autoconf/run.sh  "$@"
./ubuntu_16.04_gcc_cmake/run.sh     "$@"
./ubuntu_16.10_gcc_autoconf/run.sh  "$@"
./ubuntu_16.10_gcc_cmake/run.sh     "$@"
