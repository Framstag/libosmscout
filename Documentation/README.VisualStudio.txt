You need VisualStudio 2017 or 2019 (C++17 support!). Most of the code
might also work using Visual Studio 2015, but you likely need to 
make some changes to the code to get it really build.

The build has been tested with VisualStudio 2019. To generate
VisualStudio project files you need to use cmake. Note that
VisualStudio 2017 has direct support for cmake.

For details regarding the cmake build see the central appveyor build
and its configuration files in this repository (appveyor.yml and
ci/appveyor/).

