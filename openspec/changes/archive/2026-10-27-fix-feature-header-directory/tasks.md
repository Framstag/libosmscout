# Tasks

## 1. Derive the template's directory version-independently

- [x] 1.1 In `cmake/ProjectConfig.cmake`, replace the in-place `cmake_path(GET … PARENT_PATH …)` with a
  directory derivation that drops the file name on every supported CMake version, and state in a comment why
  the in-place form is not used
  (`specs/generated-feature-headers/spec.md#a-rendered-feature-header-lands-on-the-modules-include-path`).

## 2. Name the core library's rendered header

- [x] 2.1 In `libosmscout/CMakeLists.txt`, list the rendered `CoreFeatures.h` of the build tree in
  `HEADER_FILES_LIB` instead of the source-tree path no build produces
  (`specs/generated-feature-headers/spec.md#the-core-library-names-the-header-it-renders`).
- [x] 2.2 Pass that header as `FEATURE` to the `osmscout_library_project` call of `OSMScout`, as the map and
  client libraries already do (same requirement).

## 3. Verify the rendered headers

- [x] 3.1 Configure a fresh build directory and check that every enabled module's rendered `*Features.h` is at
  `<build>/<module>/include/<includedir>/` and that no `<Name>Features.h.cmake/` path exists
  (`specs/generated-feature-headers/spec.md#every-modules-rendered-header-is-where-its-include-path-reads`).
- [x] 3.2 Install that build into a scratch prefix and check that `<prefix>/include/osmscout/lib/CoreFeatures.h`
  exists and carries the configured gates
  (`specs/generated-feature-headers/spec.md#the-rendered-header-of-a-module-is-installed-with-its-include-directory`).
- [x] 3.3 Build the core library target from that build tree, so a translation unit that includes
  `osmscout/lib/CoreFeatures.h` compiles against the rendered file.

## 4. Verify nothing else moved

- [x] 4.1 Run the existing test suite of the build tree (`ctest`) and report the count, so the change is shown
  not to alter behaviour.
- [x] 4.2 Record the commands and their output in `verification.md`, including that the wrong-path case itself
  cannot be reproduced on this host (its CMake applies the in-place form) and why the check therefore pins the
  observable paths instead.
