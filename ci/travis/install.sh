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
      sudo apt-get install build-essential python3-pip

      echo "Installing python3.5..."
      sudo add-apt-repository -y ppa:deadsnakes/ppa
      sudo apt-get update
      sudo apt-get install -y python3.5

      # Activate python3.5
      sudo rm /usr/bin/python3
      sudo ln -s /usr/bin/python3.5 /usr/bin/python3

      echo "Updating pip..."
      pip3 install --upgrade --user pip

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
      libglm-dev

    echo "deb http://ppa.launchpad.net/keithw/glfw3/ubuntu trusty main" | sudo tee -a /etc/apt/sources.list.d/fillwave_ext.list
    echo "deb-src http://ppa.launchpad.net/keithw/glfw3/ubuntu trusty main" | sudo tee -a /etc/apt/sources.list.d/fillwave_ext.list

    sudo apt-get -qq update
    sudo apt-get --yes --force-yes install libglfw3 libglfw3-dev

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
      libmarisa-dev
  fi
elif [ "$TARGET" = "website" ]; then
  echo "Installing dependencies for website..."

  wget https://github.com/spf13/hugo/releases/download/v0.47/hugo_0.47_Linux-64bit.deb
  sudo dpkg -i hugo_0.47_Linux-64bit.deb

  sudo apt-get -qq update
  sudo apt-get install -y python3-pygments python-pygments doxygen lftp
fi

echo "Installation end time: `date`"
