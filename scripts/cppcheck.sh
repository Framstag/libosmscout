#!/bin/sh

# This script is to be called from within meson build

if [ $# -lt 2 ]; then
  echo "cppcheck.sh <meson build directory> <project file>"
  exit 1
fi

BUILDDIR="$1"
PROJECT="$2"
echo "cppcheck wrapper script"
echo "  build directory: $BUILDDIR"
echo "  project: $PROJECT"

# Some libraries (like QT) needs build-in compiler macros, 
# cppcheck pre-processor fails otherwise, see the issue https://trac.cppcheck.net/ticket/8956
# to workaround that we will create include with these macros
MACROS=$(mktemp --suffix=.h)
if [ -z "$CXX" ]; then
  # try to determine compiler from project json
  CXX=$(jq -r '.[0].command' < "$PROJECT" | awk '{print $BUILDDIR}')
fi
if [ -n "$CXX" ]; then
  $CXX -dM -E - < /dev/null > "$MACROS"
  echo "  buildin compiler ($CXX) macros: $MACROS"
fi

cd $BUILDDIR
echo "Calling cppcheck..."
cppcheck -q --force --enable=all --std=c++11 --xml-version=2 --include="$MACROS" --xml --project="$PROJECT" 2>cppcheck.xml
echo "Calling cppcheck-htmlreport..."
cppcheck-htmlreport --file cppcheck.xml --report-dir=cppcheck

rm "$MACROS"
