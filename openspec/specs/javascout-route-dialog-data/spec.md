# javascout-route-dialog-data

Route dialog pre-fill behavior for JavaScout.

## Purpose

Define how the route location search dialog is pre-filled from the current start or destination value.

## Requirements

### Requirement: Route location search pre-fills from active endpoint
When the user opens the location search dialog from a route input field, the search field SHALL be pre-filled with the current value of that endpoint if it is a named location.

#### Scenario: Start field has a previous value
- **GIVEN** the route start label shows "Dortmund Hbf"
- **WHEN** the user taps the start field to change it
- **THEN** the search overlay SHALL open with "Dortmund Hbf" in the search text field
- **AND** the search results SHALL reflect that query

#### Scenario: Destination field has a previous value
- **GIVEN** the route destination label shows "Berlin Hauptbahnhof"
- **WHEN** the user taps the destination field to change it
- **THEN** the search overlay SHALL open with "Berlin Hauptbahnhof" in the search text field
- **AND** the search results SHALL reflect that query

#### Scenario: Coordinate values are not pre-filled
- **GIVEN** the route start label shows a coordinate string such as "51.514227, 7.465279"
- **WHEN** the user taps the start field
- **THEN** the search overlay SHALL open with an empty search text field
