#!/bin/bash
cd `dirname $0`
set -x

./archlinux_clang_cmake/run.sh "$@"
RESULT_archlinux_clang_cmake=$?

./archlinux_gcc_cmake/run.sh "$@"
RESULT_archlinux_gcc_cmake=$?

./archlinux_gcc_meson/run.sh "$@"
RESULT_archlinux_gcc_meson=$?

./debian_stretch_gcc_cmake/run.sh "$@"
RESULT_debian_stretch_gcc_cmake=$?

./debian_buster_gcc_meson/run.sh "$@"
RESULT_debian_buster_gcc_meson=$?

./ubuntu_14.04_gcc_cmake/run.sh "$@"
RESULT_ubuntu_14_04_gcc_cmake=$?

./ubuntu_16.04_gcc_cmake/run.sh "$@"
RESULT_ubuntu_16_04_gcc_cmake=$?

./ubuntu_18.04_gcc_cmake/run.sh "$@"
RESULT_ubuntu_17_10_gcc_cmake=$?

# print results
set +x
echo 

echo -ne "archlinux_clang_cmake        "
if [ $RESULT_archlinux_clang_cmake       -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "archlinux_gcc_cmake          "
if [ $RESULT_archlinux_gcc_cmake         -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "archlinux_gcc_meson          "
if [ $RESULT_archlinux_gcc_meson         -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "debian_stretch_gcc_cmake     "
if [ $RESULT_debian_stretch_gcc_cmake    -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "debian_buster_gcc_meson      "
if [ $RESULT_debian_buster_gcc_meson     -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_14.04_gcc_cmake       "
if [ $RESULT_ubuntu_14_04_gcc_cmake      -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_16.04_gcc_cmake       "
if [ $RESULT_ubuntu_16_04_gcc_cmake      -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi

echo -ne "ubuntu_18.04_gcc_cmake       "
if [ $RESULT_ubuntu_17_10_gcc_cmake      -eq 0 ] ; then echo "OK"; else echo "FAILURE"; fi
