# font-dependent-test-fonts Specification

## Purpose
Defines how the tests that measure text layout and rendering choose and resolve the font they measure, so that their results depend on the font the repository ships rather than on the fonts a build machine or CI runner happens to provide.

## Requirements

### Requirement: The font-dependent tests measure the repository font

Tests that assert on text layout or glyph metrics and use the font the repository ships SHALL configure that font by the font file the repository ships, instead of naming a font family the host is expected to provide. The test SHALL pass that configuration on unchanged and SHALL leave the resolution of the file to the backend it measures through.

#### Scenario: Family comes from the font file

- **WHEN** a font-dependent test sets up its `MapParameter`
- **THEN** it SHALL configure the font file the repository ships
- **AND** the family it measures SHALL be the one stored in that file
- **AND** it SHALL NOT rely on a literal family name being installed on the host
- **AND** it SHALL NOT read the family name out of the file itself

#### Scenario: The font file is resolvable to the measured text stack

- **WHEN** a font-dependent test measures text through a backend
- **THEN** the repository font file it configured SHALL be the face that backend measures and draws
- **AND** the test SHALL NOT register the font file with the font configuration of the process itself
- **AND** it SHALL NOT select or override the font map of the text stack it measures through, so that the registration is effective on platforms whose default backend is not font configuration based

#### Scenario: Measured font does not move with the host font set

- **GIVEN** a host that does not provide the repository font as an installed family
- **WHEN** the font-dependent tests run
- **THEN** they SHALL measure the repository font
- **AND** their metric results SHALL be the same as on a host that has it installed

#### Scenario: The test passes its font argument on unchanged

- **WHEN** a font-dependent test configures a backend
- **THEN** the font name it passes SHALL be the argument the test was started with
- **AND** the test SHALL NOT convert a font file into a family name for a backend

#### Scenario: An unreadable font file fails the run

- **GIVEN** the performance test is started with a font file that cannot be read
- **WHEN** the painter's font is configured
- **THEN** the test SHALL report the font file it could not read
- **AND** SHALL NOT silently measure a different font
