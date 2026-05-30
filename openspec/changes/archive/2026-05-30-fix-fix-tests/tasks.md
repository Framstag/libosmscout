## 1. Remove MINGW ENVIRONMENT override from CMake macro

- [x] 1.1 Remove entire `if(MINGW)` block from `cmake/ProjectConfig.cmake` `osmscout_test_project` macro — `set_tests_properties(... ENVIRONMENT ...)` at configure time replaces CI shell PATH with wrong value, breaking all tests

## 2. Update CI workflow PATH

- [x] 2.1 Add `$PWD/libosmscout-map-svg` to the PATH in `.github/workflows/build_and test_on_msys.yml` `Run tests` step — SymbolRendererSVGTest needs its DLL directory in PATH since no CMake ENVIRONMENT override anymore
- [x] 2.2 Verify all transitive DLL dependency directories are covered (libosmscout, libosmscout-map, libosmscout-map-svg, libosmscout-import, libosmscout-test)

## 3. Verify

- [ ] 3.1 Confirm CI MinGW tests all pass (no `STATUS_DLL_NOT_FOUND` failures) — pending CI run
- [ ] 3.2 Confirm SymbolRendererSVGTest and HighwayMilestoneFeatureTest still pass — pending CI run
