# Tasks: debug-txt-for-fulltext

## 1. Header Changes

- [x] 1.1 Add `FILENAME_TEXT_POI_TXT`, `FILENAME_TEXT_LOC_TXT`, `FILENAME_TEXT_REGION_TXT`, `FILENAME_TEXT_OTHER_TXT` constants to `TextIndexGenerator` in `libosmscout-import/include/osmscoutimport/GenTextIndex.h`
- [x] 1.2 Add `std::ofstream` members `debugStreamPoi`, `debugStreamLocation`, `debugStreamRegion`, `debugStreamOther` and `#include <fstream>` to `GenTextIndex.h`
- [x] 1.3 Extend `AddKeyStr` signature with `const std::string& typeName` and `std::ostream& debugStream` parameters

## 2. Implementation

- [x] 2.1 Define the four filename constants in `libosmscout-import/src/osmscoutimport/GenTextIndex.cpp`
- [x] 2.2 Register the four `.txt` files as provided analysis files in `TextIndexGenerator::GetDescription()`
- [x] 2.3 Add `OpenDebugStream` helper (opens stream with `std::locale::classic()`, reports error on failure)
- [x] 2.4 Open the four debug streams in `Import()` before adding text data; return false on open failure
- [x] 2.5 Select the matching debug stream alongside the keyset in `AddNodeTextToKeysets`, `AddWayTextToKeysets`, `AddAreaTextToKeysets`
- [x] 2.6 Pass `typeInfo->GetName()` and the selected debug stream to every `AddKeyStr` call
- [x] 2.7 Write one debug line per entry in `AddKeyStr`: `<name> <Node|Way|Area> <typeName> <fileOffset>`

## 3. Verification

- [x] 3.1 Build `libosmscout-import` without errors
- [x] 3.2 Run import on a small OSM file and confirm `textpoi.txt`, `textloc.txt`, `textregion.txt`, `textother.txt` are generated in the destination directory
- [x] 3.3 Confirm each file lists name, object type, type name, and id for node, way, and area entries
- [x] 3.4 Confirm existing tests still pass (`ctest` in build directory)
