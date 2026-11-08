# generated-feature-headers

The rendered `<Name>Features.h` of each CMake module: where it is written, how its directory is derived, and
how the module that exports it names it.

## Purpose

Defines where a CMake module's rendered `<Name>Features.h` is written, how that directory is derived from the
template the module names, and how the header the module exports reaches a consumer's include path and the
install tree.

## Requirements

### Requirement: A rendered feature header lands on the module's include path

When a CMake module renders `<Name>Features.h` from an `.h.cmake` template, the build SHALL write the header
to `<build>/include/<includedir>/<Name>Features.h`, for every module and on every CMake version the project
supports. The directory SHALL be derived from the template's own path without depending on an operation whose
result differs between CMake versions.

#### Scenario: A module that names only a template

- **GIVEN** a module whose `osmscout_library_project` call names a template and no rendered header
- **WHEN** the project is configured
- **THEN** `libosmscout/<module>/<...>/<Name>Features.h` SHALL exist in the build tree
- **AND** no path that ends in `<Name>Features.h.cmake/` SHALL exist in the build tree

#### Scenario: A module that names the rendered header itself

- **GIVEN** a module whose call names the rendered header in its build tree
- **WHEN** the project is configured
- **THEN** the rendered header SHALL be written to the named path
- **AND** the module SHALL consume the header from that path

#### Scenario: Every module's rendered header is where its include path reads

- **GIVEN** a configured build tree
- **WHEN** the rendered `*Features.h` files of all enabled modules are collected
- **THEN** each is at `<build>/<module>/include/<includedir>/<Name>Features.h`
- **AND** compiling a translation unit that includes `<includedir>/<Name>Features.h` against that module's
  include directories SHALL read the rendered file

### Requirement: The rendered header of a module is installed with its include directory

A module's rendered feature header SHALL be installed under `<prefix>/include/<includedir>/` next to the
hand-written headers of that directory, so that an installed consumer includes it from the same path it uses
in the build tree.

#### Scenario: Install places the core library's rendered header

- **GIVEN** a configured CMake build
- **WHEN** `cmake --install <dir> --prefix <prefix>` is run
- **THEN** `<prefix>/include/osmscout/lib/CoreFeatures.h` SHALL exist
- **AND** it SHALL define the feature gates of the configured core library

### Requirement: The core library names the header it renders

The core library's header list SHALL name its rendered `CoreFeatures.h` in the build tree rather than a
source-tree copy that no build produces, and its `osmscout_library_project` call SHALL name that header as the
rendered one, as the map and client libraries already do.

#### Scenario: The listed header is the rendered one

- **GIVEN** a configured build tree
- **WHEN** the headers the core library target lists are resolved
- **THEN** the `CoreFeatures.h` entry SHALL resolve to the rendered file in the build tree
- **AND** no entry of the list SHALL resolve to a non-existent source-tree path

#### Scenario: A configured build has no source-tree feature header

- **GIVEN** a source tree of the project
- **WHEN** the project is configured and the core library is built
- **THEN** configure and build SHALL succeed without a `CoreFeatures.h` in the source tree
