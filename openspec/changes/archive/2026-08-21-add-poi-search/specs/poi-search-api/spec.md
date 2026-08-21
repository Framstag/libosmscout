## Purpose

Describe the POI search API exposed by `OSMScoutClient` for finding points of interest of given categories within a radius around a coordinate.

## ADDED Requirements

### Requirement: POI search API on OSMScoutClient

`OSMScoutClient` SHALL expose a `searchPOIs` method that queries the loaded database for POIs of a given category within a search radius around a center coordinate.

#### Scenario: Search returns POIs of the category
- **WHEN** user calls `searchPOIs("hotels", 52.0, 8.0, 5000, 50)` on an open database containing hotels within 5 km of (52.0, 8.0)
- **THEN** result array SHALL contain only POIs of the "hotels" category
- **AND** each result SHALL lie within the requested radius of the center coordinate
- **AND** result array SHALL NOT exceed 50 entries

#### Scenario: Search with unknown category
- **WHEN** user calls `searchPOIs("unknown-category", 52.0, 8.0, 5000, 50)`
- **THEN** result array SHALL be empty
- **AND** no error SHALL be raised

#### Scenario: Search on uninitialized client
- **WHEN** user calls `searchPOIs("hotels", 52.0, 8.0, 5000, 50)` before `openDatabase()` or after `close()`
- **THEN** result array SHALL be empty

#### Scenario: Search with zero radius
- **WHEN** user calls `searchPOIs("hotels", 52.0, 8.0, 0, 50)`
- **THEN** result array SHALL be empty

### Requirement: POI result data class

New Java class `com.framstag.libosmscout.client.PoiEntry` SHALL represent a single POI search result.

#### Scenario: PoiEntry structure
- **WHEN** a `PoiEntry` is returned from `searchPOIs`
- **THEN** it SHALL expose fields: `label` (String), `objectType` (String), `lat` (double), `lon` (double), `distance` (double, meters from search center)

#### Scenario: PoiEntry fields populated
- **WHEN** user calls `searchPOIs("restaurants", 52.0, 8.0, 5000, 50)` on a database containing a restaurant
- **THEN** each returned `PoiEntry` SHALL have non-null `label` and `objectType`
- **AND** `distance` SHALL be the distance in meters from the search center

### Requirement: Predefined POI categories

The POI search API SHALL support a fixed set of categories, each mapped to a fixed set of OSM feature types. The mapping SHALL be hardcoded in the first iteration.

#### Scenario: Hotels category
- **WHEN** user calls `searchPOIs("hotels", ...)`
- **THEN** results SHALL be limited to POIs of hotel-type OSM features (e.g. hotel, motel, hostel, guest house)

#### Scenario: Restaurants category
- **WHEN** user calls `searchPOIs("restaurants", ...)`
- **THEN** results SHALL be limited to POIs of restaurant-type OSM features (e.g. restaurant, fast food)

#### Scenario: Grocery store category
- **WHEN** user calls `searchPOIs("grocery", ...)`
- **THEN** results SHALL be limited to POIs of grocery-type OSM features (e.g. supermarket, convenience store, grocery store)

### Requirement: JNI bridge for POI search

Native C++ code in `libosmscout-client-java/src/` SHALL bridge the POI search to the native POI service.

#### Scenario: Native search delegation
- **WHEN** Java `searchPOIs` is called
- **THEN** native code SHALL resolve the category to its OSM type set
- **AND** call the native POI search with the center coordinate, radius, and type set
- **AND** convert each found POI to a Java `PoiEntry` object
- **AND** return the array to Java

#### Scenario: Memory cleanup
- **WHEN** native code converts results to Java objects
- **THEN** all intermediate C++ objects SHALL be released before returning
- **AND** no native heap SHALL leak per call
