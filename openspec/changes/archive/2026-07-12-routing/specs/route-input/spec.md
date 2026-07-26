## ADDED Requirements

### Requirement: Route start/destination input

JavaScout SHALL provide a route panel with two location input fields: start and destination. Each field SHALL reuse the search overlay pattern — user types a location name, sees autocomplete results from `OSMScoutClient.searchLocations()`, and selects one to set the coordinate.

#### Scenario: Enter start location by text search
- **WHEN** user types a location name in the start field
- **THEN** debounced search shows matching results in a dropdown
- **WHEN** user selects a result
- **THEN** the start coordinate is set to the selected location's lat/lon

#### Scenario: Enter destination by text search
- **WHEN** user types a location name in the destination field
- **THEN** debounced search shows matching results in a dropdown
- **WHEN** user selects a result
- **THEN** the destination coordinate is set to the selected location's lat/lon

#### Scenario: Pick start on map via long-press
- **WHEN** user long-presses on the map with the start field active
- **THEN** the coordinate under the cursor is set as the start location

#### Scenario: Pick destination on map via long-press
- **WHEN** user long-presses on the map with the destination field active
- **THEN** the coordinate under the cursor is set as the destination location

#### Scenario: Clear input field
- **WHEN** user clears a location field
- **THEN** the corresponding coordinate is unset and any existing route is cleared

#### Scenario: Calculate button disabled until both fields filled
- **WHEN** either start or destination is empty
- **THEN** the "Calculate Route" button SHALL be disabled
- **WHEN** both fields have valid coordinates
- **THEN** the "Calculate Route" button SHALL be enabled
