You need VisualStudio2015 or higher (C++11 support!).

The build has been tested with VisualStudio 2015.

Build Files for visual studio can be found in the windows directory.
* The project files (*.vcxproj) belong in the corresponding
  sub project folder
* The solution file (*.sln) has to be placed into the top level
  directory.
* Not all sub projects are being build, currently only building
  the base libraries is supported.

The build files currently make the following assumptions:
* zlib, libxml2 and iconv are compiled and placed within the libsosmcout
  top-level directory (in parallel to the libosmscout, libosmscout-import,...
  directories). You can find the binaries for exmaple here:
  http://www.zlatkovic.com/libxml.en.html
  You need to either download the 32 or 64 bit builds. I tested with the
  64bit binaries.
  Include and library search pathes in the projects are configured
  relative to these directories.
* protobuf is not yet configured and tested but should in priciple 
  work similar. You should use protobuf since it offers an dramatic
  performance boost.
* libosmscout-map will follow.