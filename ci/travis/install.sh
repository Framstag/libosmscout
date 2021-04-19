#!/bin/sh

set -e

echo "Target:     " "$TARGET"
echo "OS:         " "$TRAVIS_OS_NAME"
echo "Build tool: " "$BUILDTOOL"
echo "Compiler:   " "$CXX"

echo "Installation start time: $(date)"

echo "web.sourceforge.net ssh-rsa AAAAB3NzaC1yc2EAAAABIwAAAQEA2uifHZbNexw6cXbyg1JnzDitL5VhYs0E65Hk/tLAPmcmm5GuiGeUoI/B0eUSNFsbqzwgwrttjnzKMKiGLN5CWVmlN1IXGGAfLYsQwK6wAu7kYFzkqP4jcwc5Jr9UPRpJdYIK733tSEmzab4qc5Oq8izKQKIaxXNe7FgmL15HjSpatFt9w/ot/CHS78FUAr3j3RwekHCm/jhPeqhlMAgC+jUgNJbFt3DlhDaRMa0NYamVzmX8D47rtmBbEDU3ld6AezWBPUR5Lh7ODOwlfVI58NAf/aYNlmvl2TZiauBCTa7OPYSyXJnIPbQXg6YQlDknNCr0K769EjeIlAfY87Z4tw==" >> "$HOME"/.ssh/known_hosts

export DEBIAN_FRONTEND=noninteractive

if [ "$TARGET" = "build" ]; then
  if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo apt-get -qq update

    if [ "$BUILDTOOL" = "meson" ]; then
      echo "Installing ninja, python and meson..."
      sudo apt-get update
      sudo apt-get install -y \
        ninja-build \
        build-essential python3-pip \
        python3 python3-setuptools

      echo "Installing meson..."
      pip3 install --user meson
    elif [ "$BUILDTOOL" = "cmake" ]; then
      sudo apt-get install -y cmake ninja-build
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
      libicu-dev \
      liblzma-dev
  fi
elif [ "$TARGET" = "website" ]; then
  echo "Installing dependencies for website..."

  wget https://github.com/gohugoio/hugo/releases/download/v0.76.5/hugo_0.76.5_Linux-64bit.deb
  sudo dpkg -i hugo_0.76.5_Linux-64bit.deb

  sudo apt-get -qq update
  sudo apt-get install -y python3-pygments python-pygments doxygen lftp
fi

echo "Installation end time: $(date)"
