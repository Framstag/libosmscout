# Spec Delta

## MODIFIED Requirements

### Requirement: POI result list

POI search results SHALL be displayed in a list similar to the location search result list, naming each
result's object type by its description rather than by its technical name.

#### Scenario: Results shown with label and type

- **WHEN** POI search results are returned
- **THEN** each entry SHALL show the POI label and, in place of the technical type name, the
  description of the object type in the language the application displays
- **AND** entries SHALL be sorted by distance from the map center (nearest first)

#### Scenario: An entry without a type description falls back

- **WHEN** a result is displayed for an object whose type carries no description in the language the
  application displays
- **THEN** the entry SHALL show the technical type name in place of the description
- **AND** the entry SHALL be displayed without an error

#### Scenario: Result click navigates map

- **WHEN** the user clicks a POI result entry
- **THEN** the map SHALL pan/center to the POI's latitude/longitude

### Requirement: Long-click on result shows details

The system SHALL show the details dialog for a POI when the user long-clicks a search result entry,
naming the object's type by its description.

#### Scenario: Long-click on result opens details

- **WHEN** the user presses and holds the primary mouse button on a POI result entry for longer than the configured long-press timeout
- **THEN** the system SHALL retrieve the description for the POI's coordinates
- **AND** the system SHALL display the details dialog
- **AND** the dialog SHALL name the object's type by its description in the language the application
  displays

#### Scenario: Long-click on result without description data

- **WHEN** the user long-clicks a POI result entry and the POI has no description data
- **THEN** the system SHALL show the details dialog with a "No description available" message
- **AND** the system SHALL NOT produce an error
