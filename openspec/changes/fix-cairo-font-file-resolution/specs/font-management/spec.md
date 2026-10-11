# Spec Delta

## ADDED Requirements

### Requirement: A configured font name is drawn as the face it names

Every map backend SHALL draw and measure labels with the face named by the font it was configured
with, whether that configuration names a font file or a font family. A configuration naming a
readable font file SHALL be drawn with the face inside that file, both on a host where that file is
installed and on a host where it is not. A configuration naming a font family SHALL keep resolving by
family.

#### Scenario: A font file is drawn as the face of that file

- **GIVEN** a map database and a painter configured with a readable font file that the host does not provide as an installed family
- **WHEN** the map is drawn and the text measured
- **THEN** the drawn labels and the measured metrics SHALL be those of that file's face
- **AND** they SHALL NOT be those of the face the text stack would fall back to for an unknown name

#### Scenario: Both backends that resolve by family draw the file's face

- **GIVEN** the same readable font file configured for the Cairo painter and for the SVG painter's Pango text path
- **WHEN** each draws and measures the same text
- **THEN** each SHALL draw and measure the face of that file

#### Scenario: Both text stacks of a backend draw the file's face

- **GIVEN** the same readable font file configured for a painter that offers more than one text stack
- **WHEN** each text stack draws and measures the same text
- **THEN** the label dimensions and glyph boxes they report SHALL agree within the tolerance the measurement contract sets

#### Scenario: A font family keeps resolving by family

- **GIVEN** a painter configured with a font family the host provides
- **WHEN** the map is drawn and the text measured
- **THEN** the result SHALL be the same as before this requirement existed

#### Scenario: An unresolvable font configuration is reported

- **GIVEN** a painter configured with a name that is neither a readable font file nor a family the host can resolve
- **WHEN** the map is drawn
- **THEN** the backend SHALL report the configured name
- **AND** SHALL draw with a default face instead of silently choosing an unrelated one

#### Scenario: A font file that cannot be read is reported

- **GIVEN** a painter configured with a path to a font file that exists but cannot be read as a font
- **WHEN** the map is drawn
- **THEN** the backend SHALL report that file
- **AND** SHALL NOT draw with the face of a different file
