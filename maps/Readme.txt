Simply download OSM export files (ending in *.osm or *.osm.pbf)
into this directory and call

./build.sh <OSM export file>

The script will create a sub directory for the import data
and call the import tool to generate the data into this
directory.

It will also create a log file containing the output of the import
named <OSM export file>.txt.

Note:
For import of *.osm of *.osm.pbf files you need to have the corresponding
support in the import tool enabled. 

*.osm file require libxml during build and run.

* osm.pbf files require google protobuf compiler and library
at compile time and the library at runtime.
