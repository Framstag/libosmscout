# Spec Delta

## ADDED Requirements

### Requirement: OpenGL resources install into the data directory under the install prefix

When building with CMake and installing, the OpenGL shader files SHALL be installed under `${CMAKE_INSTALL_DATADIR}/osmscout/shaders` and the OpenGL font files under `${CMAKE_INSTALL_DATADIR}/osmscout/fonts`, expressed relative to the install prefix so that an install prefix chosen at install time is honored.

#### Scenario: Default install places resources under the data directory

- **GIVEN** a CMake build configured with the default install prefix
- **WHEN** the user runs `cmake --install`
- **THEN** the shader files SHALL exist under `<prefix>/share/osmscout/shaders` and the font file under `<prefix>/share/osmscout/fonts`

#### Scenario: Install prefix override is honored

- **GIVEN** a CMake build configured with `-DCMAKE_INSTALL_PREFIX=/usr/local`
- **WHEN** the user runs `cmake --install build --prefix /tmp/oso-install`
- **THEN** the shader and font files SHALL be installed under `/tmp/oso-install/share/osmscout/shaders` and `/tmp/oso-install/share/osmscout/fonts`
- **AND** the install SHALL NOT write into `/usr/local`

#### Scenario: Non-default data directory follows the prefix

- **GIVEN** a CMake build configured with `-DCMAKE_INSTALL_DATADIR=share/libosmscout`
- **WHEN** the user runs `cmake --install`
- **THEN** the shader and font files SHALL be installed under `<prefix>/share/libosmscout/osmscout/shaders` and `<prefix>/share/libosmscout/osmscout/fonts`

### Requirement: Compiled-in OpenGL default paths stay absolute

The CMake-generated `MapOpenGLFeatures.h` SHALL define `SHADER_INSTALL_DIR` and `DEFAULT_FONT_FILE` as absolute paths under the configure-time install prefix and data directory, so applications and demos resolve shaders and fonts from their default path after a normal install, and the explicit shader-path override keeps precedence.

#### Scenario: Generated header reports an absolute default shader directory

- **GIVEN** a CMake build configured with a prefix
- **WHEN** the generated `MapOpenGLFeatures.h` is inspected
- **THEN** `SHADER_INSTALL_DIR` SHALL be an absolute path ending in the platform shader directory
- **AND** `DEFAULT_FONT_FILE` SHALL be an absolute path to the installed font file

#### Scenario: Consumers initialize after install without an override

- **GIVEN** a normal CMake install into the configure-time prefix
- **WHEN** `OSMScoutOpenGL`, `DrawMapOpenGL` or `DrawMapAll` is run without a `--shaders` argument
- **THEN** the OpenGL painter SHALL initialize using the compiled-in default shader path

#### Scenario: Explicit override still wins

- **WHEN** a consumer is invoked with `--shaders <directory>` after an install
- **THEN** the painter SHALL load shaders from the given directory regardless of the compiled-in default
