You need VisualStudio2013 or higher (C++11 support!).

The build has been tested with VisualStudio 2013.

Build Files for visual studio can be found in the windows directory.


The build files currently make the following assumptions:

* zlib, libxml2 and iconv are compiled and places within the libsosmcout
  top-level directory (in parallel to the libosmscout, libosmscout-import,...
  directories). You can find the binaries for exmaple here:
  http://www.zlatkovic.com/libxml.en.html
  You need to either download the 32 or 64 bit builds. I tested with the
  64bit binaries.
  Include and library search pathes in thw projects are configured
  relative to these directories.
* protobuf is not yet configured and tested but should in priciple 
  work similar. You should use protobuf since it offers an dramatic
  performance boost.
* libosmscout-map will follow.