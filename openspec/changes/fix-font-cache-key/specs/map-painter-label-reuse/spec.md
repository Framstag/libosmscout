# Spec Delta

## MODIFIED Requirements

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

#### Scenario: A changed font name is measured and drawn with the new font

- **GIVEN** a backend whose measurement and whose drawing both depend on the requested font name, on a painter that has drawn a frame with one font name
- **WHEN** the requested font name changes and a label is measured and drawn
- **THEN** the measurement SHALL be the measurement for the new font name
- **THEN** the drawn glyphs SHALL be the glyphs of the new font name, so that the drawn font and the measurement agree

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
