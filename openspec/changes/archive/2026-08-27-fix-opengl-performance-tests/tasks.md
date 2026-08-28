## 1. PerformanceTest CLI shader directory option

Spec: `opengl-performance-test` — "OpenGL performance test resolves shaders without an install step"

- [x] 1.1 Add `--shaders` string option to PerformanceTest argument parser (default: `SHADER_INSTALL_DIR`), mirroring the `--shaders` option in `Demos/src/DrawMapOpenGL.cpp`; verify `PerformanceTest --help` lists it with the default value
- [x] 1.2 Pass the resolved shader directory into `PerformanceTestBackendOGL` instead of the hardcoded `SHADER_INSTALL_DIR`; verify the OpenGL driver initializes when invoked with `--shaders <libosmscout-map-opengl>/data/shaders` from the build dir (no shader-loading error, painter initializes)
- [x] 1.3 Verify default behavior unchanged: invoking without `--shaders` still uses `SHADER_INSTALL_DIR` (installed-usage path)

## 2. CMake test registration

Spec: `opengl-performance-test` — "CMake registers OpenGL performance tests with build-tree resources"

- [x] 2.1 Add `--shaders "${CMAKE_CURRENT_SOURCE_DIR}/../libosmscout-map-opengl/data/shaders"` to the `add_test` COMMAND in `Tests/CMakeLists.txt` (alongside the existing `--font`/`--icons` args); verify `ctest -R "PerformanceTest-opengl"` passes without a prior `cmake --install`
- [x] 2.2 Verify non-OpenGL drivers unaffected: `ctest -R "PerformanceTest-(noop|none|cairo|agg|Qt)"` still passes
- [x] 2.3 Verify gating intact: a CMake build without OpenGL backend or without GLFW registers no `PerformanceTest-opengl-*` tests (`ctest -N` shows none)

## 3. Meson test registration

Spec: `opengl-performance-test` — "Meson registers OpenGL performance tests"

- [x] 3.1 In `Tests/meson.build`, register `test()` entries for PerformanceTest mirroring the CMake driver × stylesheet matrix, using `meson.current_source_dir()`-based absolute paths for database, stylesheet, icons, font, and shaders; verify `meson test` discovers the PerformanceTest tests
- [x] 3.2 Gate the OpenGL driver tests on `buildMapOpenGL and glfwDep.found()`; verify a meson build without GLFW registers no OpenGL driver tests
- [x] 3.3 Verify `meson test` runs the OpenGL driver tests successfully from the build dir (shaders resolve, painter initializes)

## 4. Integration verification

Spec: `opengl-performance-test` — all requirements

- [x] 4.1 Rebuild both build systems cleanly and verify no compile errors (`cmake --build build` and `meson compile -C debug`)
- [x] 4.2 Run the full CMake test suite (`ctest -j 2 --output-on-failure`) and verify all tests pass, including all `PerformanceTest-*` tests
- [x] 4.3 Run the full meson test suite (`meson test --timeout-multiplier 2 -C debug --print-errorlogs`) and verify all tests pass, including the new PerformanceTest tests
- [x] 4.4 Verify failure reporting: invoking the OpenGL driver with a bogus `--shaders` directory prints an error naming the missing shader file and exits non-zero
