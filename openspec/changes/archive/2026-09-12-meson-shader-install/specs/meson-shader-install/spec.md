## Purpose

Defines how meson builds deliver the OpenGL shader files and how the meson-generated `MapOpenGLFeatures.h` reports the shader install directory, at parity with CMake, so meson-installed applications and demos resolve shaders from their default path.

## ADDED Requirements

### Requirement: Meson installs the OpenGL shader files
When a meson build is installed, the OpenGL shader files SHALL be installed to a platform-appropriate directory, mirroring the CMake install rules (Linux: `share/osmscout/shaders` under the install prefix; Apple/Windows: the corresponding platform paths). The font files SHALL be installed to the matching fonts directory.

#### Scenario: Meson install places shader files
- **WHEN** `meson install` completes into a clean prefix
- **THEN** the shader files exist under the platform shader directory (e.g. `<prefix>/share/osmscout/shaders/` on Linux) and the font file exists under the fonts directory

#### Scenario: Shader file set matches data dir
- **WHEN** the installed shader directory is compared to `libosmscout-map-opengl/data/shaders/`
- **THEN** it contains the same shader file set (all `*.vert`/`*.frag` files listed by the CMake `SHADER_FILES` variable)

### Requirement: Meson-generated SHADER_INSTALL_DIR is absolute and correct
The meson-generated `MapOpenGLFeatures.h` SHALL define `SHADER_INSTALL_DIR` (and `DEFAULT_FONT_FILE`) as absolute paths that point at the directories the meson install rules populate, so the compiled-in defaults resolve after installation.

#### Scenario: Generated header reports absolute install dir
- **WHEN** a meson build is configured and the generated `MapOpenGLFeatures.h` is inspected
- **THEN** `SHADER_INSTALL_DIR` does not contain the relative value `"shaders"` and expands to the platform shader directory (e.g. `<prefix>/share/osmscout/shaders` on Linux)

#### Scenario: Consumers initialize after meson install
- **WHEN** `meson install` completed and `OSMScoutOpenGL` (or `DrawMapOpenGL`, `DrawMapAll`) is run from a directory without a `shaders/` subdirectory and without a `--shaders` argument
- **THEN** the OpenGL painter initializes successfully using the compiled-in default shader path

### Requirement: Explicit shader path override keeps working
The `--shaders` command-line override used by PerformanceTest and the demos SHALL continue to take precedence over the compiled-in default.

#### Scenario: Override still used by tests
- **WHEN** the meson test suite (or CMake test suite) runs PerformanceTest with its explicit `--shaders` argument pointing into the source tree
- **THEN** the OpenGL performance tests pass unchanged

#### Scenario: Override wins over default
- **WHEN** a demo is invoked with `--shaders <directory>` after a meson install
- **THEN** the painter loads shaders from the given directory, regardless of the compiled-in default
