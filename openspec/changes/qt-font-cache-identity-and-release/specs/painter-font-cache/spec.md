# Spec Delta

## ADDED Requirements

### Requirement: The identity of a cached font follows from the drawing parameters at the resolution of the drawing

A painter SHALL select a cached resolved font by the requested font name together with the size the
font is resolved at, quantized to the pixel grid of the drawing, rather than by an incidental
representation of that size. Two labels whose resolved font sizes differ by less than a device pixel
SHALL select the same cached font, and the number of fonts a painter resolves SHALL NOT grow with the
number of labels rendered.

#### Scenario: Sizes that differ by less than a device pixel share one font

- **GIVEN** a painter and two labels whose resolved font sizes differ by less than a device pixel
- **WHEN** both labels are measured and drawn
- **THEN** the painter SHALL serve the second label from the font it resolved for the first
- **THEN** the number of fonts resolved for the two labels SHALL be one

#### Scenario: Many distinct automatic label sizes resolve no font per label

- **GIVEN** a painter and a view whose labels carry automatic sizes derived from the projected boxes of many distinct objects
- **WHEN** the view is rendered
- **THEN** the number of fonts the painter resolves SHALL NOT grow with the number of labels
- **THEN** it SHALL exceed the number of distinct font pixel sizes that occur on screen by no more than the fonts of the other drawing parameters

#### Scenario: Repeated frames under unchanged parameters resolve no further font

- **GIVEN** a view that has been rendered through a painter
- **WHEN** the same view is rendered again with the same drawing parameters
- **THEN** the painter SHALL resolve no font it did not already hold
- **THEN** the rendered output SHALL be identical to the earlier frame

### Requirement: The resolved fonts a painter retains are released

A painter SHALL let a caller release the resolved fonts it retains, SHALL perform that release when
the stylesheet it resolved them for is replaced by a reload, and SHALL release them when it is
destroyed. After a release the painter SHALL retain none of them and SHALL resolve the fonts of a
later frame again, drawing that frame as before the release.

#### Scenario: An explicit release drops the retained fonts

- **GIVEN** a painter that has resolved and drawn with fonts for one font size
- **WHEN** the caller releases the resolved fonts and then draws that size again
- **THEN** the number of fonts the painter retains SHALL be zero at the moment of the release
- **THEN** drawing that size again SHALL resolve a font rather than serve one from the cache
- **THEN** the drawn output SHALL be as before the release

#### Scenario: A stylesheet reload releases the fonts of the replaced stylesheet

- **GIVEN** a painter that has resolved and drawn with fonts for a stylesheet
- **WHEN** the stylesheet is reloaded through that painter
- **THEN** the painter SHALL retain none of the fonts it resolved for the replaced stylesheet
- **THEN** it SHALL resolve them again when a later frame draws with them

### Requirement: Releasing a cached font does not change the rendered output

Releasing a resolved font SHALL NOT change what a painter draws: a font that was released and
resolved again SHALL draw and measure a label as it did before it was released.

#### Scenario: A font resolved again after a release draws as it did before

- **GIVEN** a label drawn through a painter that holds the font it needs
- **WHEN** the same label is drawn through a painter that had to resolve that font again after a release
- **THEN** the drawn glyphs and the measured dimensions SHALL be equal to those of the first drawing

#### Scenario: The measurement and the drawn font of a frame agree under the identity rule

- **GIVEN** a painter whose label measurement and whose font resolution both depend on the requested font name and the font size
- **WHEN** a label is measured and drawn, and another label with a size in the same pixel step is drawn after it
- **THEN** the drawn glyphs of each label SHALL be those of the font the painter resolved for it
- **THEN** the measurement of each label SHALL be the measurement of that font

### Requirement: The retained resolved fonts of a painter are observable

A painter SHALL let a caller read how many resolved fonts it has resolved and how many it retains,
so that the identity rule and the release can be asserted by a test rather than by a memory
measurement.

#### Scenario: A test reads the number of resolved fonts

- **GIVEN** a painter that has rendered a view
- **WHEN** the caller reads the number of fonts the painter has resolved
- **THEN** the value SHALL equal the number of times the painter resolved a font rather than served one from its cache
- **THEN** rendering the same view again SHALL leave the value unchanged

#### Scenario: A test reads the number of retained fonts

- **GIVEN** a painter that has rendered a view
- **WHEN** the caller reads the number of fonts the painter retains after the caller released them
- **THEN** the value SHALL be zero until a later frame resolves a font again
