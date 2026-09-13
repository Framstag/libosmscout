## Purpose

Defines consistency requirements for text measurement across map backends: the SVG backend's non-Pango variant must not diverge from real font metrics, and the `TextMetricsAll` FreeType reference must report ink height consistent with the ink-semantics contract used by the backends.

## Requirements

### Requirement: SVG backend text metrics are independent of Pango availability
The SVG map backend SHALL report text metrics (width, height, glyph boxes) that are consistent whether the backend is built with or without Pango. The non-Pango variant SHALL measure with real font metrics rather than character-count approximations, so measured widths and heights do not systematically diverge from the Pango build or from other backends.

#### Scenario: Non-Pango and Pango SVG builds measure the same text consistently
- **WHEN** the same text, font, and font size are measured by an SVG build with Pango and by an SVG build without Pango
- **THEN** the reported label width and height differ by at most 10%

#### Scenario: Non-Pango SVG build measures near the ink width
- **WHEN** a non-Pango SVG build measures a label whose actual ink width at 30 px font is 216 px (measured today: 340 px, ~57% over)
- **THEN** the reported width is within 10% of the ink width

#### Scenario: Existing Pango path keeps its behavior
- **WHEN** a Pango-enabled SVG build measures text
- **THEN** the reported metrics remain unchanged from before this change

### Requirement: TextMetricsAll reference reports ink height
The `TextMetricsAll` demo's FreeType reference SHALL report glyph ink height for label heights, consistent with the ink-semantics contract used by the map backends, instead of the font box height.

#### Scenario: Reference height agrees with backend label height
- **WHEN** the `TextMetricsAll` demo measures a label at 30 px font
- **THEN** the reference reports an ink height matching the backends' label height (23 px today, not the 35 px font box)

#### Scenario: Reference height used by text metrics comparisons
- **WHEN** the text metrics tests compare backend measurements against the `TextMetricsAll` reference
- **THEN** the reference's reported height is within the comparison margins without extra per-test tolerance

### Requirement: Measurement drift is caught by the test suite
The text metrics test suite SHALL include comparisons for the non-Pango SVG measurement path and for the reference height semantics, so regressions in either are detected automatically.

#### Scenario: SVG non-Pango path is covered
- **WHEN** the test suite runs on a build without Pango for the SVG backend
- **THEN** a test compares SVG non-Pango measurements against the reference within the defined margins

#### Scenario: Reference height drift is covered
- **WHEN** the text metrics tests run
- **THEN** a comparison asserts the reference's height semantics (ink) match the backends' label height semantics
