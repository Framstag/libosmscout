A sample XCode project for iOS (iPhone, iPad) and Mac OSX is located in the Apple directory.
Before opening the project in XCode you should do configure in libosmscout and libosmscout-map to generate the config files.
If your target is iOS you have to comment out #define OSMSCOUT_HAVE_SSE2 1 in libosmscout/include/osmscout/CoreFeatures.h 
and libosmscout/include/osmscout/private/Config.h  
Then you should update the OSMSCOUTDATA, LATITUDE, LONGITUDE, ZOOM defines in the OSMScoutView.m and/or OSMScoutIOSView.m.
Finally you should be able to compile the sample code for the chosen target (OSMScoutOSX or OSMScoutIOS).
