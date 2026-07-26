## ADDED Requirements

### Requirement: LRU tile cache for rendered map tiles
The system SHALL cache rendered map tiles in an LRU (Least Recently Used) map. Each tile SHALL be keyed by (zoom level, tile X, tile Y). After a full render completes, the result SHALL be split into tiles and stored in the cache. The tile cache SHALL NOT be used to compose full images for display — tiles are keyed by pixel coordinates only, not geographic position, so a composite would return stale content after a pan.

#### Scenario: Tiles stored after render
- **WHEN** a full render completes
- **THEN** the rendered pixel buffer SHALL be split into 256×256 tiles
- **AND** each tile SHALL be stored in the LRU cache with the current epoch

#### Scenario: Tile cache eviction on size limit
- **WHEN** the tile cache exceeds its configured maximum size (default 200)
- **THEN** the least recently used tile SHALL be evicted

#### Scenario: Tile cache invalidation on style change
- **WHEN** the stylesheet is reloaded
- **THEN** all cached tiles SHALL be invalidated
- **AND** the epoch counter SHALL be incremented

#### Scenario: Tile cache NOT used for display composition
- **WHEN** a render is requested after a pan
- **THEN** the system SHALL NOT compose a full image from cached tiles
- **AND** the system SHALL always trigger a full JNI render
- **AND** the rendered result SHALL be stored in the cache for future use
