## 1. LocationEntry data class

- [x] 1.1 Create `libosmscout-client-java/java/com/framstag/libosmscout/client/LocationEntry.java` — Java POJO with fields: `label`, `type`, `objectType`, `lat`, `lon`, `region`
- [x] 1.2 Add `LocationEntry` to `libosmscout-client-java/java/meson.build` source list

## 2. JNI bridge for location search

- [x] 2.1 Add `#include <osmscout/location/LocationService.h>` to `OSMScoutClient.cpp`
- [x] 2.2 Implement `Java_com_framstag_libosmscout_client_OSMScoutClient_searchLocations` native function — creates `LocationStringSearchParameter`, calls `SearchForLocationByString()`, converts results to `jobjectArray` of `LocationEntry`
- [x] 2.3 Add `LocationEntry` JNI helper: `jobjectArray` construction with string fields, double coords, string array for region

## 3. Java API on OSMScoutClient

- [x] 3.1 Add `public native LocationEntry[] searchLocations(String query, int limit)` method declaration to `OSMScoutClient.java`

## 4. Search overlay UI (JavaFX)

- [x] 4.1 Create `JavaScout/src/main/java/com/framstag/libosmscout/SearchOverlay.java` — JavaFX control: floating search button, expandable text field, result list, cancel button
- [x] 4.2 Implement responsive layout: full-width result list when window < 600px, popup when >= 600px
- [x] 4.3 Implement result sorting on Java side (type priority × distance × match quality)
- [x] 4.4 Wire cancel button to collapse overlay back to search button

## 5. Integration into JavaScout

- [x] 5.1 Add `SearchOverlay` to `MainController.java` — position over map panel, wire `searchLocations()` calls on Enter
- [x] 5.2 Wire result click to `MapRenderer` pan-to-coord for map navigation
- [x] 5.3 Run search on background thread to avoid blocking JavaFX UI thread

## 6. Build system

- [x] 6.1 Verify `libosmscout-client-java` links against `osmscout` library (needed for `LocationService`)
- [x] 6.2 Verify `JavaScout/pom.xml` picks up new `LocationEntry` class from clientjar dependency
