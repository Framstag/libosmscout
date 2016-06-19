#!/bin/sh

echo "Target:     " $TARGET
echo "OS:         " $TRAVIS_OS_NAME
echo "Build tool: " $BUILDTOOL
echo "Compiler:   " $GXX

if [ "$TARGET" = "build" ]; then
  if [ "$TRAVIS_OS_NAME" = "linux" ]; then
    sudo -E apt-get -yq update &>> ~/apt-get-update.log
    
    if [ "$BUILDTOOL" = "autoconf" ]; then
      sudo -E apt-get -yq --no-install-suggests --no-install-recommends --force-yes install \
        autoconf 
    elif [ "$BUILDTOOL" = "cmake" ]; then
      sudo -E apt-get -yq --no-install-suggests --no-install-recommends --force-yes install \
        cmake
    fi
    
    sudo -E apt-get -yq --no-install-suggests --no-install-recommends --force-yes install \
      pkg-config \
      libxml2-dev \
      libprotobuf-dev protobuf-compiler \
      libagg-dev libfreetype6-dev \
      libcairo2-dev libpangocairo-1.0-0 libpango1.0-dev \
      qt5-default qtdeclarative5-dev libqt5svg5-dev qtlocation5-dev \
      freeglut3 freeglut3-dev \
      libmarisa-dev
  elif  [ "$TRAVIS_OS_NAME" = "osx" ]; then
    brew update
    
    if [ "$BUILDTOOL" = "cmake" ]; then
      brew install cmake
    fi
    
    brew install protobuf qt5
  fi
elif [ "$TARGET" = "website" ]; then
  echo "Installing dependencies for website..."
fi

