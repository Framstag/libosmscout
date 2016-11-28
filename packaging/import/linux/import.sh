#!/bin/bash

# Add PBF file into this directory and run ./import.sh <PBFfilename>

set -e

if [ "$#" -ne 1 ]; then
    echo "Usage: ./import.sh openstreetmapfilename"
    exit 0
fi

D=${1%-latest.osm.pbf}
D=${D%.osm.pbf}
D=${D%.pbf} 
D=${D%.osm} 

if [ "$1" == "$D" ] 
then
  D=$1-imported
fi

echo Make directory: $D
mkdir "$D"
./bin/Import --typefile stylesheets/map.ost --delete-temporary-files true --delete-debugging-files true --delete-analysis-files true --delete-report-files true --destinationDirectory "$D" "$1"
