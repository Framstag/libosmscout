# Spec Delta

## Purpose

Lay out and draw a text label in the Cairo backend when no text stack (Pango) is available, wrapping it at
word boundaries as the map parameters ask.

## ADDED Requirements

### Requirement: The variant without a text stack wraps at word boundaries

The Cairo label layout without a text stack SHALL read the line-wrapping map parameters (the minimum and
maximum character count, fit-to-area and fit-to-width) and break a multi-word label into lines that fit the
line width the base class proposes for them. It SHALL measure an average character width from the resolved
font to do so, being the only width source that variant has.

#### Scenario: A multi-word label wider than the allowed line is broken

- **GIVEN** a painter without a text stack and a wrapping parameter whose fit-to-width is half of the
  unwrapped text width
- **WHEN** the label is laid out
- **THEN** it SHALL carry more than one line
- **AND** it SHALL be narrower than the unwrapped label and no wider than the allowed line width (within a
  pixel)

#### Scenario: A word is never broken

- **GIVEN** a single word wider than the allowed line width
- **WHEN** the label is laid out
- **THEN** the word SHALL stay whole on one line
- **AND** the label's width and height SHALL match the unwrapped label

#### Scenario: A short label stays on one line

- **GIVEN** a multi-word label whose length is below the minimum character count of the parameters
- **WHEN** the label is laid out
- **THEN** it SHALL carry one line
- **AND** its width and height SHALL match the unwrapped label

#### Scenario: Every character is drawn once

- **GIVEN** a label wrapped into more than one line
- **WHEN** the glyphs of the laid-out label are counted
- **THEN** the count SHALL be the unwrapped count less one character per line break (the break consumes no
  glyph)

#### Scenario: A label that does not wrap keeps its draw position

- **GIVEN** a label whose text fits the allowed line width
- **WHEN** it is laid out and drawn
- **THEN** its ink height and its draw position SHALL be what they were before wrapping existed

### Requirement: A wrapped label reports the extents of the lines it draws

A wrapped label SHALL carry its lines, each line's ink extents and the baseline advance between two lines.
Its reported width SHALL be the widest line's ink width, not the sum of the lines, and its reported height
SHALL span the stacked lines, so the label layouter places and culls the size that is drawn.

#### Scenario: The reported size is the size that is drawn

- **GIVEN** a label wrapped into more than one line
- **WHEN** its reported width and height are compared with the lines it draws
- **THEN** the width SHALL be the widest line's ink width
- **AND** the height SHALL be greater than the unwrapped label's height

#### Scenario: Drawing and glyph reporting work per line

- **GIVEN** a wrapped label
- **WHEN** it is drawn as a normal text style, as a shield style, and its glyphs are reported
- **THEN** each line SHALL be drawn on its own baseline, centred in the label rectangle
- **AND** the glyphs SHALL be reported in text order with their line's baseline
