#!/bin/bash
cd `dirname $0`
set -x

./archlinux_clang_cmake/run.sh "$@"
RESULT_archlinux_clang_cmake=$?

./archlinux_gcc_cmake/run.sh "$@"
RESULT_archlinux_gcc_cmake=$?

./archlinux_gcc_meson/run.sh "$@"
RESULT_archlinux_gcc_meson=$?

./debian_13_trixie_gcc_meson/run.sh "$@"
RESULT_debian_13_trixie_gcc_meson=$?

./ubuntu_24.04_gcc_cmake/run.sh "$@"
RESULT_ubuntu_24_04_gcc_cmake=$?

./ubuntu_24.04_gcc_meson_qt6/run.sh "$@"
RESULT_ubuntu_24_04_gcc_meson_qt6=$?

./ubuntu_24.04_aarch64_gcc_cmake/run.sh "$@"
RESULT_ubuntu_24_04_aarch64_gcc_cmake=$?


# print results
set +x
echo

echo -ne "archlinux_clang_cmake          "
if [ $RESULT_archlinux_clang_cmake       -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "archlinux_gcc_cmake            "
if [ $RESULT_archlinux_gcc_cmake         -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "archlinux_gcc_meson            "
if [ $RESULT_archlinux_gcc_meson         -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "debian_13_trixie_gcc_meson     "
if [ $RESULT_debian_13_trixie_gcc_meson  -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_24.04_gcc_cmake         "
if [ $RESULT_ubuntu_24_04_gcc_cmake      -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_24.04_gcc_meson_qt6     "
if [ $RESULT_ubuntu_24_04_gcc_meson_qt6  -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_24.04_aarch64_gcc_cmake "
if [ $RESULT_ubuntu_24_04_aarch64_gcc_cmake -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi
