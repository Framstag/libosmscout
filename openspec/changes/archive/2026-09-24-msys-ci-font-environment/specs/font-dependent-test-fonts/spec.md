# Spec Delta

## Purpose

Defines how the tests that measure text layout and rendering choose and resolve the font they measure, so that their results depend on the font the repository ships rather than on the fonts a build machine or CI runner happens to provide.

## ADDED Requirements

### Requirement: The font-dependent tests measure the repository font

Tests that assert on text layout or glyph metrics and use the font the repository ships SHALL take the font's *family* from the font file itself and SHALL make that file resolvable to the text stack they measure through, instead of naming a font family that the host is expected to provide.

#### Scenario: Family comes from the font file

- **WHEN** a font-dependent test sets up its `MapParameter`
- **THEN** the font name it sets SHALL be the family name stored in the repository font file
- **AND** the test SHALL NOT rely on a literal family name being installed on the host

#### Scenario: The font file is resolvable to the measured text stack

- **WHEN** a font-dependent test measures text through the Cairo/Pango path
- **THEN** the repository font file SHALL be registered with the font configuration of the test process where the font configuration is available
- **AND** the test SHALL select the font configuration backend of the Pango/cairo font map when that backend is available and the environment has not selected one, so that the registration is effective on platforms whose default backend is not font configuration based

#### Scenario: Measured font does not move with the host font set

- **GIVEN** a host that provides no font of the requested family by itself
- **WHEN** the font-dependent tests run
- **THEN** they SHALL measure the repository font
- **AND** their metric results SHALL be the same as on a host that has the family installed

### Requirement: A font file argument is resolved to a family name for family-based backends

Test drivers that accept a font file path SHALL pass a family name to backends that resolve fonts by family, while backends that load a font file directly SHALL keep receiving the path.

#### Scenario: Cairo driver receives a family

- **GIVEN** the performance test is started with `--driver cairo` and a font file path
- **WHEN** the painter's font is configured
- **THEN** the font name SHALL be the family name stored in that file
- **AND** the test SHALL NOT pass the file path to the family-based interface

#### Scenario: File-based drivers keep the path

- **GIVEN** the performance test is started with a driver that loads a font file directly (AGG or OpenGL)
- **WHEN** the painter's font is configured
- **THEN** the font name SHALL remain the file path

#### Scenario: An unreadable font file fails the run

- **GIVEN** the performance test is started with a font file that cannot be read
- **WHEN** the family name cannot be resolved from it
- **THEN** the test SHALL report the font file it could not read
- **AND** SHALL NOT silently measure a different font
