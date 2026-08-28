# OpenGL Performance Test Specification

## Purpose

Defines how the OpenGL driver of the PerformanceTest tool locates its runtime shader resources, reports backend initialization failures, and is registered in both the CMake and meson test suites so OpenGL rendering performance stays covered in development builds.

## Requirements

### Requirement: OpenGL performance test resolves shaders without an install step
The PerformanceTest OpenGL driver SHALL load its shader files from a shader directory that can be provided explicitly at invocation time, so the test runs from a build tree without requiring a prior `cmake --install` step. When no explicit directory is provided, the driver SHALL fall back to the compiled-in default shader directory.

#### Scenario: Explicit shader directory provided
- **WHEN** the test is invoked with an explicit shader directory that contains the required shader files
- **THEN** the OpenGL painter initializes successfully and the test runs without a shader-loading error

#### Scenario: No explicit shader directory and default missing
- **WHEN** the test is invoked without an explicit shader directory and the compiled-in default directory does not contain the required shader files
- **THEN** the test reports a shader-loading failure and exits with a non-zero code

### Requirement: OpenGL backend initialization failure is reported clearly
When the OpenGL backend cannot be initialized — because shader files are missing or no OpenGL context can be created — the test SHALL print an error message that identifies the failing resource and SHALL exit with a non-zero code.

#### Scenario: Missing shader file
- **WHEN** the shader directory does not contain a required shader file
- **THEN** the test prints an error message naming the missing shader file and exits with a non-zero code

#### Scenario: OpenGL context creation fails
- **WHEN** no OpenGL context can be created in the current environment
- **THEN** the test prints an error message about the failed context creation and exits with a non-zero code

### Requirement: CMake registers OpenGL performance tests with build-tree resources
When a CMake build is configured with the OpenGL map backend and GLFW available, the test suite SHALL register `PerformanceTest-opengl-*` tests that pass the shader directory (and other runtime resources) from the source tree, so the tests pass without a prior install step.

#### Scenario: CMake build with OpenGL enabled
- **WHEN** a CMake build is configured with the OpenGL map backend and GLFW, and `ctest` runs the `PerformanceTest-opengl-*` tests
- **THEN** the tests pass without requiring a prior `cmake --install` step

#### Scenario: CMake build without OpenGL
- **WHEN** a CMake build is configured without the OpenGL map backend or without GLFW
- **THEN** no `PerformanceTest-opengl-*` tests are registered

### Requirement: Meson registers OpenGL performance tests
When a meson build is configured with the OpenGL map backend and GLFW is available, the meson test suite SHALL register the PerformanceTest tests — including the OpenGL driver — with the same runtime resources as the CMake registration, so meson builds get the same coverage as CMake builds.

#### Scenario: Meson build with OpenGL enabled
- **WHEN** a meson build is configured with the OpenGL map backend and GLFW is found, and `meson test` runs the PerformanceTest tests
- **THEN** the OpenGL driver tests run and pass without requiring a prior install step

#### Scenario: Meson build without OpenGL or GLFW
- **WHEN** a meson build is configured without the OpenGL map backend or GLFW is not found
- **THEN** no OpenGL driver performance tests are registered
