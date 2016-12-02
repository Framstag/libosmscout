This is a package that allows you to import OpenStreetMap maps
distributed as PBF or OSM. It contains a compiled import script
(import.cmd), Import program, required DLLs, and the map
stylesheet/typefile.

To use,

1. Unpack this archive

2. To import the map, drag and drop PBF or OSM file on
"import.cmd". This should open a terminal with the Import program
running. The import script will also create a new folder with the name
derived from the map file name. For example, import of

   C:\Users\Aaa\Desktop\xyz-latest.osm.pbf

would import map into C:\Users\Aaa\Desktop\xyz .

3. When import is finished, you would have to press any key to close a
terminal after examining the import process output. Copy the imported
folder to your device (internal disk or SD card) and point the program
using libosmscout to that folder.


Its possible to use import.cmd and bin\Import.exe from terminal as
well. See import.cmd for alternative options.


For direct use of Import.exe:

Copy PBF or OSM into the unpacked folder libosmscout-importer. In
the following exampe, let's use xyz-latest.osm.pbf

1. Open terminal and change to that unpacked folder

2. Create a folder that will be used for importing the map. In the
example, xyz.

3. Run import command from bin\Import.exe . For options and
description of import, see
http://libosmscout.sourceforge.net/tutorials/Importing/ . In example case:

bin\Import.exe --typefile .\stylesheets\map.ost --destinationDirectory xyz xyz-latest.osm.pbf
