#!/bin/sh

# This script is to be called from within meson build

if [ $# -lt 2 ]; then
  echo "cppcheck.sh <meson build directory> <project file>"
  exit 1
fi

echo "cppcheck wrapper script"
echo "  build directory: $1"
echo "  project: $2"

cd $1
echo "Calling cppcheck..."
cppcheck -q --force --enable=all --std=c++11 --xml-version=2 --xml $2 2>cppcheck.xml
echo "Calling cppcheck-htmlreport..."
cppcheck-htmlreport --file cppcheck.xml --report-dir=cppcheck
