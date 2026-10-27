# Spec Delta

## Purpose

Provide a public measurement API on the map painter base class so external tools can obtain label
dimensions and per-glyph bounding boxes for a given text, font, and font size, consistently across all
rendering backends.

## ADDED Requirements

### Requirement: Measurement states the object width and the wrapping flag

The measurement API SHALL offer an overload that states the width of the object the label belongs to and
whether the text may be wrapped, and SHALL return the label the renderer lays out for those values: a
wrapped label reports the extents of its lines (see the `cairo-label-wrapping` capability). The overload
SHALL have the same contract as the existing measurement in every other respect.

#### Scenario: An unwrapped measurement is the existing measurement

- **GIVEN** a painter and a measurement call that states wrapping disabled
- **WHEN** its result is compared with the measurement that states no object width and no wrapping
- **THEN** the label width, height and glyph positions SHALL be equal within the existing tolerance

#### Scenario: A wrapping measurement reports the wrapped layout

- **GIVEN** a painter and a text wider than the allowed line width of the parameters, with wrapping enabled
- **WHEN** the measurement is called
- **THEN** its label height SHALL be greater than the unwrapped measurement's height
- **AND** its glyph positions SHALL sit on more than one baseline

#### Scenario: The measurement is a public entry point

- **GIVEN** the public header of the map painter
- **WHEN** its public methods are inspected
- **THEN** the wrapping measurement SHALL be declared there
- **AND** a caller SHALL NOT need to know the backend to call it
