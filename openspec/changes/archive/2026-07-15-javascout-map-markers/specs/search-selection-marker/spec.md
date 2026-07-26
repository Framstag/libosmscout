## ADDED Requirements

### Requirement: JavaScout renders a marker for the selected search result
JavaScout SHALL render a marker for the currently selected search result. The marker SHALL be previewed while the user highlights a result in the search result list and SHALL persist at the navigated-to result after the user clicks or presses Enter to navigate. The marker SHALL be rendered through the existing Cairo pipeline by adding a synthetic node with type `_search_selected` to `MapData::poiNodes`.

#### Scenario: Highlighted search result previews marker
- **GIVEN** the search overlay is open and results are displayed
- **WHEN** the user highlights a result with the mouse or arrow keys
- **THEN** a marker SHALL appear on the map at that result's location

#### Scenario: Navigated search result keeps marker after overlay closes
- **GIVEN** the user has highlighted a search result
- **WHEN** the user clicks the result or presses Enter
- **THEN** the map SHALL pan/zoom to the result
- **AND** the search overlay SHALL collapse
- **AND** the search marker SHALL remain visible at the result's location

### Requirement: Search-selection marker is cleared on new search query
When the user starts a new search query, the previous search-selection marker SHALL be cleared.

#### Scenario: New query clears old marker
- **GIVEN** a search-selection marker is visible from a previous search
- **WHEN** the user types a new query in the search field
- **THEN** the old search-selection marker SHALL be removed
- **AND** the new result list SHALL be displayed without a selected marker until a result is highlighted

### Requirement: Search-selection marker is cleared when search is cancelled without navigation
When the search overlay is closed without navigating to a result (e.g., by pressing Escape, clicking Cancel, clicking outside, or clicking the close button), the search-selection marker SHALL be cleared.

#### Scenario: Escape cancels search and clears marker
- **GIVEN** the search overlay is open and a result is highlighted
- **WHEN** the user presses Escape
- **THEN** the overlay SHALL collapse
- **AND** the search-selection marker SHALL be removed

### Requirement: Search-selection marker is styled via stylesheet
The `_search_selected` type SHALL be styled in `stylesheets/include/route.oss`. The marker SHALL use a highly visible color and size and SHALL appear at detail zoom levels or higher.

#### Scenario: Search marker visible at detail zoom
- **GIVEN** a search-selection marker is within the viewport
- **WHEN** the map is rendered at detail zoom or higher
- **THEN** the search-selection marker SHALL be visible

#### Scenario: Search marker hidden at low zoom
- **GIVEN** a search-selection marker is within the viewport
- **WHEN** the map is rendered below detail zoom
- **THEN** the search-selection marker SHALL NOT be visible

### Requirement: Search-selection marker renders above typical POI icons
The `_search_selected` NODE style SHALL have a higher numeric priority than typical POI icons so that it is not displaced by nearby bus stops, railway stations, or amenities.

#### Scenario: Search marker near a bus stop
- **GIVEN** a search-selection marker is close to a bus stop icon
- **WHEN** the map is rendered
- **THEN** the search-selection marker SHALL remain visible
