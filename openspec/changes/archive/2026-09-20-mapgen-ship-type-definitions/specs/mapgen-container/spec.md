# Spec Delta

## ADDED Requirements

### Requirement: The bundled type configuration is complete and loads

The image SHALL contain the type definition file it bundles together with every module that file includes, so
that the bundled type file loads without an error. A set of type definitions supplied by the operator through
the type file variable SHALL be treated the same way: the modules have to sit next to the type file, and a
missing module SHALL be reported as a configuration failure rather than as an import failure.

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
