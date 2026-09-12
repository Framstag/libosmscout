## 1. Meson install rules for shaders and fonts

Spec: `meson-shader-install` — "Meson installs the OpenGL shader files"

- [ ] 1.1 Add `install_data()` rules in `libosmscout-map-opengl/meson.build` for the shader files (`data/shaders/*.vert`, `*.frag` — the same set as CMake's `SHADER_FILES`) and the font files (`data/fonts/*`), installing to the platform directories used by CMake
- [ ] 1.2 Verify `meson install` into a clean prefix places the shader files under `<prefix>/share/osmscout/shaders/` (Linux) and the font file under `<prefix>/share/osmscout/fonts/`; verify the installed shader file set matches `libosmscout-map-opengl/data/shaders/`

## 2. Absolute SHADER_INSTALL_DIR in the meson-generated header

Spec: `meson-shader-install` — "Meson-generated SHADER_INSTALL_DIR is absolute and correct"

- [ ] 2.1 In `libosmscout-map-opengl/include/osmscoutmapopengl/meson.build`, compute `SHADER_INSTALL_DIR` (and `DEFAULT_FONT_FILE`) per platform from `get_option('prefix')` using the same mapping CMake uses (Linux `share/osmscout/shaders`, Apple `/Library/Application Support/osmscout/shaders`, Windows `C:/ProgramData/osmscout/shaders`); remove the inline `# TODO: setup installation dir properly`
- [ ] 2.2 Verify the meson-configured `MapOpenGLFeatures.h` defines `SHADER_INSTALL_DIR` as an absolute path (no relative `"shaders"` value) and `DEFAULT_FONT_FILE` pointing into the fonts directory

## 3. Consumer verification

Spec: `meson-shader-install` — "Consumers initialize after meson install" and "Explicit shader path override keeps working"

- [ ] 3.1 After a meson install, run meson-built `OSMScoutOpenGL`, `DrawMapOpenGL`, and `DrawMapAll` from a directory without a `shaders/` subdirectory and without `--shaders`; verify each initializes the OpenGL painter (no `Projection.vert`-style shader-loading failure)
- [ ] 3.2 Verify the `--shaders` override still wins over the default: run a demo with `--shaders <directory>` and confirm it loads shaders from there
- [ ] 3.3 Verify the meson test suite is unaffected: `meson test --timeout-multiplier 2 -C build --print-errorlogs` passes, including the PerformanceTest OpenGL tests that pass `--shaders` explicitly

## 4. Integration verification

Spec: `meson-shader-install` — all requirements

- [ ] 4.1 Rebuild both build systems cleanly (`meson setup build && meson compile -C build`, `cmake --build build`); verify no compile errors
- [ ] 4.2 Verify CMake behavior unchanged: `ctest -j 2 --output-on-failure` passes and CMake-installed apps still resolve shaders from the CMake-installed default
- [ ] 4.3 Add a comment in both build files cross-referencing the platform-path mapping so future drift between CMake and meson is visible; update TODO.md (drop the resolved `Meson relative shader path` item)
