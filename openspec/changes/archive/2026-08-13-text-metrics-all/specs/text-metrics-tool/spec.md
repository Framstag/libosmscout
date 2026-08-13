# Text Metrics Tool

**Purpose:** A manually invoked comparison application that renders given text with each available Linux backend, draws the text and its bounding boxes onto a stored canvas, dumps the measured values to the terminal, and compares them against an independent reference measurement.

## ADDED Requirements

### Requirement: Render text with bounding boxes per backend

The tool SHALL render a given text with each available backend, draw the text and its per-glyph bounding boxes onto a canvas, and store the resulting image.

#### Scenario: Image stored per backend
- **GIVEN** a text, a font file, a font size, and an output directory
- **WHEN** the tool is invoked
- **THEN** it SHALL store one image per available backend in the output directory
- **AND** each image SHALL show the text drawn at a common baseline position with the per-glyph bounding boxes overlaid

#### Scenario: Same baseline position across backends
- **GIVEN** a text, a font file, and a font size
- **WHEN** the tool is invoked
- **THEN** all backend images SHALL draw the text at the same baseline position

### Requirement: Dump measured values to terminal

The tool SHALL print the label dimensions and per-glyph bounding boxes as measured by each backend to the terminal.

#### Scenario: Values printed for every backend
- **GIVEN** a text, a font file, and a font size
- **WHEN** the tool is invoked
- **THEN** the terminal output SHALL contain, for each available backend, the label width, label height, and per-glyph bounding box values

#### Scenario: Values printed for every glyph
- **GIVEN** a text with N characters
- **WHEN** the tool is invoked
- **THEN** the terminal output SHALL contain per-glyph bounding box values for all N glyphs of each backend

### Requirement: Independent reference measurement

The tool SHALL compute an independent reference measurement of the per-glyph ink bounding boxes for the given text, font, and font size, without using any backend's measurement code.

#### Scenario: Reference values printed
- **GIVEN** a text, a font file, and a font size
- **WHEN** the tool is invoked
- **THEN** the terminal output SHALL contain the reference per-glyph ink bounding box values alongside the backend values

#### Scenario: Reference uses same font size conversion
- **GIVEN** a font size and rendering parameters
- **WHEN** the tool computes the reference measurement
- **THEN** the reference SHALL use the same pixel size conversion as the backends

### Requirement: Report differences

The tool SHALL report, for each backend, the difference between the backend's measured values and the reference values.

#### Scenario: Difference reported per backend
- **GIVEN** a text, a font file, and a font size
- **WHEN** the tool is invoked
- **THEN** the terminal output SHALL contain, for each backend, the per-glyph difference between the backend's bounding box and the reference bounding box

### Requirement: Skip unavailable backends

The tool SHALL render and measure only with backends that are compiled into the build.

#### Scenario: Backend not compiled in
- **GIVEN** a build without a particular backend
- **WHEN** the tool is invoked
- **THEN** the tool SHALL not fail
- **AND** the terminal output SHALL note that the backend is unavailable
