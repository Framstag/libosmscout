## Purpose

Defines the contract for the label stage of the map painter: the work that turns the labels
registered for a frame into the measured, placed labels a frame draws, including the label
overlap state of the frame. It exists so that a label is measured once while its measurement
inputs stay unchanged and so that the label stage's scratch storage is reused across labels
and frames, while the label set, the label placement, the draw order, the rendered output and
the text measurement results stay exactly as they are.

## ADDED Requirements

### Requirement: Label measurement is reused while its inputs are unchanged

A label whose measurement inputs - its text, font, font size, proposed width and whether it is
a path label - are unchanged SHALL NOT be measured again for a later frame; the painter SHALL
reuse the earlier measurement instead. A measurement SHALL be performed whenever any of those
inputs, or a painter parameter the measurement depends on, changes.

#### Scenario: A repeated frame reuses all measurements of unchanged labels

- **GIVEN** a view whose labels have been measured for a rendered frame
- **WHEN** the same view is rendered again with an unchanged stylesheet and unchanged drawing parameters
- **THEN** none of the labels whose measurement inputs are unchanged SHALL be measured again

#### Scenario: More labels do not increase the number of measurements of a repeated frame

- **GIVEN** two views rendered twice, one registering substantially more labels than the other
- **WHEN** the second frame of each view is measured
- **THEN** the number of label measurements performed in each second frame SHALL be zero for the labels the first frame already measured

#### Scenario: A changed measurement input is measured again

- **GIVEN** a label that has been measured at one font size and one proposed width
- **WHEN** the same text is measured at a different font size or a different proposed width
- **THEN** the label SHALL be measured for those inputs
- **THEN** the resulting dimensions SHALL be the dimensions for the new inputs, not those of the earlier measurement

#### Scenario: A changed drawing parameter is measured again

- **GIVEN** a backend whose measurement depends on a painter parameter such as the drawing target's resolution or font settings
- **WHEN** that parameter changes between two frames
- **THEN** the labels SHALL be measured for the new parameter and SHALL NOT be drawn with the measurements of the earlier parameter

#### Scenario: A changed line wrapping parameter is measured again

- **GIVEN** a label that has been measured while the painting parameters put one wrapping width, one minimum and one maximum line length and one wrap-to-area setting in force
- **WHEN** the same label is measured again with a different value of one of those parameters
- **THEN** the label SHALL be measured for the new parameter
- **THEN** a later frame with unchanged parameters SHALL reuse that measurement

#### Scenario: Reuse does not depend on a label having been drawn

- **GIVEN** a label that has been measured for a frame in which it did not take part because it lost the overlap resolution or lay outside the viewport
- **WHEN** that label is registered again in a later frame
- **THEN** its measurement SHALL be reused
- **THEN** its dimensions SHALL equal the dimensions of the same label measured by a painter that never saw it before

### Requirement: Per-glyph data is reused

The per-glyph representation a backend derives from a measured label SHALL be reused when that
label takes part in a later frame, instead of being derived again.

#### Scenario: Glyphs of a label that takes part in two frames are derived once

- **GIVEN** a label that has been drawn in a frame
- **WHEN** the same view is rendered again
- **THEN** the per-glyph data of that label SHALL NOT be derived again

#### Scenario: Reused glyph data equals freshly derived glyph data

- **GIVEN** a label drawn from a reused measurement
- **WHEN** its per-glyph data is compared with the per-glyph data derived from the same text by a painter that never saw it before
- **THEN** the glyphs SHALL be identical in number, order, position and bounding box

#### Scenario: Path labels reuse glyph data as well

- **GIVEN** a path label whose glyphs are placed along a path in a frame
- **WHEN** the same view is rendered again
- **THEN** the glyphs of that path label SHALL NOT be derived again
- **THEN** their positions along the path SHALL be identical to the previous frame

#### Scenario: Glyph data does not outlive the measurement it was derived from

- **GIVEN** a painter that has derived the glyph data of a label whose measurement it no longer remembers
- **WHEN** a later frame draws a label whose text differs from that label
- **THEN** the glyph data drawn for it SHALL be the glyph data of its own text

### Requirement: The measurement cache is bounded, drops the least recently used measurement, and can be switched off

The painter SHALL remember a bounded number of label measurements. When the bound is reached,
the measurement that has not been used for the longest time SHALL be the one that a new
measurement replaces. With the reuse of measurements switched off, the painter SHALL remember no
measurement and SHALL retain no glyph data between frames.

