#!/bin/bash

set -e

BUILDTOOL=${BUILDTOOL:-cmake}
COMPILER=${COMPILER:-gcc}

echo "BUILDTOOL: ${BUILDTOOL}"
echo "COMPILER:  ${COMPILER}"

apt-get update

apt-get install -y \
          git make ninja-build libtool pkg-config \
          libxml2-dev libprotobuf-dev protobuf-compiler \
          libagg-dev \
          libfreetype6-dev \
          libcairo2-dev \
          libpangocairo-1.0-0 libpango1.0-dev \
          qt5-default qtdeclarative5-dev libqt5svg5-dev \
          qtlocation5-dev qtpositioning5-dev qttools5-dev-tools qttools5-dev \
          freeglut3 freeglut3-dev \
          libmarisa-dev \
          swig openjdk-8-jdk \
          locales

locale-gen en_US.UTF-8

if [ "${COMPILER}" = "gcc" ] ; then
    apt-get install -y \
          g++
fi
if [ "${COMPILER}" = "clang" ] ; then
    apt-get install -y \
          clang
fi
if [ "${BUILDTOOL}" = "cmake" ] ; then
    apt-get install -y \
          cmake
fi
if [ "${BUILDTOOL}" = "meson" ] ; then
    # Ubuntu:bionic distribute Meson 0.45.1 but libosmscout requires >=0.46.0.
    # lets borrow it from eoan ;-)
    echo "deb http://de.archive.ubuntu.com/ubuntu eoan main universe" >> /etc/apt/sources.list
    apt-get update
    apt-get install -y \
          meson
fi
