#!/bin/sh

set -e

echo "Setting LANG to C.UTF-8:"
export LANG="C.UTF-8"

echo "New locale settings:"
locale

echo "Build start time: $(date)"

if [ "$TARGET" = "importer" ]; then
    packaging/import/linux/build_import.sh
fi

echo "Build end time: $(date)"
