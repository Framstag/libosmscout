# javascout-current-road-info

## Purpose

TBD

## Requirements

### Requirement: Current road info is reported during navigation
The system SHALL report the name, reference, and type of the road the vehicle is currently on during an active navigation session.

#### Scenario: Vehicle on named road
- **GIVEN** an active navigation session
- **WHEN** the vehicle is on a road with a name, reference, and type
- **THEN** the road info overlay shows the road's name, ref, and type

#### Scenario: Road has no name
- **GIVEN** an active navigation session
- **WHEN** the vehicle is on a road without a name tag
- **THEN** the road info overlay shows an empty name
- **AND** the ref and type are still shown if available

#### Scenario: Vehicle off-route
- **GIVEN** an active navigation session
- **WHEN** the vehicle is off the planned route
- **THEN** the road info overlay is hidden

### Requirement: Current road info is displayed in a separate overlay
The system SHALL display the current road information in a dedicated overlay positioned above the next-turn instructions in the top-left corner of the map.

#### Scenario: Road info visible during navigation
- **GIVEN** an active navigation session with road info available
- **WHEN** a position estimate is received
- **THEN** the road info overlay shows the road reference, type, and name in a single line
- **AND** the overlay uses the same font size and styling as the routing instructions

#### Scenario: No road info available
- **GIVEN** an active navigation session
- **WHEN** no road info is available (off-route, unnamed road, etc.)
- **THEN** the road info overlay is hidden
- **AND** the next-turn instructions remain visible

### Requirement: Road info uses DescriptionService lookup at current position
The system SHALL look up road information by querying the DescriptionService at the current geo coordinate, not by extracting it from the route description.

#### Scenario: Data source
- **GIVEN** a position estimate from the navigation engine
- **WHEN** the system looks up road info
- **THEN** it calls `client.getDescription(lat, lon)` at the estimated position
- **AND** parses the returned "General" section entries for "NameRef", "Type", and "Name" labels

#### Scenario: Road info lookup is throttled
- **GIVEN** frequent position updates
- **WHEN** a position update arrives less than 2 seconds after the last lookup
- **THEN** the lookup is skipped
- **AND** the previous road info remains displayed

#### Scenario: Road info lookup skips small movements
- **GIVEN** the vehicle has moved less than ~50 meters since the last lookup
- **WHEN** a position update arrives
- **THEN** the lookup is skipped
- **AND** the previous road info remains displayed
