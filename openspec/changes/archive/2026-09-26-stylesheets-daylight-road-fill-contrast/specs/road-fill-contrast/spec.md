# Spec Delta

## ADDED Requirements

### Requirement: Daylight road fills keep the way label readable

The daylight fills of the motorway, trunk, primary and secondary road class SHALL be light enough that the way label drawn on them - black in this presentation, without a halo - keeps the contrast a label of that size needs. The fills SHALL stay distinguishable from the light land they are drawn on and from each other.

#### Scenario: The fills of the daylight road classes resolve as the lightened values

- **GIVEN** a style sheet loaded with the daylight presentation
- **WHEN** the line styles of the motorway, trunk, primary and secondary class are resolved at a zoom where the class is drawn cased
- **THEN** each of them resolves a fill stroke and an outline stroke
- **AND** the fill of each class is the lightened value the style sheet declares for it
- **AND** the outline of each class is darker than its fill and is the wider stroke, so the road keeps its border

#### Scenario: The thin variant of a class stays a usable variant of the same class

- **GIVEN** a style sheet loaded with the daylight presentation
- **WHEN** the line styles of the motorway, trunk and primary class are resolved at a zoom below the full road width
- **THEN** each of them resolves a single stroke, without an outline
- **AND** that stroke is the thin variant the style sheet derives from the fill of the class
- **AND** it is lighter than the full width fill of the same class

#### Scenario: The dark presentation keeps its own road fills

- **GIVEN** a style sheet loaded with the dark presentation
- **WHEN** the line styles of a road class are resolved
- **THEN** the fill is the darkened variant of that class
- **AND** it differs from the daylight fill of the same class, so the presentation change stays visible

### Requirement: Constants derived from a road fill follow it

A style sheet SHALL derive the constants it computes from a road fill - the thin variant of the class, the background of the highway shield and the colour of the junction label - from that fill, so that changing a fill keeps every derived constant consistent with it. The background of a highway shield SHALL stay dark enough for the white shield text painted on it.

#### Scenario: A shield background is darker than the road fill

- **GIVEN** a style sheet loaded with the daylight presentation
- **WHEN** the shield style of the motorway, trunk and primary class is resolved
- **THEN** the background of the shield is the darkened variant of the fill of its class
- **AND** it is darker than that fill
- **AND** the shield text is white, so a lighter fill would have been the contrast loss this change removes

#### Scenario: The junction label keeps a step from the fill it is drawn on

- **GIVEN** a style sheet loaded with the daylight presentation
- **WHEN** the fill of the motorway class and the colour of its junction label are resolved
- **THEN** the junction label colour is derived from the motorway fill
- **AND** it differs from that fill, so the label stays visible on the road
