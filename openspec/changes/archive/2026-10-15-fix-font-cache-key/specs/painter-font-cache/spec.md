# Spec Delta

## Purpose

Defines the contract for the resolved font a map painter keeps for later frames: the font a painter
draws with is the font of the drawing parameters in force for the frame being drawn, never the font
resolved for an earlier set of parameters, so that a painter kept open across a font-name change
draws the new font.

## ADDED Requirements

### Requirement: The drawn font depends on the current drawing parameters

The font a map painter resolves and draws with SHALL be the font of the drawing parameters in force
for the frame being drawn, including the requested font name and the font size the font is resolved
at. It SHALL NOT be a font resolved for a drawing parameter of an earlier frame. A painter that is
kept open across frames SHALL draw a label with the font of the font name requested for that label,
even when a different font name was requested through the same painter before.

#### Scenario: A font-name change on a live painter is drawn with the new font

- **GIVEN** a painter that has drawn a label with one requested font name
- **WHEN** the same label is drawn through that same painter after the requested font name has changed to another name
- **THEN** the drawn text's metrics and ink SHALL be those of the new font name
- **THEN** they SHALL be equal to the metrics and ink of the same label drawn through a painter that only ever used the new name

#### Scenario: Returning to an earlier font name draws that font

- **GIVEN** a painter whose requested font name has changed from one name to another and back to the first
- **WHEN** a label is drawn through that painter
- **THEN** the drawn text's metrics and ink SHALL be those of the first font name
- **THEN** they SHALL NOT be those of the second font name

#### Scenario: Repeated frames under an unchanged font name are unchanged

- **GIVEN** a view that has been drawn through a painter with one font name
- **WHEN** the same view is drawn again through the same painter with the same drawing parameters
- **THEN** the rendered output SHALL be identical to the earlier frame

#### Scenario: The measurement and the drawn font of a frame agree

- **GIVEN** a painter whose label measurement and whose drawing both depend on the requested font name
- **WHEN** the requested font name changes between two frames
- **THEN** the measurement of a label drawn after the change SHALL be the measurement for the new font name
- **THEN** the drawn glyphs of that label SHALL be the glyphs of the new font name

### Requirement: The resolved font of a name stays available

A painter that resolved a font for a font name SHALL be able to draw with that font again after
another font name has been requested, without the painter being reopened.

#### Scenario: Two font names alternate on one painter

- **GIVEN** a painter that has drawn with a first font name and then with a second font name
- **WHEN** it draws with the first font name again and then with the second font name again
- **THEN** each drawing SHALL use the font of the font name it was requested with
