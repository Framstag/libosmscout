# Spec Delta

## Purpose

Render map frames for the Java/JavaFX client through the JNI bridge, including the projection, the optional
overlays and the result the caller reads.

## ADDED Requirements

### Requirement: The render states the label line-wrapping bounds

The JNI render SHALL state all four label line-wrapping values on its rendering parameter instead of
inheriting them: a minimum character count of 5, a maximum of 15, fit-to-area enabled, and fit-to-width the
shorter side of the render buffer, that buffer carrying the pan overrun. The stated fit-to-width SHALL NOT
be the library default of 8000 px, which does not bind in a viewport of any phone or desktop size.

#### Scenario: A long multi-word label is wrapped

- **GIVEN** a viewport whose shorter side is well below the unwrapped width of a long multi-word label in
  the map style
- **WHEN** a frame is rendered
- **THEN** the label SHALL be drawn on more than one line
- **AND** no line SHALL be wider than the shorter side of the buffer

#### Scenario: A one-word label stays on one line

- **GIVEN** a label that is a single word, however long
- **WHEN** a frame is rendered
- **THEN** it SHALL be drawn on one line, because the layouter breaks only at word boundaries

#### Scenario: A short label is not wrapped

- **GIVEN** a label shorter than the minimum character count
- **WHEN** a frame is rendered
- **THEN** it SHALL be drawn on one line

#### Scenario: The bound follows the buffer the caller asked for

- **GIVEN** two render requests that differ only in viewport size
- **WHEN** both frames are rendered
- **THEN** each SHALL wrap against the shorter side of its own buffer
- **AND** the wider request SHALL allow a wider line

### Requirement: The stated bounds are a decision a host can read

The four bounds SHALL be produced by one dependency-free function of the buffer size that the render uses
and a host test can call, so the applied values cannot drift from the stated ones. The function SHALL take
the shorter side: the width for a tall buffer, the height for a wide one.

#### Scenario: A tall buffer bounds by its width

- **GIVEN** a buffer 360 wide and 640 high
- **WHEN** the decision is read
- **THEN** fit-to-width SHALL be 360
- **AND** the character bounds SHALL be 5 and 15 and fit-to-area SHALL be enabled

#### Scenario: A wide buffer bounds by its height

- **GIVEN** a buffer 1024 wide and 768 high
- **WHEN** the decision is read
- **THEN** fit-to-width SHALL be 768

#### Scenario: The decision is not the non-binding library default

- **GIVEN** a buffer of the sizes above
- **WHEN** the decision's fit-to-width is compared with the library default of an unconfigured parameter
- **THEN** it SHALL differ from 8000 px in every case
