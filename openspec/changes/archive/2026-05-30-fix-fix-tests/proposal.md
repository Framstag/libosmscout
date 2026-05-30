## Why

## Why

Previous fix (commit `a465f4093`) added `if(MINGW)` block to `cmake/ProjectConfig.cmake` using `set_tests_properties(... ENVIRONMENT "PATH=...;$ENV{PATH}")`. `$ENV{PATH}` resolves at CMake configure time, not CTest run time. Result: bad PATH overwrites CI shell PATH → all dynamically-linked tests fail with `STATUS_DLL_NOT_FOUND` (0xc0000135).

## What Changes

- **Remove** entire `if(MINGW)` ENVIRONMENT block from `cmake/ProjectConfig.cmake` `osmscout_test_project` macro — `set_tests_properties(... ENVIRONMENT ...)` at configure time replaces CI shell PATH with stale value, breaking all MinGW tests
- Add `$PWD/libosmscout-map-svg` to CI workflow PATH (missing DLL directory for SymbolRendererSVGTest)
- No source code changes to test logic or library behavior

## Capabilities

### New Capabilities
*(none)*

### Modified Capabilities
*(none)*

## Impact

- `cmake/ProjectConfig.cmake` — `if(MINGW)` block removed
- `.github/workflows/build_and test_on_msys.yml` — `$PWD/libosmscout-map-svg` added to test PATH
- No API, ABI, or feature behavior changes

## Capabilities

### New Capabilities
*(none)*

### Modified Capabilities
*(none)*

## Impact

- `cmake/ProjectConfig.cmake` — `osmscout_test_project` macro MINGW PATH logic replaced
- `.github/workflows/build_and test_on_msys.yml` — CI workflow `Run tests` step may need simplification if the PATH workaround becomes unnecessary
- No API, ABI, or feature behavior changes
