#!/bin/bash
cd `dirname $0`
set -x

./archlinux_clang_cmake/run.sh "$@"
RESULT_archlinux_clang_cmake=$?

./archlinux_gcc_cmake/run.sh "$@"
RESULT_archlinux_gcc_cmake=$?

./archlinux_gcc_meson/run.sh "$@"
RESULT_archlinux_gcc_meson=$?

./debian_buster_gcc_meson/run.sh "$@"
RESULT_debian_buster_gcc_meson=$?

./debian_bullseye_gcc_meson/run.sh "$@"
RESULT_debian_bullseye_gcc_meson=$?

./ubuntu_20.04_gcc_cmake/run.sh "$@"
RESULT_ubuntu_20_04_gcc_cmake=$?

./ubuntu_22.04_gcc_cmake/run.sh "$@"
RESULT_ubuntu_22_04_gcc_cmake=$?

./ubuntu_22.04_gcc_meson_qt6/run.sh "$@"
RESULT_ubuntu_22_04_gcc_meson_qt6=$?

./ubuntu_22.04_aarch64_gcc_cmake/run.sh "$@"
RESULT_ubuntu_22_04_aarch64_gcc_cmake=$?

# print results
set +x
echo

echo -ne "archlinux_clang_cmake          "
if [ $RESULT_archlinux_clang_cmake       -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "archlinux_gcc_cmake            "
if [ $RESULT_archlinux_gcc_cmake         -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "archlinux_gcc_meson            "
if [ $RESULT_archlinux_gcc_meson         -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "debian_buster_gcc_meson        "
if [ $RESULT_debian_buster_gcc_meson     -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "debian_bullseye_gcc_meson      "
if [ $RESULT_debian_bullseye_gcc_meson   -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_20.04_gcc_cmake         "
if [ $RESULT_ubuntu_20_04_gcc_cmake      -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_22.04_gcc_cmake         "
if [ $RESULT_ubuntu_22_04_gcc_cmake      -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_22.04_gcc_meson_qt6     "
if [ $RESULT_ubuntu_22_04_gcc_meson_qt6  -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_22.04_aarch64_gcc_cmake "
if [ $RESULT_ubuntu_22_04_aarch64_gcc_cmake -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi
