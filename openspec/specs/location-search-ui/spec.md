## Purpose

Describe the search overlay UI in JavaScout for querying locations and navigating the map.

## Requirements

### Requirement: Map overlay search button

JavaScout SHALL display a floating search button over the map canvas.

#### Scenario: Search button visible on startup
- **WHEN** JavaScout starts and database loads
- **THEN** a search button SHALL be visible in the top-right corner of the map panel

#### Scenario: Search button expands on click
- **WHEN** user clicks the search button
- **THEN** the button SHALL animate/expand to reveal a text input field
- **AND** an empty result list SHALL appear below the text field
- **AND** a cancel button SHALL appear to the right of the text field

### Requirement: Search text input

Expanded search area SHALL contain a text field for entering search queries.

#### Scenario: Typing triggers search
- **WHEN** user types text in the search field and presses Enter
- **THEN** `OSMScoutClient.searchLocations()` SHALL be called with the entered text
- **AND** results SHALL be displayed in the result list below

#### Scenario: Empty input clears results
- **WHEN** user clears the search text field
- **THEN** the result list SHALL be empty

### Requirement: Result list display

Search results SHALL be displayed in a list below the search text field.

#### Scenario: Results shown sorted
- **WHEN** search results are returned
- **THEN** they SHALL be displayed in the result list sorted by relevance (highest first)
- **AND** each entry SHALL show the location label and type

#### Scenario: Full-width on small screens
- **WHEN** the window width is less than 600px
- **THEN** the result list SHALL span the full width of the window

#### Scenario: Popup on large screens
- **WHEN** the window width is 600px or more
- **THEN** the result list SHALL appear as a popup/dropdown anchored to the search bar
- **AND** SHALL NOT span the full window width

### Requirement: Cancel button

A cancel button SHALL appear to the right of the search text field when expanded.

#### Scenario: Cancel returns to overview
- **WHEN** user clicks the cancel button
- **THEN** the search field and result list SHALL collapse
- **AND** the search button SHALL reappear
- **AND** the map SHALL return to the overview state (no search overlay visible)

### Requirement: Result click navigates map

Clicking a search result SHALL move the map to the selected location.

#### Scenario: Click pans to location
- **WHEN** user clicks a result entry in the list
- **THEN** the map SHALL pan/center to the location's latitude/longitude
- **AND** the search overlay SHALL collapse back to the search button
