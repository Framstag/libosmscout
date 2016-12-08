#!/bin/sh

git clone https://github.com/Framstag/libosmscout.git libosmscout

cd libosmscout

echo "Generating HTML API documentation..."
( cat doxygen.cfg ; echo "OUTPUT_DIRECTORY=webpage/static/api-doc" ) | doxygen -
echo "Generating static web site..."
cd webpage
hugo --verbose

