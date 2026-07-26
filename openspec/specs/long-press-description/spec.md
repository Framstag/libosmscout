## Purpose

Describe the behavior of the long-press description feature in JavaScout. On mouse long press, the system finds the most reasonable visible object at the pressed coordinate, retrieves a structured description via `DescriptionService`, and displays it in a dynamic overlay dialog.

## Requirements

### Requirement: Long press triggers object description lookup

The system SHALL detect mouse long press on the map canvas and initiate a description lookup for the most reasonable visible object at the pressed coordinate.

#### Scenario: Long press on map with object at location
- **GIVEN** the map is loaded with OSM data
- **WHEN** the user presses and holds the primary mouse button for longer than the configured timeout (default 500ms) at a map location that contains a visible object with description data
- **THEN** the system SHALL query objects in a small bounding box around the coordinate
- **AND** the system SHALL rank candidates by (1) has description data, (2) visible at current zoom, (3) proximity
- **AND** the system SHALL select the highest-ranked object
- **AND** the system SHALL call `DescriptionService::GetDescription()` on the selected object
- **AND** the system SHALL display the description in an overlay dialog

#### Scenario: Long press on object with no description data
- **GIVEN** the map is loaded with OSM data
- **WHEN** the user long-presses at a location where the closest object has no `DescriptionService` entries
- **THEN** the system SHALL show a dialog with a "No description available" message

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

### Requirement: Description overlay dialog

The system SHALL display the object description in a dynamic overlay dialog that adapts to the `ObjectDescription` structure.

#### Scenario: Dialog shows description sections
- **GIVEN** an `ObjectDescription` with entries in sections "General", "Location", "Contact"
- **WHEN** the description dialog opens
- **THEN** the dialog SHALL display "General" as a section header
- **AND** the dialog SHALL display "Location" as a section header
- **AND** the dialog SHALL display "Contact" as a section header
- **AND** each section SHALL contain its label/value pairs indented below the header

#### Scenario: Dialog shows subsections
- **GIVEN** an `ObjectDescription` with entries having subsection "Lanes" under section "Way"
- **WHEN** the description dialog opens
- **THEN** the dialog SHALL display "Lanes" as a subsection header indented under "Way"
- **AND** label/value pairs under "Lanes" SHALL be further indented

#### Scenario: Dialog shows indexed subsections
- **GIVEN** an `ObjectDescription` with multiple entries having the same subsection key but different index values
- **WHEN** the description dialog opens
- **THEN** the dialog SHALL display the subsection header once
- **AND** SHALL group entries with the same index together
- **AND** SHALL repeat the subsection header for each new index value

#### Scenario: Fullscreen on small screen
- **GIVEN** the window width is less than 600px
- **WHEN** the description dialog opens
- **THEN** the dialog SHALL fill the entire window
- **AND** the content area SHALL be scrollable

#### Scenario: Centered overlay on desktop
- **GIVEN** the window width is 600px or greater
- **WHEN** the description dialog opens
- **THEN** the dialog SHALL appear as a centered overlay
- **AND** SHALL NOT fill the entire window

#### Scenario: Close dialog by clicking outside
- **GIVEN** the description dialog is open
- **WHEN** the user clicks outside the dialog content area
- **THEN** the dialog SHALL close with a fade animation

#### Scenario: Close dialog by Escape key
- **GIVEN** the description dialog is open
- **WHEN** the user presses the Escape key
- **THEN** the dialog SHALL close

#### Scenario: Scrolling long descriptions
- **GIVEN** the description content exceeds the available window height
- **WHEN** the dialog is open
- **THEN** the content area SHALL be scrollable
- **AND** a scroll indicator SHALL be visible

### Requirement: JNI bridge for DescriptionService

The `libosmscout-client-java` library SHALL expose a native method to retrieve object descriptions by coordinate.

#### Scenario: getDescription returns ObjectDescription
- **GIVEN** a coordinate within the loaded database
- **WHEN** `OSMScoutClient.getDescription(lat, lon)` is called
- **THEN** the method SHALL return an `ObjectDescription` Java object
- **AND** the `ObjectDescription` SHALL contain a list of `DescriptionEntry` objects
- **AND** each `DescriptionEntry` SHALL have `sectionKey`, `subsectionKey`, `hasIndex`, `index`, `labelKey`, and `value` fields

#### Scenario: getDescription outside database
- **GIVEN** a coordinate outside all loaded database bounding boxes
- **WHEN** `OSMScoutClient.getDescription(lat, lon)` is called
- **THEN** the method SHALL return an `ObjectDescription` with zero entries

#### Scenario: getDescription with no database loaded
- **GIVEN** no database is open
- **WHEN** `OSMScoutClient.getDescription(lat, lon)` is called
- **THEN** the method SHALL return an `ObjectDescription` with zero entries
