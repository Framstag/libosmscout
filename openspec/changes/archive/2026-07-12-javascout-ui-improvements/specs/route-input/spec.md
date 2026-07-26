## MODIFIED Requirements

### Requirement: Route start/destination input

JavaScout SHALL provide a route panel with two location input fields: start and destination. Each field SHALL reuse the search overlay pattern — user taps a field to open the existing location search, selects a result, and the location is shown as a read-only summary label. The search dialog SHALL pre-fill with the current field value when changing an existing location.

#### Scenario: Enter start location by text search
- **WHEN** user taps the start field
- **THEN** the existing search overlay opens with prompt "Select start"
- **WHEN** user types a location name and selects a result
- **THEN** the start coordinate is set to the selected location's lat/lon
- **AND** the start label shows the location name and admin region hierarchy

#### Scenario: Enter destination by text search
- **WHEN** user taps the destination field
- **THEN** the existing search overlay opens with prompt "Select destination"
- **WHEN** user types a location name and selects a result
- **THEN** the destination coordinate is set to the selected location's lat/lon
- **AND** the destination label shows the location name and admin region hierarchy

#### Scenario: Pick start on map via long-press
- **WHEN** no start is set and user long-presses on the map
- **THEN** the coordinate under the cursor is set as the start location

#### Scenario: Pick destination on map via long-press
- **WHEN** start is set but destination is not, and user long-presses on the map
- **THEN** the coordinate under the cursor is set as the destination location

#### Scenario: Swap start and destination
- **WHEN** user clicks the swap button
- **THEN** start and destination locations are exchanged
- **AND** any existing route is cleared

#### Scenario: Calculate button disabled until both fields filled
- **WHEN** either start or destination is empty
- **THEN** the "Calculate" button SHALL be disabled
- **WHEN** both fields have valid coordinates
- **THEN** the "Calculate" button SHALL be enabled

#### Scenario: Start search pre-fills with current value
- **GIVEN** the start label already shows a named location
- **WHEN** user taps the start field to change it
- **THEN** the search overlay text field SHALL contain the existing start label text
- **AND** a search for that text SHALL be performed automatically

#### Scenario: Destination search pre-fills with current value
- **GIVEN** the destination label already shows a named location
- **WHEN** user taps the destination field to change it
- **THEN** the search overlay text field SHALL contain the existing destination label text
- **AND** a search for that text SHALL be performed automatically
