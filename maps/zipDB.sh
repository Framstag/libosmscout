#!/bin/sh

if [ $# -lt 1 ]; then
  echo "zipDB.sh <map directory>"
  exit 1
fi

scriptDirectory="${BASH_SOURCE%/*}"
if [[ ! -d "$scriptDirectory" ]]; then scriptDirectory="$PWD"; fi

mapDirectory="$1"

if [ ! -d "$mapDirectory" ]; then
  echo "Map directory $mapDirectory does not exist!"
  exit 1
fi

outputFile="$scriptDirectory/$mapDirectory.zip"

zip -u -9 -du \
    $outputFile \
    "$mapDirectory/types.dat" \
    "$mapDirectory/bounding.dat" \
    "$mapDirectory/nodes.dat" \
    "$mapDirectory/areas.dat" \
    "$mapDirectory/ways.dat" \
    "$mapDirectory/areanode.idx" \
    "$mapDirectory/areaarea.idx" \
    "$mapDirectory/areaway.idx" \
    "$mapDirectory/areasopt.dat" \
    "$mapDirectory/waysopt.dat" \
    "$mapDirectory/location.idx" \
    "$mapDirectory/water.idx" \
    "$mapDirectory/intersections.dat" \
    "$mapDirectory/intersections.idx" \
    "$mapDirectory/router.dat" \
    "$mapDirectory/router2.dat" \
    "$mapDirectory/router.idx"
    
