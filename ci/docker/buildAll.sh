#!/bin/bash
cd $(dirname $0)
set -xe

./archlinux_clang_cmake/build.sh
./archlinux_gcc_cmake/build.sh
./archlinux_gcc_meson/build.sh
./debian_buster_gcc_meson/build.sh
./debian_bullseye_gcc_meson/build.sh
./debian_bookworm_gcc_meson/build.sh
./ubuntu_22.04_gcc_cmake/build.sh
./ubuntu_22.04_gcc_meson_qt6/build.sh
./ubuntu_22.04_aarch64_gcc_cmake/build.sh
./ubuntu_24.04_gcc_cmake/build.sh
./ubuntu_24.04_gcc_meson_qt6/build.sh
