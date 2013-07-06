Build Files for visual studio can be found in the windows directory.

The build files currently make the following assumptions:
1: zlib, libxml2 and google protocol buffers (pbf) are compiled 
   and installed somewhere.
2: protoc.exe (pbf) compiler) can be found in the path
3: QTDIR environment variable is set to the current Qt dir.
4: Directory to headers for zlib, libxml2 and pbf are in 
   environment variable EXTRALIBS_HEADERS
5: Directory to debug builds of zlib, libxml2 and pbf are in 
   environment variable EXTRALIBS_DEBUGLIBS
6: Directory to release builds of zlib, libxml2 and pbf are in 
   environment variable EXTRALIBS_DEBUGLIBS
   
Happy building!