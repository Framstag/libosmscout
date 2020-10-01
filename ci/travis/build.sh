#!/bin/sh

set -e

echo "Setting LANG to C.UTF-8:"
export LANG="C.UTF-8"

echo "New locale settings:"
locale

echo "Build start time: $(date)"

if [ "$TARGET" = "build" ]; then
  if  [ "$TRAVIS_OS_NAME" = "osx" ] && [ "$PLATFORM" = "osx" ] ; then
    export PATH="/usr/local/opt/qt/bin:$PATH"
    export PATH="/usr/local/opt/gettext/bin:$PATH"
    export PATH="/usr/local/opt/libxml2/bin:$PATH"
    export PKG_CONFIG_PATH="/usr/local/opt/libffi/lib/pkgconfig:$PKG_CONFIG_PATH"
  fi

  if [ "$BUILDTOOL" = "meson" ]; then
    # Travis currently cannot build clang + OpenMP (https://github.com/travis-ci/travis-ci/issues/8613)
    if [ "$CXX" = "clang++" ]; then
      meson setup --buildtype debugoptimized --unity on debug -Dopenmp=false
    else
      meson setup --buildtype debugoptimized --unity on debug
    fi

    ninja -C debug
    meson test -C debug --print-errorlogs
  elif [ "$BUILDTOOL" = "cmake" ]; then
    mkdir build
    cd build

    if  [ "$TRAVIS_OS_NAME" = "osx" ] && [ "$PLATFORM" = "ios" ] ; then
      cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/iOS.cmake -DMARISA_INCLUDE_DIRS=/usr/local/include/ -DPKG_CONFIG_EXECUTABLE=/usr/local/bin/pkg-config ..
    else
      cmake ..
    fi

    make

    if  [ "$TRAVIS_OS_NAME" = "osx" ] && [ "$PLATFORM" = "ios" ] ; then
        echo "Skip test execution for iOS platform"
    else
        make test
    fi
  fi
elif [ "$TARGET" = "importer" ]; then
    packaging/import/linux/build_import.sh
elif [ "$TARGET" = "website" ]; then
  echo "Building website..."
  if [ "$TRAVIS_PULL_REQUEST" = "false" ] && [ "$TRAVIS_BRANCH" = "master" ]; then
    echo "Generating HTML API documentation..."
    ( cat doxygen.cfg ; echo "OUTPUT_DIRECTORY=webpage/static/api-doc" ) | doxygen -
    echo "Generating static web site..."
    cd webpage
    hugo --verbose

    if [ -n "$SOURCEFORGE_USER" ]; then
      echo "Copying web site content to sourceforge..."
      lftp -c "open --user $SOURCEFORGE_USER --password $SOURCEFORGE_PASSWORD sftp://web.sourceforge.net; cd /home/project-web/libosmscout/htdocs/; mirror -R -n --verbose=3 public ."
    else
      echo "No user name for web page upload given"
    fi;
  else
    echo "This build was triggered from a pull request or a branch, skipping generation of documentation"
  fi
fi

echo "Build end time: $(date)"
