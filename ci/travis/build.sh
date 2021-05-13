#!/bin/sh

set -e

echo "Setting LANG to C.UTF-8:"
export LANG="C.UTF-8"

echo "New locale settings:"
locale

echo "Build start time: $(date)"

if [ "$TARGET" = "importer" ]; then
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
