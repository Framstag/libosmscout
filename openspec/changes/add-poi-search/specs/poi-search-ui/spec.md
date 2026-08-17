## Purpose

Describe the POI search UI in JavaScout: opening the search from the main menu, choosing a POI category, defining the search area, and browsing results with details on long-click.

## ADDED Requirements

### Requirement: POI search opens from main menu

The system SHALL open the POI search UI when the user selects the POI search entry in the main menu.

#### Scenario: Menu entry opens POI search
- **WHEN** the user selects the POI search entry in the main menu
- **THEN** the POI search UI SHALL open
- **AND** the menu SHALL close

### Requirement: POI category selection

The POI search UI SHALL let the user choose a POI category from a list.

#### Scenario: Category list shows supported categories
- **WHEN** the POI search UI opens
- **THEN** the category list SHALL show at least "Hotels", "Restaurants", and "Grocery store"

#### Scenario: Category selection drives search
- **WHEN** the user selects a category and triggers the search
- **THEN** the search SHALL use the selected category
- **AND** results SHALL contain only POIs of that category

### Requirement: Search area slider

The POI search UI SHALL let the user define the size of the search area with a stepped slider.

#### Scenario: Slider has discrete steps
- **WHEN** the POI search UI opens
- **THEN** a slider SHALL be visible with discrete steps
- **AND** each step SHALL correspond to a search radius value

#### Scenario: Slider value used as search radius
- **WHEN** the user moves the slider to a step and triggers the search
- **THEN** the search SHALL use the radius of the selected step

### Requirement: POI search execution

The system SHALL search for POIs of the selected category within the selected search area around the current map center.

#### Scenario: Search around map center
- **WHEN** the user triggers the POI search
- **THEN** the system SHALL search around the current map center
- **AND** the search SHALL use the selected category and radius
- **AND** results SHALL be displayed in the result list

#### Scenario: Search with no results
- **WHEN** the user triggers the POI search and no POIs of the selected category exist within the search area
- **THEN** the result list SHALL be empty
- **AND** the system SHALL NOT produce an error

### Requirement: POI result list

POI search results SHALL be displayed in a list similar to the location search result list.

#### Scenario: Results shown with label and type
- **WHEN** POI search results are returned
- **THEN** each entry SHALL show the POI label and type
- **AND** entries SHALL be sorted by distance from the map center (nearest first)

#### Scenario: Result click navigates map
- **WHEN** the user clicks a POI result entry
- **THEN** the map SHALL pan/center to the POI's latitude/longitude

### Requirement: Long-click on result shows details

The system SHALL show the details dialog for a POI when the user long-clicks a search result entry.

#### Scenario: Long-click on result opens details
- **WHEN** the user presses and holds the primary mouse button on a POI result entry for longer than the configured long-press timeout
- **THEN** the system SHALL retrieve the description for the POI's coordinates
- **AND** the system SHALL display the details dialog

#### Scenario: Long-click on result without description data
- **WHEN** the user long-clicks a POI result entry and the POI has no description data
- **THEN** the system SHALL show the details dialog with a "No description available" message
- **AND** the system SHALL NOT produce an error
