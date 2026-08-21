# poi-search-api Specification

## Purpose

Describe the POI search API exposed by `OSMScoutClient` for finding points of interest of given categories within a radius around a coordinate.

## Requirements

### Requirement: Predefined POI categories

The POI search API SHALL support a fixed set of categories, each mapped to a fixed set of OSM feature types. The mapping SHALL be hardcoded in code (shared between the client API and the JavaScout UI) in the current iteration.

#### Scenario: Hotels category
- **WHEN** user calls `searchPOIs("hotels", ...)`
- **THEN** results SHALL be limited to POIs of hotel-type OSM features (e.g. hotel, motel, hostel, guest house)

#### Scenario: Restaurants category
- **WHEN** user calls `searchPOIs("restaurants", ...)`
- **THEN** results SHALL be limited to POIs of restaurant-type OSM features (e.g. restaurant, fast food)

#### Scenario: Grocery store category
- **WHEN** user calls `searchPOIs("grocery", ...)`
- **THEN** results SHALL be limited to POIs of grocery-type OSM features (e.g. supermarket, convenience store, grocery store)

#### Scenario: Viewpoint category
- **WHEN** user calls `searchPOIs("viewpoint", ...)`
- **THEN** results SHALL be limited to POIs of `tourism_viewpoint` OSM feature type

#### Scenario: Museum category
- **WHEN** user calls `searchPOIs("museum", ...)`
- **THEN** results SHALL be limited to POIs of museum-type OSM features (e.g. museum, museum building)

#### Scenario: Fuel category
- **WHEN** user calls `searchPOIs("fuel", ...)`
- **THEN** results SHALL be limited to POIs of fuel station OSM features (e.g. `amenity_fuel`, `amenity_fuel_building`)

#### Scenario: Charging station category
- **WHEN** user calls `searchPOIs("charging_station", ...)`
- **THEN** results SHALL be limited to POIs of electric vehicle charging station OSM feature type `amenity_charging_station`

#### Scenario: ATM category
- **WHEN** user calls `searchPOIs("atm", ...)`
- **THEN** results SHALL be limited to POIs of `amenity_atm` OSM feature type

#### Scenario: Tourism category
- **WHEN** user calls `searchPOIs("tourism", ...)`
- **THEN** results SHALL be limited to POIs of general tourist-interest OSM feature types (e.g. attraction, artwork, aquarium, zoo, theme park, picnic site, viewpoint, museum, information)

#### Scenario: Parking category
- **WHEN** user calls `searchPOIs("parking", ...)`
- **THEN** results SHALL be limited to POIs of parking OSM feature types (e.g. `amenity_parking`, `amenity_bicycle_parking`)

#### Scenario: Police category
- **WHEN** user calls `searchPOIs("police", ...)` on a database imported with a stylesheet that defines `amenity_police`
- **THEN** results SHALL be limited to POIs of `amenity_police` OSM feature type

#### Scenario: Hospital category
- **WHEN** user calls `searchPOIs("hospital", ...)`
- **THEN** results SHALL be limited to POIs of hospital OSM feature types (e.g. `amenity_hospital`, `amenity_hospital_building`)

#### Scenario: Doctors category
- **WHEN** user calls `searchPOIs("doctors", ...)` on a database imported with a stylesheet that defines `amenity_doctors`
- **THEN** results SHALL be limited to POIs of `amenity_doctors` OSM feature type

#### Scenario: Public transport category
- **WHEN** user calls `searchPOIs("public_transport", ...)`
- **THEN** results SHALL be limited to POIs of public transport OSM feature types (e.g. railway station, halt, tram stop, bus station, public transport platform, subway entrance)

#### Scenario: Category mapping covers all supported categories
- **WHEN** the client exposes its category list
- **THEN** the list SHALL contain the categories: hotels, restaurants, grocery, viewpoint, museum, fuel, charging_station, atm, tourism, parking, police, hospital, doctors, public_transport
- **AND** each category SHALL map to at least one OSM feature type

#### Scenario: Search with unknown category
- **WHEN** user calls `searchPOIs("unknown-category", ...)`
- **THEN** result array SHALL be empty
- **AND** no error SHALL be raised

#### Scenario: Search in a category with no matching types in the database
- **WHEN** user calls `searchPOIs("police", ...)` on a database whose `TypeConfig` lacks `amenity_police` (e.g. imported before the type existed)
- **THEN** result array SHALL be empty
- **AND** no error SHALL be raised

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
