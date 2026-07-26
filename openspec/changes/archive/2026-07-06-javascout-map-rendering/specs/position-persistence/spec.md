## ADDED Requirements
### Requirement: Persist last map position
The system SHALL save and restore the last map view position (latitude, longitude, magnification) across application restarts.

#### Scenario: Save position on view change
- **WHEN** user pans or zooms the map
- **THEN** after 500ms debounce, system writes `map.latitude`, `map.longitude`, `map.magnification` to `config.properties`
- **THEN** values are persisted to disk

#### Scenario: Restore position on startup
- **WHEN** application starts and database loads
- **THEN** system reads `map.latitude`, `map.longitude`, `map.magnification` from `config.properties`
- **THEN** map renders at the saved position
- **WHEN** config file has no map position keys
- **THEN** system uses default position: lat=51.5142273, lon=7.4652789, mag=5

#### Scenario: Config file format
- **WHEN** config file is written
- **THEN** it contains lines: `map.latitude=51.5142273`, `map.longitude=7.4652789`, `map.magnification=5`
- **THEN** existing config keys (`maps.directory`) are preserved

### Requirement: Config API
`Config` class SHALL support reading and writing map position properties.

#### Scenario: Read map position
- **WHEN** `config.getMapLatitude()` is called
- **THEN** returns latitude value from config or `null` if not set
- **WHEN** `config.getMapLongitude()` is called
- **THEN** returns longitude value from config or `null` if not set
- **WHEN** `config.getMapMagnification()` is called
- **THEN** returns magnification value from config or `null` if not set

#### Scenario: Write map position
- **WHEN** `config.setMapPosition(lat, lon, mag)` is called
- **THEN** config file is updated with new values
- **THEN** existing config keys are preserved
