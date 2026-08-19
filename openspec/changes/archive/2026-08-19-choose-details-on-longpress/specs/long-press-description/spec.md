## MODIFIED Requirements

### Requirement: Long press triggers object description lookup

The system SHALL detect mouse long press on the map canvas and gather all reasonable candidate objects with description data at the pressed coordinate, for user selection.

#### Scenario: Long press on map with multiple objects at location
- **GIVEN** the map is loaded with OSM data
- **WHEN** the user presses and holds the primary mouse button for longer than the configured timeout (default 500ms) at a map location that contains multiple visible objects with description data
- **THEN** the system SHALL query objects in a small bounding box around the coordinate
- **AND** the system SHALL rank candidates by (1) has description data, (2) visible at current zoom, (3) proximity
- **AND** the system SHALL NOT automatically select a single object
- **AND** the system SHALL present the ranked candidate list to the user for selection

#### Scenario: Long press on map with object at location
- **GIVEN** the map is loaded with OSM data
- **WHEN** the user presses and holds the primary mouse button for longer than the configured timeout (default 500ms) at a map location that contains exactly one visible object with description data
- **THEN** the system SHALL query objects in a small bounding box around the coordinate
- **AND** the system SHALL rank candidates by (1) has description data, (2) visible at current zoom, (3) proximity
- **AND** the system SHALL show the description dialog directly for that object
- **AND** the system SHALL NOT show an intermediate candidate list

#### Scenario: Long press on object with no description data
- **GIVEN** the map is loaded with OSM data
- **WHEN** the user long-presses at a location where no object in the bounding box has `DescriptionService` entries
- **THEN** the system SHALL show a dialog with a "No description available" message
- **AND** the system SHALL NOT show a candidate list

#### Scenario: Long press on empty area
- **GIVEN** the map is loaded with OSM data
- **WHEN** the user long-presses at a location with no objects in the bounding box
- **THEN** the system SHALL show a dialog with a "No description available" message
- **AND** the system SHALL NOT produce an error

#### Scenario: Mouse release before timeout cancels long press
- **GIVEN** the map is loaded
- **WHEN** the user presses the primary mouse button
- **AND** releases it before the configured timeout elapses
- **THEN** the system SHALL NOT trigger a description lookup
- **AND** normal tap/drag behavior SHALL proceed

#### Scenario: Mouse drag cancels long press
- **GIVEN** the map is loaded
- **WHEN** the user presses the primary mouse button
- **AND** drags the mouse before the configured timeout elapses
- **THEN** the system SHALL cancel the long-press timer
- **AND** the system SHALL pan the map as normal

#### Scenario: Configurable long-press timeout
- **GIVEN** the user has set `longPressTimeoutMs` to 1000 in config
- **WHEN** the user presses and holds for 600ms
- **THEN** the system SHALL NOT trigger a description lookup
- **WHEN** the user presses and holds for 1000ms
- **THEN** the system SHALL trigger a description lookup

### Requirement: JNI bridge for DescriptionService

The `libosmscout-client-java` library SHALL expose native methods to retrieve object descriptions by coordinate, both for a single object and for the candidate list.

#### Scenario: getDescription returns ObjectDescription
- **GIVEN** a coordinate within the loaded database
- **WHEN** `OSMScoutClient.getDescription(lat, lon)` is called
- **THEN** the method SHALL return an `ObjectDescription` Java object
- **AND** the `ObjectDescription` SHALL contain a list of `DescriptionEntry` objects
- **AND** each `DescriptionEntry` SHALL have `sectionKey`, `subsectionKey`, `hasIndex`, `index`, `labelKey`, and `value` fields

#### Scenario: getDescriptionCandidates returns ranked candidate list
- **GIVEN** a coordinate within the loaded database
- **WHEN** `OSMScoutClient.getDescriptionCandidates(lat, lon)` is called
- **THEN** the method SHALL return a list of `ObjectDescription` objects
- **AND** the list SHALL contain one `ObjectDescription` per reasonable object at the coordinate
- **AND** the list SHALL be ordered by ranking (description data, visibility at current zoom, proximity)
- **AND** each `ObjectDescription` SHALL include the object reference so the selected candidate can be identified

#### Scenario: getDescription outside database
- **GIVEN** a coordinate outside all loaded database bounding boxes
- **WHEN** `OSMScoutClient.getDescription(lat, lon)` is called
- **THEN** the method SHALL return an `ObjectDescription` with zero entries

#### Scenario: getDescription with no database loaded
- **GIVEN** no database is open
- **WHEN** `OSMScoutClient.getDescription(lat, lon)` is called
- **THEN** the method SHALL return an `ObjectDescription` with zero entries

#### Scenario: getDescriptionCandidates outside database
- **GIVEN** a coordinate outside all loaded database bounding boxes
- **WHEN** `OSMScoutClient.getDescriptionCandidates(lat, lon)` is called
- **THEN** the method SHALL return an empty list

#### Scenario: getDescriptionCandidates with no database loaded
- **GIVEN** no database is open
- **WHEN** `OSMScoutClient.getDescriptionCandidates(lat, lon)` is called
- **THEN** the method SHALL return an empty list
