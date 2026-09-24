# Spec Delta

## MODIFIED Requirements

### Requirement: Image contents
The container image SHALL contain the regeneration process, the import tool and the basemap import
tool with their runtime dependencies, the type definitions for regional and for basemap databases,
and the utilities needed to download, verify and unpack source data. The image SHALL identify the
exact import tool version it bundles. The image SHALL carry a default coastline source for the
basemap, so that a basemap can be produced without the operator naming one.

#### Scenario: Self-contained image
- **WHEN** the image is started with the required mounts
- **THEN** the full regeneration process including import runs inside the container without host-provided tooling

#### Scenario: Tool version identifiable
- **WHEN** an operator inspects the image
- **THEN** the bundled import tool version is determinable and matches the version recorded in generated database metadata

#### Scenario: Both tools and the basemap type definitions are present
- **WHEN** the image is inspected
- **THEN** the basemap import tool SHALL be present beside the import tool
- **AND** the basemap type definitions SHALL be present for the pass to use
- **AND** the utility needed to unpack the coastline data SHALL be present

#### Scenario: A default coastline source is available
- **GIVEN** a basemap configuration that names no coastline source
- **WHEN** the basemap step fetches coastline data
- **THEN** the location the image carries SHALL be usable without further configuration

### Requirement: Volume mounts
The image SHALL mount three distinct areas: a transient work area for downloads and intermediate import data, the database repository to update, and the configuration holding the imports manifest and the basemap configuration read-only. The basemap inputs the configuration names SHALL be read from the configuration area. The region index used for layout is not part of the configuration; the script reads it from the served root of the repository (`public/names.json`).

#### Scenario: Work area isolated
- **WHEN** an import runs in the container
- **THEN** downloads and intermediate data are confined to the transient work area

#### Scenario: Configuration read-only
- **WHEN** the regeneration process reads configuration
- **THEN** the configuration area is not writable by the process

#### Scenario: Repository updated in place
- **WHEN** a database is placed or pruned
- **THEN** this happens in the mounted database repository

#### Scenario: Basemap inputs come from the configuration area
- **GIVEN** a basemap configuration naming a planet export inside the configuration area
- **WHEN** the basemap step runs
- **THEN** it SHALL read the planet export from the configuration area
- **AND** it SHALL not require the planet export to be reachable as a network source

### Requirement: The bundled type configuration is complete and loads

The image SHALL contain every type definition file it bundles - the regional one and the basemap
one - together with every module those files include, so that each bundled type file loads without
an error. A set of type definitions supplied by the operator through the type file variable SHALL
be treated the same way: the modules have to sit next to the type file, and a missing module SHALL
be reported as a configuration failure rather than as an import failure.

#### Scenario: The bundled type file loads

- **GIVEN** the image as built
- **WHEN** the import tool is run against the bundled type file
- **THEN** it SHALL load the type configuration without reporting a module it cannot read

#### Scenario: Every module is present

- **GIVEN** the type file the image bundles
- **WHEN** the modules it includes are looked up beside it
- **THEN** each of them SHALL exist in the image

#### Scenario: A missing module is a configuration failure

- **GIVEN** a type file whose module cannot be read
- **WHEN** an import starts
- **THEN** the failure SHALL name the module and SHALL be reported as a type configuration failure, before
  any object of the source is processed

#### Scenario: The basemap type definitions load

- **GIVEN** the image as built
- **WHEN** the import tool is run against the bundled basemap type definitions
- **THEN** it SHALL load them without reporting a file it cannot read

## ADDED Requirements

### Requirement: Configuration check covers the basemap configuration

A configuration check run in the container SHALL validate the basemap configuration as part of the
configuration it reports on, and SHALL report a missing or invalid basemap configuration as a
failure of that check.

#### Scenario: Check reports a missing basemap configuration

- **GIVEN** a container started with a configuration check and a configuration area without a basemap configuration
- **WHEN** the check runs
- **THEN** it SHALL fail and name the missing basemap configuration

#### Scenario: Check reports a valid basemap configuration

- **GIVEN** a container started with a configuration check and a complete, valid configuration
- **WHEN** the check runs
- **THEN** it SHALL report the configuration as usable
- **AND** it SHALL not download or import anything
