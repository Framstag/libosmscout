## ADDED Requirements

### Requirement: A dependency the backends feature-gate is not a configure requirement

A configure of the project SHALL complete when a dependency that the map backends feature-gate is
disabled on the command line, and the corresponding feature gate SHALL then be off in the configured
feature header of the backend that consumes it. Disabling such a dependency SHALL NOT abort the
configure, and a configure that leaves it enabled SHALL still find it and define the gate.

#### Scenario: A configure with the font configuration disabled completes

- **GIVEN** a host whose font configuration is found by the project
- **WHEN** `cmake -B <dir> -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON` is run
- **THEN** the configure SHALL reach `-- Configuring done` and exit 0
- **AND** `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` SHALL be undefined in the configured
  `libosmscout-map-cairo/include/osmscoutmapcairo/MapCairoFeatures.h`

#### Scenario: A configure with FreeType disabled completes

- **GIVEN** a host whose FreeType is found by the project and whose OpenGL map backend is disabled
- **WHEN** `cmake -B <dir> -DCMAKE_BUILD_TYPE=Release -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` is run
- **THEN** the configure SHALL reach `-- Configuring done` and exit 0
- **AND** `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` and `OSMSCOUT_MAP_CAIRO_HAVE_LIB_CAIRO_FT` SHALL be
  undefined in the configured feature header

#### Scenario: A configure that leaves both enabled still finds both

- **GIVEN** a host that provides FreeType and the font configuration
- **WHEN** `cmake -B <dir> -DCMAKE_BUILD_TYPE=Release` is run without disabling either
- **THEN** the configure SHALL exit 0, report `Found Freetype` and `Found Fontconfig`, and define
  `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` and `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` in the configured
  feature header

#### Scenario: The Cairo backend compiles with either dependency disabled

- **GIVEN** a build directory configured as in the two disabled scenarios above
- **WHEN** `cmake --build <dir> --target OSMScoutMapCairo` is run
- **THEN** the target SHALL build and link without an error or warning
