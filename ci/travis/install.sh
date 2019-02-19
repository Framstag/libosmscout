#!/bin/sh

set -e

echo "Target:     " $TARGET
echo "OS:         " $TRAVIS_OS_NAME
echo "Build tool: " $BUILDTOOL
echo "Compiler:   " $CXX

echo "Installation start time: `date`"

export DEBIAN_FRONTEND=noninteractive

if [ "$TARGET" = "build" ]; then
  if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo apt-get -qq update

    if [ "$BUILDTOOL" = "meson" ]; then
      echo "INstalling ninja..."
      wget https://github.com/ninja-build/ninja/releases/download/v1.8.2/ninja-linux.zip
      unzip ninja-linux.zip
      mkdir -p ~/bin
      mv ninja ~/bin
      export PATH=~/bin:$PATH

      echo "Installing python..."
      sudo apt-get update
      sudo apt-get install -y \
        build-essential python3-pip \
        python3 python3-setuptools

      echo "Installing meson..."
      pip3 install --user meson==0.46.0
    elif [ "$BUILDTOOL" = "cmake" ]; then
      sudo apt-get install -y cmake
    fi

    sudo apt-get install -y \
      pkg-config \
      gettext \
      libxml2-dev \
      libprotobuf-dev protobuf-compiler \
      libagg-dev libfreetype6-dev \
      libcairo2-dev libpangocairo-1.0-0 libpango1.0-dev \
      qt5-default qtdeclarative5-dev libqt5svg5-dev qtlocation5-dev \
      qttools5-dev-tools qttools5-dev \
      freeglut3 freeglut3-dev \
      libmarisa-dev \
      libglew-dev \
      libglm-dev \
      libglfw3 libglfw3-dev

  elif  [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update

      # Current images have preinstalled
      # - cmake
      # - libxml2

    if [ "$BUILDTOOL" = "meson" ]; then
      brew install meson || true
    fi

    if  [ "$TRAVIS_OS_NAME" = "osx" ] && [ "$PLATFORM" = "osx" ]; then
      brew unlink python
      brew install gettext protobuf cairo pango qt5 glfw3 glew glm
      brew link --force gettext
      brew link --force qt5
      brew link --force --overwrite python
    fi
  fi
elif [ "$TARGET" = "importer" ]; then
  if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo apt-get -qq update

    if [ "$BUILDTOOL" = "cmake" ]; then
      sudo apt-get install -y cmake
    fi

    sudo apt-get install -y \
      pkg-config \
      libxml2-dev \
      libprotobuf-dev protobuf-compiler \
      libmarisa-dev \
      libicu-dev
  fi
elif [ "$TARGET" = "website" ]; then
  echo "Installing dependencies for website..."

  wget https://github.com/spf13/hugo/releases/download/v0.47/hugo_0.47_Linux-64bit.deb
  sudo dpkg -i hugo_0.47_Linux-64bit.deb

  sudo apt-get -qq update
  sudo apt-get install -y python3-pygments python-pygments doxygen lftp
fi

echo "Installation end time: `date`"
