#!/bin/sh

echo "Build start time: `date`"

if [ "$TARGET" = "build" ]; then
  if [ "$BUILDTOOL" = "autoconf" ]; then
    make full
    (cd libosmscout/tests && make check)
  elif [ "$BUILDTOOL" = "cmake" ]; then
    mkdir build
    cd build
    cmake ..
    make
  fi
elif [ "$TARGET" = "website" ]; then
  echo "Building website..."
  if [ "$TRAVIS_PULL_REQUEST" = "false" ] && [ "$TRAVIS_BRANCH" = "master" ]; then
    cd webpage
    hugo --verbose
    
    lftp -c "open --user $SOURCEFORGE_USER --password $SOURCEFORGE_PASSWORD sftp://web.sourceforge.net; cd /home/project-web/libosmscout/htdocs/; mirror -R -n --verbose=3 public ."
  else
    echo "This build was triggered from a pull request or a branch, skipping generation of documentation"
  fi  
fi

echo "Build end time: `date`"

