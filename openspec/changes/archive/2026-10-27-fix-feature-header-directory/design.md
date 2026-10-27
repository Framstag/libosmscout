# Design

## Context

`cmake/ProjectConfig.cmake` renders a module's feature header from the template the module names:

```
cmake_path(RELATIVE_PATH _template BASE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ OUTPUT_VARIABLE _template_dir)
cmake_path(GET _template_dir PARENT_PATH _template_dir)
set(_feature ${CMAKE_CURRENT_BINARY_DIR}/include/${_template_dir}/${_template_name}.h)
```

The second call stores its result in the variable it reads. CMake documents that the `GET` subcommands take an
input variable and a mandatory output variable, and the in-place form is not part of that contract; on some
3.22 builds it does not overwrite the input, so `_template_dir` keeps the file name. `<Name>Features.h` then
lands in `<build>/…/<Name>Features.h.cmake/<Name>Features.h`, which no include directory, no install rule and
no `HEADER` list reads.

CMake's own bug tracker and downstream projects record a broken `cmake_path(... PARENT_PATH)` in the 3.22.1 /
3.22.2 builds (Zephyr, for one, refuses those versions outright), and the CMake shipped with the Android
Gradle plugin is a 3.22. The project supports that CMake: the Android build imports the tree with it.

## Goals / Non-Goals

**Goals**
- A module that names only a template gets its rendered header at the path its include directory reads, on
  every CMake version the project supports.
- The core library, the only module that did not pass `FEATURE`, names the header it actually renders.
- No change to the header's name, include path, contents or install destination.

**Non-Goals**
- Replacing `cmake_path` elsewhere. Its other uses do not use the in-place form.
- Meson: it renders the feature headers itself and is unaffected.
- Raising the minimum CMake version. Avoiding the fragile form is cheaper than a version floor.

## Decisions

**Derive the directory with `get_filename_component(<var> DIRECTORY)`.** It is a documented, version-stable
way to drop the file name, it exists in every CMake the project supports, and it is what the rest of the
project's older CMake code already uses. The alternative — a separate output variable in `cmake_path` — keeps
the 3.22 bug out of the picture too, but the `PARENT_PATH` implementation is the part that is broken, so a
different command avoids it entirely.

**Let the core library name its rendered header, like the other modules.** `libosmscout-map` and
`libosmscout-client` already pass `FEATURE ${CMAKE_CURRENT_BINARY_DIR}/include/…`, and the macro falls back to
the derivation only when a module names nothing. Naming it for the core library makes the header list honest
(it named a source-tree file no build produces) and removes the dependency on the derivation for the one
module whose header every other module includes.

## Risks / Trade-offs

- **The bug cannot be reproduced on the development host.** Its CMake is 4.4, where the in-place form works,
  so a regression test here cannot show the difference. Verification therefore pins the observable result
  (every module's rendered header at `<build>/…/<includedir>/<Name>Features.h`, no `<Name>Features.h.cmake/`
  path, install placing the header) and the absence of the fragile form in the macro, rather than emulating
  3.22.
- The core library's `HEADER` list now names a file that exists only after `configure_file` has run. The macro
  renders before it creates the target, and the file is in the build tree, so CMake resolves it in the same
  configure; the previous source-tree entry never existed at all.
- `libosmscout/CMakeLists.txt` keeps its explicit `install(FILES ${CMAKE_CURRENT_BINARY_DIR}/…)`; the macro
  installs the same file into the same directory. Installing one file twice into one destination is harmless
  and keeps this change to the two places that were wrong.

## Migration

None. Consumers see the same header, path and content; a build tree configured before this change keeps
whatever header it has until it is configured again.
