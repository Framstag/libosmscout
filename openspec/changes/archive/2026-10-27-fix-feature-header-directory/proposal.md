# Proposal

## Why

`osmscout_library_project` renders each module's `<Name>Features.h` from its `.h.cmake` template, and it
works out the directory of the rendered header by taking the parent of the template path. It derives that
parent with `cmake_path(GET <var> PARENT_PATH <var>)`, an in-place form that some CMake 3.22 builds — the one
the Android Gradle plugin ships — do not apply, so the file name stayed in the directory and the header was
written to `<build>/…/<Name>Features.h.cmake/<Name>Features.h`: a path that no include path, no install rule
and no other module reads. A module that names no `FEATURE` therefore compiled against a header that was
never there, and the failure surfaced far from its cause (a missing macro at compile time, a stale
source-tree copy at best).

## What Changes

- The macro derives the template's directory with a path operation that drops the file name on every CMake
  version, so a module that names no `FEATURE` still gets its rendered header at
  `<build>/include/<includedir>/<Name>Features.h`.
- The core library names its rendered header where it actually lives and passes it as `FEATURE`, which is
  what `libosmscout-map` and `libosmscout-client` already do, instead of listing a source-tree copy that no
  build produces.
- No behaviour of a consumer changes: the header keeps its name, its include path and its install
  destination.

## Capabilities

### New Capabilities
- `generated-feature-headers`: where a module's rendered feature header is written, how its path is derived,
  and that the module which renders it also names it — including the install of that header.

### Modified Capabilities
- None. `cmake-install-libdir` and `meson-shader-install` describe install destinations and header contents,
  not the derivation of the rendered header's directory; `ci-build-fixes` collects per-platform CI repairs.

## Impact

- `cmake/ProjectConfig.cmake` — the directory derivation inside `osmscout_library_project`.
- `libosmscout/CMakeLists.txt` — the core library's header list and its `osmscout_library_project` call.
- `openspec/specs/generated-feature-headers/spec.md` (new), `openspec/changes/fix-feature-header-directory/`.
- Unchanged: Meson (it renders the header into its own build tree), every consumer of the headers, the
  install layout, the Android and Apple builds.
