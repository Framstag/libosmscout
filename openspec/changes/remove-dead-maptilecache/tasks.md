# Tasks

Pure removal of unused code; no spec is created for this change, so each task refers to the proposal's
"What Changes" section.

## 1. Remove the unused tile cache implementation

- [ ] 1.1 Delete `libosmscout-map/include/osmscoutmap/MapTileCache.h` and
  `libosmscout-map/src/osmscoutmap/MapTileCache.cpp`, and drop both entries from
  `libosmscout-map/CMakeLists.txt` (source list and header list) and from
  `libosmscout-map/meson.build`. Verify: `grep -rn "MapTileCache"` over the tree (excluding build
  directories) reports no source reference, and the library builds in CMake and in Meson without
  warnings.
- [ ] 1.2 Verify the removed header is gone from the installed header set: inspect the CMake install
  list and run the Meson install plan for the map library and confirm `osmscoutmap/MapTileCache.h` no
  longer appears.

## 2. Remove the remaining unused declarations

- [ ] 2.1 Remove the unused tile cache reference alias from
  `libosmscout-map/include/osmscoutmap/DataTileCache.h`. Verify: `grep -rn` for the alias reports no
  reference anywhere, and the library builds in both build systems.
- [ ] 2.2 Remove the unused cache cleanup method from `libosmscout-map/include/osmscoutmap/MapService.h`
  and `libosmscout-map/src/osmscoutmap/MapService.cpp`, and keep the two size accessors of the map
  service untouched for the memory budget change. Verify: `grep -rn` for the removed method reports no
  reference, the size accessors are still declared and defined, and the library builds in both build
  systems.

## 3. Documentation and verification

- [ ] 3.1 Note in the documentation of the tile cache which cache is the one in use, so the remaining
  type is unambiguous. Verify: the note names the remaining implementation and the removed one.
- [ ] 3.2 Verify the full test suite still passes (`ctest -j 2 --output-on-failure`) and report any
  test that needed a change; no test is expected to change because nothing that is used was removed.
- [ ] 3.3 Run the static analysis and formatting checks on the touched files (`.clang-tidy`,
  `.uncrustify`) and verify no new findings. Verify the header guard naming and license header of the
  remaining files are unchanged.
