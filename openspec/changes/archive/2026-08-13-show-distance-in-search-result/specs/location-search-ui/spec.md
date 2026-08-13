## ADDED Requirements

### Requirement: Result distance display

Each search result entry SHALL display the straight-line distance from the
current map center to the result location, in kilometers, right-aligned in a
smaller font than the entry's primary text.

#### Scenario: Distance shown for each result
- **WHEN** search results are displayed in the result list
- **THEN** each entry SHALL show the distance from the current map center to
  the result location in kilometers
- **AND** the distance SHALL be right-aligned within the entry
- **AND** the distance SHALL be rendered in a smaller font than the entry's
  primary label text

#### Scenario: Distance updates with map center
- **WHEN** the map center changes (e.g. after navigating to a result or
  panning the map)
- **THEN** the displayed distances SHALL be recomputed against the new map
  center on the next search

#### Scenario: Distance formatting
- **WHEN** a result is less than 1 km from the map center
- **THEN** the distance SHALL be shown with a precision that distinguishes
  sub-kilometer results (e.g. one decimal place)
- **AND** the unit "km" SHALL be included in the displayed value
