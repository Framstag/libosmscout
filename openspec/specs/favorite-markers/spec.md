## Purpose

Render all loaded favorite locations as markers on the JavaScout map so users can see where their saved places are.

## Requirements

### Requirement: JavaScout renders all loaded favorites as map markers
JavaScout SHALL render every loaded favorite location as a marker on the map. The marker coordinates SHALL be derived from `client.getFavoriteGroups()`. The markers SHALL be rendered through the existing Cairo map rendering pipeline by adding synthetic nodes with type `_favorite` to `MapData::poiNodes`.

#### Scenario: Favorites appear after loading
- **GIVEN** one or more favorite locations are loaded via `client.loadFavoriteLocations()`
- **WHEN** the map renders after favorites are loaded
- **THEN** a marker SHALL appear on the map at each favorite location

#### Scenario: Favorites are hidden when none exist
- **GIVEN** no favorite locations are loaded
- **WHEN** the map renders
- **THEN** no favorite markers SHALL be rendered

### Requirement: Favorite markers update when favorites change
When favorite locations are added, deleted, renamed, or saved through the favorites dialog, JavaScout SHALL refresh the favorite marker data and trigger a re-render.

#### Scenario: Adding a favorite shows a new marker
- **GIVEN** the favorites dialog is open
- **WHEN** a new favorite is added and saved
- **THEN** a new marker SHALL appear on the map at the favorite's location

#### Scenario: Deleting a favorite removes its marker
- **GIVEN** the favorites dialog is open with an existing favorite visible as a marker
- **WHEN** the favorite is deleted and saved
- **THEN** its marker SHALL disappear from the map on the next render

### Requirement: Favorite markers are styled via stylesheet
The `_favorite` type SHALL be styled in `stylesheets/include/route.oss`. The marker SHALL be visible only at detail zoom levels or higher to avoid clutter at low zoom.

#### Scenario: Favorite marker visible at detail zoom
- **GIVEN** a favorite marker is within the viewport
- **WHEN** the map is rendered at detail zoom or higher
- **THEN** the favorite marker SHALL be visible

#### Scenario: Favorite marker hidden at low zoom
- **GIVEN** a favorite marker is within the viewport
- **WHEN** the map is rendered below detail zoom
- **THEN** the favorite marker SHALL NOT be visible

### Requirement: Favorite markers render above typical POI icons
The `_favorite` NODE style SHALL have a higher numeric priority than typical POI icons so that favorite markers are not displaced by nearby bus stops, railway stations, or amenities.

#### Scenario: Favorite near a bus stop
- **GIVEN** a favorite marker is close to a bus stop icon
- **WHEN** the map is rendered
- **THEN** the favorite marker SHALL remain visible