#### Scenario: The measurement cache stays within its bound

- **GIVEN** a painter whose measurement cache is bounded
- **WHEN** it measures more labels than the bound
- **THEN** it SHALL remember at most as many measurements as the bound
- **THEN** the measurements it still remembers SHALL be reused by the next frame

#### Scenario: A measurement that is used again outlives one that is not

- **GIVEN** a painter whose cache is full and holds a measurement that the current frame uses and one that it does not use
- **WHEN** a label with a new measurement key is measured
- **THEN** the measurement that is not used SHALL be the one that is dropped
- **THEN** the measurement that is used SHALL be reused by the next frame

#### Scenario: Reuse can be switched off

- **GIVEN** a painter whose reuse of measurements is switched off
- **WHEN** frames are rendered
- **THEN** the painter SHALL remember no measurement and SHALL retain no glyph data between frames
- **THEN** each frame SHALL measure its labels and derive their glyph data again
- **THEN** the frame SHALL draw the labels as a painter that never saw them before

### Requirement: Label stage scratch storage is reused

The storage the label stage uses for a frame SHALL be reused across labels and across frames
instead of being allocated per object, per label and per frame. The allocation count of the
label stage SHALL NOT grow with the number of labels that are registered but do not take part
in the frame, and SHALL NOT grow with the number of times the same view is rendered.

#### Scenario: A repeated frame does not allocate the frame-wide label state again

- **GIVEN** a view rendered repeatedly
- **WHEN** the frame-wide label state (the overlap state of the frame and the ordered label stores) is set up for the second and later frames
- **THEN** it SHALL NOT be allocated again
- **THEN** the label stage's allocation count SHALL NOT grow with the frame number

#### Scenario: Labels that do not take part in the frame do not allocate

- **GIVEN** a view in which many registered labels lose the overlap resolution or lie outside the viewport
- **WHEN** the frame is rendered
- **THEN** the label stage's allocations SHALL NOT grow proportionally to those labels

#### Scenario: More labels do not raise the label stage's allocation count of a repeated frame

- **GIVEN** two views rendered twice, one registering substantially more labels than the other
- **WHEN** the allocation count of the label stage of the second frame is measured
- **THEN** the heavier view's count SHALL NOT be substantially higher than the lighter view's when both have few labels taking part in the frame

### Requirement: Labels, placement, draw order and rendered output are unchanged

For any view, the label stage SHALL produce the same set of drawn labels, the same placement of
those labels, the same draw order and the same rendered output as before reuse was introduced,
including for labels whose ordering criteria compare equal.

#### Scenario: Label set and placement are unchanged

- **GIVEN** a view rendered with a fixed stylesheet and fixed drawing parameters
- **WHEN** the labels that take part in the frame are enumerated with their positions, sizes and styles
- **THEN** the enumeration SHALL be identical to the enumeration of the same view before the change

#### Scenario: Draw order is unchanged

- **GIVEN** a view whose labels are drawn in a defined order, including labels whose ordering criteria compare equal
- **WHEN** the frame is rendered more than once
- **THEN** the draw order SHALL be identical to the order before the change and identical in every frame

#### Scenario: Rendered output is unchanged

- **GIVEN** a view rendered before the change
- **WHEN** the same view is rendered after the change with the same stylesheet and parameters
- **THEN** the rendered output SHALL be identical

### Requirement: Measurement results stay consistent with the text measurement contract

A measurement obtained from a reused label SHALL report the same label dimensions and the same
per-glyph bounding boxes as a measurement of the same text obtained without reuse.

#### Scenario: Reused and fresh measurements agree

- **GIVEN** the same text, font, font size, proposed width and wrapping mode
- **WHEN** the text is measured once on a fresh painter and once on a painter that measured the same text in an earlier frame
- **THEN** the label dimensions SHALL be equal
- **THEN** the per-glyph positions and bounding boxes SHALL be equal

#### Scenario: Cross-backend measurement consistency is preserved

- **GIVEN** the same text, font, font size and rendering parameters
- **WHEN** the measurement method is called on each of the Cairo, AGG, Qt, Skia and SVG painters after reuse has been introduced
- **THEN** the returned label dimensions and glyph positions SHALL stay equal within the tolerance of the text measurement contract
