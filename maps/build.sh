#!/bin/sh

if [ $# -lt 1 ]; then
  echo "build.sh <mapping file>"
  exit 1
fi

if [[ -x ../Import/src/Import ]]; then
  importExe=../Import/src/Import
elif [[ -x ../debug/Import/Import ]]; then
  importExe=../debug/Import/Import
elif [[ -x ../build/Import/Import ]]; then
  importExe=../build/Import/Import
else
  echo "Cannot find Import executable!"
  exit 1
fi

scriptDirectory="${BASH_SOURCE%/*}"
if [ ! -d "$scriptDirectory" ]; then scriptDirectory="$PWD"; fi

mappingFile="$1"

if [ ! -f "$mappingFile" ]; then
  echo "Mapping file $mappingFile does not exist!"
  exit 1
fi

if [ "$mappingFile" != "${mappingFile%.osm.pbf}" ]; then
  mappingFileBase="${mappingFile%.osm.pbf}"
elif [ "$mappingFile" != "${mappingFile%.osm}" ]; then
  mappingFileBase="${mappingFile%.osm}"
else
  echo "$mapping file is neither an *.osm nor an *.osm.pbf file"
  exit 1
fi

targetDirectory="$mappingFileBase"
outputFile="${mappingFileBase}.txt"

echo -n >$outputFile

echo "Mapping File:" | tee $outputFile
echo " $mappingFile" | tee -a $outputFile

mappingFileOpt="$scriptDirectory/${mappingFileBase}.opt"
defaultOpt="$scriptDirectory/default.opt"

if [ -f "$mappingFileOpt" ]; then
  echo "Options file:" | tee -a $outputFile
  echo " $mappingFileOpt" | tee -a $outputFile
  . "$mappingFileOpt"
elif [ -f "$defaultOpt" ]; then
  echo "Options file:" | tee -a $outputFile
  echo " $defaultOpt" | tee -a $outputFile
  . "$defaultOpt"
fi

boundingPolygonFile="$scriptDirectory/${mappingFileBase}.poly"

if [ -f "$boundingPolygonFile" ]; then
  options="$options --bounding-polygon $boundingPolygonFile"
fi

if [ ! -d "$targetDirectory" ]; then
  echo "Creating target directory $targetDirectory..."
  mkdir "$targetDirectory"
fi

echo "Target directory:" | tee -a $outputFile
echo " $targetDirectory" | tee -a $outputFile
echo "Outputfile:"  | tee -a $outputFile
echo " $outputFile" | tee -a $outputFile
echo "Options:" | tee -a $outputFile
echo " $options" | tee -a $outputFile
echo "Call:" | tee -a $outputFile
echo " $importExe $options --typefile ../stylesheets/map.ost --destinationDirectory $targetDirectory $@" | tee -a $outputFile

$importExe $options --typefile ../stylesheets/map.ost --destinationDirectory "$targetDirectory" "$@" 2>&1 | tee -a $outputFile
