#!/bin/sh

set -e

echo "Setting LANG to C.UTF-8:"
export LANG="C.UTF-8"

echo "New locale settings:"
locale

echo "Build start time: $(date)"

if [ "$TARGET" = "build" ]; then
  if [ "$BUILDTOOL" = "meson" ]; then
    # Travis currently cannot build clang + OpenMP (https://github.com/travis-ci/travis-ci/issues/8613)
    if [ "$CXX" = "clang++" ]; then
      meson setup --buildtype debugoptimized --unity on debug -Dopenmp=false
    else
      meson setup --buildtype debugoptimized --unity on debug
    fi

    meson compile -C debug
    meson test -C debug --print-errorlogs
  elif [ "$BUILDTOOL" = "cmake" ]; then
    mkdir build
    # cmake -B build -DCMAKE_UNITY_BUILD=ON -Wno-dev -G Ninja .
    (cd build && cmake -Wno-dev -G Ninja ..)
    #cmake --build build
    (cd build && ninja)

    (cd build && ctest -j 2 --output-on-failure)
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
