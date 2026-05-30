## 1. HighwayMilestoneFeatureTest — fix temp path

- [x] 1.1 Replace `/tmp/highway_milestone_minimal_roundtrip.dat` with `std::filesystem::temp_directory_path()` + unique filename in **Round-trip with only ref set** test case
- [x] 1.2 Replace `/tmp/highway_milestone_default_roundtrip.dat` with `std::filesystem::temp_directory_path()` + unique filename in **Round-trip with all defaults** test case
- [x] 1.3 Add `#include <filesystem>` to `HighwayMilestoneFeatureTest.cpp`
- [x] 1.4 Clean up temp files after each test case using `std::filesystem::remove()`

## 2. SymbolRendererSVGTest — fix DLL resolution on MinGW

- [x] 2.1 Add `if(MINGW)` block in `cmake/ProjectConfig.cmake` `osmscout_test_project` macro that appends `$<TARGET_FILE_DIR:OSMScoutMapSVG>` and `$<TARGET_FILE_DIR:OSMScoutMap>` to CTest PATH environment, analogous to existing `if(MSVC)` block
- [x] 2.2 Verify `SymbolRendererSVGTest` links against `osmscout`, `osmscoutmap`, and `osmscoutmapsvg` — confirm `if(MINGW)` PATH includes all three target dirs
