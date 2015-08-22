Minimum required VisualStudio version is 2013 due tue the code
requirements for certain C++11 features.

Currently supported are the build files for VisualStudio 2015.

"Support" means, that the main developer from time to time is
building the core libraries under Windows to assure that the
code still compiles and improves code quality by handling
certain classes of compiler warnings. libosmscout is not
actively developed under Windows by the main developer.

The set of project files is currrently not complete due to the
rather complex installation of protobuf library, protobuf
compiler and Qt. Scripts that help to setup these 
automatically would be very helpful :-)

Support also means that the main developer is willing to fix
issues with the project files and we certainly do accept
patches.

The projects files are not authorative. They are build to give
a good example how to set up projects under VisualStudio. You
can and should change directories, build options for your
requirements.

Project files for older Versions are left in the repository,
but are not actively mantained anymore and thus may not 
work anymore. We do however accept patches to fix problems.