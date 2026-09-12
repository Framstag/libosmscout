## Why

Running the test suite in a development environment regularly fails: all seven `PerformanceTest-opengl-*.oss` tests exit with a non-zero code. The OpenGL performance test cannot locate the shader files it needs at runtime, so the OpenGL painter fails to initialize and the test aborts. This makes the test suite unreliable for developers who build from source without a prior install step. The same problem exists in meson builds, where the OpenGL performance test is not even registered as a test at all.

## What Changes

- Make the OpenGL performance test resolve shader files reliably in development builds, without requiring a prior `cmake --install` step.
- Ensure the OpenGL performance test reports a clear, actionable error when the OpenGL backend cannot be initialized (e.g. missing shaders, missing display/GL context), instead of failing opaquely.
- Keep the test registered in the CMake test suite so OpenGL rendering performance remains covered where the environment supports it.
- Register the OpenGL performance test in the meson test suite as well, so meson builds get the same coverage as CMake builds.
- Align the OpenGL performance test's runtime resource handling with the existing pattern used by the OpenGL demos and the OSMScoutOpenGL application.

## Capabilities

### New Capabilities
- `opengl-performance-test`: Behavior of the `PerformanceTest` tool's OpenGL driver — how it locates shader resources, how it reports backend initialization failures, and how it is registered as a test in the CMake test suite.

### Modified Capabilities
<!-- No existing spec-level behavior changes. -->

## Impact

- `Tests/src/PerformanceTest.cpp` — `PerformanceTestBackendOGL` constructor and `PrepareBackend`; shader directory resolution and error reporting for the OpenGL driver.
- `Tests/CMakeLists.txt` — test registration for `PerformanceTest-opengl-*`; runtime arguments passed to the test binary.
- `libosmscout-map-opengl/` — shader data files (`data/shaders/*.vert`, `*.frag`) are the runtime resource the test depends on; no library code change expected.
- `Tests/meson.build` — PerformanceTest is built but not registered as a meson test; must be registered (including the OpenGL driver, conditional on GLFW availability) with the same runtime arguments as the CMake registration.
- CI workflows referencing `PerformanceTest` (`sanitize_on_ubuntu_24_04.yml`, `build_and test_on_osx.yml`) — behavior of the OpenGL performance tests under CI must not regress.
