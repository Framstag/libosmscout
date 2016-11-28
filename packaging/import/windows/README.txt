This is a package that allows you to import OpenStreetMap maps
distributed as PBF or OSM. It contains a compiled Import program,
required DLLs, and the map stylesheet/typefile.

To use,

1. Unpack this archive

2. Copy PBF or OSM into the unpacked folder libosmscout-importer. In
the following exampe, let's use xyz-latest.osm.pbf

3. Open terminal and change to that unpacked folder

4. Create a folder that will be used for importing the map. In the
example, xyz.

5. Run import command from bin\Import.exe . For options and
description of import, see
http://libosmscout.sourceforge.net/tutorials/Importing/ . In example case:

bin\Import.exe --typefile .\stylesheets\map.ost --destinationDirectory xyz xyz-latest.osm.pbf


