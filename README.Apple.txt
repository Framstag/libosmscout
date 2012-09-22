A sample XCode project for iOS (iPhone, iPad) and Mac OSX is located in the Apple directory.
Before opening the project in XCode you should do configure in libosmscout and libosmscout-map to generate the config files.
Then you should update the OSMSCOUTDATA, LATITUDE, LONGITUDE, ZOOM defines in the OSMScoutView.m and/or OSMScoutIOSView.m.
Finally you should be able to compile the sample code for the chosen target (OSMScoutOSX or OSMScoutIOS).

TODO - KNOWN BUGS :

- The map is drawn upside-down in Mac OSX, I should integrate the code to flip and translate the Y-axis on this target
- the libosmscout-map-iOSX adapter lib has still some graphic bugs and is not optimized
- make a better sample application
 
