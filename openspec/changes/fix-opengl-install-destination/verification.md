# Verification: fix-opengl-install-destination

Evidence collected while applying the tasks in `tasks.md`.

## Environment

- `build/` — CMake 4.4.3, Ninja, `CMAKE_BUILD_TYPE=Release`, configure-time prefix `/usr/local`, OpenGL backend enabled.
- `build-meson/` — Meson build.
- Probe configurations and install prefixes were created under the project directory and removed afterwards; `build/` was reconfigured back to its original prefix and rebuilt at the end.
- Database for the consumer runs: `Tests/data/testregion`, stylesheet `stylesheets/standard.oss`.

## 1.1 Prefix-relative install destination

After reconfiguring, `build/libosmscout-map-opengl/cmake_install.cmake` contains:

```
file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/osmscout/shaders" TYPE FILE FILES ...)
file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/osmscout/fonts" TYPE FILE FILES ...)
```

Before the change the same lines carried the configure-time literal (`/usr/local/share/osmscout/shaders`), which is what made the destination absolute.

## 1.2 Compiled-in defaults unchanged

`build/libosmscout-map-opengl/include/osmscoutmapopengl/MapOpenGLFeatures.h` on a default configure:

```
#define SHADER_INSTALL_DIR "/usr/local/share/osmscout/shaders"
#define DEFAULT_FONT_FILE "/usr/local/share/osmscout/fonts/LiberationSans-Regular.ttf"
```

Byte-identical to the pre-change values. The Meson side (`meson-shader-install` parity) is untouched.

## 1.3 Non-default data directory

Fresh configure with `-DCMAKE_INSTALL_DATADIR=share/libosmscout`:

- install script: `file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/libosmscout/osmscout/shaders" ...)` and the matching fonts destination;
- generated header: `<prefix>/share/libosmscout/osmscout/shaders` and `<prefix>/share/libosmscout/osmscout/fonts/LiberationSans-Regular.ttf` — the same directory;
- running that subdirectory's install script with a project-local prefix placed the 11 shader files and the 2 font files under `share/libosmscout/osmscout/{shaders,fonts}`. The `.so` install step failed only because the probe tree was configured but not compiled; the resource files are installed before it.

The fallback that makes this work is recorded in `design.md` (Decision 2): GNUInstallDirs may leave `CMAKE_INSTALL_DATADIR` empty, and an empty component would have turned the relative destination into the absolute path `/osmscout/shaders`.

## 1.4 No new unit test

The change is build configuration only and adds no runtime code, so no unit test applies. Its scenarios are covered by the install commands in 1.1-1.3 and 2.1-2.3; the consumer behaviour is exercised by the existing binaries in 2.3.

## 2.1 Default install unchanged

`cmake --install build --prefix <probe>` (configure-time prefix still `/usr/local`):

- exit 0, 7964 files installed;
- shader files under `<probe>/share/osmscout/shaders`, `LiberationSans-Regular.ttf` and `LICENSE` under `<probe>/share/osmscout/fonts`;
- zero `-- Installing: /usr/local` lines — nothing written outside the chosen prefix.

## 2.2 Prefix override honored

`cmake --install build --prefix <probe>` from the configure-time prefix `/usr/local` completes with exit 0 (previously `file cannot create directory: /usr/local/share/osmscout/shaders`), reaches the subdirectories installed after the OpenGL backend (last lines are `share/osmscout/stylesheets/map.ost`, `lib/cmake/libosmscout/libosmscoutConfig*.cmake`), and writes no file into `/usr/local`.

## 2.3 Consumers resolve the compiled-in default; override still wins

Configured with a project-local prefix and installed normally (`cmake --install build`), so `SHADER_INSTALL_DIR` and `DEFAULT_FONT_FILE` name the installed tree:

| run | result |
|---|---|
| `PerformanceTest --driver opengl` (no `--shaders`), neutral working directory without `./shaders`, shaders present only in the installed prefix | exit 0, `Using driver 'OpenGL'...`, 1 tile drawn, map 37.97 ms, 277 ways / 29 areas |
| same, with the installed `share/osmscout/shaders` renamed away | exit 1 with shader errors — the compiled-in default was the source of the shaders |
| `PerformanceTest --driver opengl --shaders libosmscout-map-opengl/data/shaders` | exit 0, no errors — the override wins |
| `DrawMapOpenGL --database Tests/data/testregion` (no `--shaders`) | exit 0, wrote a 6220817-byte PPM |
| `DrawMapOpenGL ... --shaders libosmscout-map-opengl/data/shaders` | exit 0, same output size |

`DrawMapOpenGL --help` also shows its `--fontName` default as the compiled-in `DEFAULT_FONT_FILE` of the installed prefix, and that file exists there. `OSMScoutOpenGL` is not built in this configuration (`OSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF`), so it was not run; its default-path code (`OSMScoutOpenGL.cpp:45`, `--shaders` at `:231`) is the same one `PerformanceTest` exercises.

## 2.4 Builds compile

- `cmake --build build` — exit 0.
- `meson compile -C build-meson` — exit 0 (Meson is unchanged by this change).

## 2.5 Existing suites pass

- CMake: `ctest -j 2 --output-on-failure` — 137/137 passed (includes the OpenGL `PerformanceTest` cases with their explicit `--shaders` and `--font`).
- Meson: `meson test --timeout-multiplier 2 -C build-meson --print-errorlogs` — Ok 137, Fail 0.

## 2.6 Out-of-scope finding recorded

`TODO.md` gained the group "Pre-existing issues found during implementation of `fix-opengl-install-destination`" with the `cmake/ProjectConfig.cmake:138` MSVC PDB entry, naming the file and line and stating that the fix needs a Windows/MSVC run.

## Lint / static analysis

`clang-tidy` and `uncrustify` cover C++ sources; this change touches only `libosmscout-map-opengl/CMakeLists.txt`, `TODO.md` and the OpenSpec change directory, so neither applies.
