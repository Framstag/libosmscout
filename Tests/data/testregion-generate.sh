#!/bin/bash -e

if [ $# -lt 2 ] ; then
    echo "Too few arguments!"
    echo "Usage:"
    echo "$0 <Import tool binary> <output directory>"
    exit 1
fi

IMPORT="$1"
OUTPUT="$2"
TOPDIR=$(readlink -f "$(dirname "$0")/../..")

if [ -f "$OUTPUT" ] ; then
    echo "$OUTPUT exists already"
    exit 1
fi
TMPDIR=$(mktemp -d)

function cleanup () {
    rm -rf "$TMPDIR"
}

trap cleanup EXIT TERM INT
mkdir -p "$OUTPUT"

wget "http://download.geofabrik.de/europe/czech-republic-latest.osm.pbf" "-O$TMPDIR/czech-republic-latest.osm.pbf"

TESTREGION_DATA="$TMPDIR/testregion.osm.pbf"
TESTREGION_POLY="$TOPDIR/Tests/data/testregion.poly"
osmconvert \
  --verbose \
  --complex-ways \
  "$TMPDIR/czech-republic-latest.osm.pbf" \
  "-B=$TESTREGION_POLY" \
  "-o=$TESTREGION_DATA"

"$IMPORT" \
  -d  \
  --eco true \
  --typefile "$TOPDIR/stylesheets/map.ost" \
  --destinationDirectory "$OUTPUT" \
  --altLangOrder en \
  "$TESTREGION_DATA" \
  "$TESTREGION_POLY"

rm \
  "$OUTPUT/"*html \
  "$OUTPUT/"*.idmap \
  "$OUTPUT/"*.txt \
  "$OUTPUT/areaaddress.dat" \
  "$OUTPUT/coord.dat"
