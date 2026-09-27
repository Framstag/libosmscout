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
Alongside the label, the object type, the coordinate and the distance, a result SHALL expose the
operator and the brand of the found object when the object carries them.

#### Scenario: PoiEntry structure

- **WHEN** a `PoiEntry` is returned from `searchPOIs`
- **THEN** it SHALL expose fields: `label` (String), `operator` (String, empty when the found object
  carries no operator), `brand` (String, empty when the found object carries no brand),
  `objectType` (String), `lat` (double), `lon` (double), `distance` (double, meters from search
  center)

#### Scenario: PoiEntry fields populated

- **WHEN** user calls `searchPOIs("restaurants", 52.0, 8.0, 5000, 50)` on a database containing a restaurant
- **THEN** each returned `PoiEntry` SHALL have non-null `label` and `objectType`
- **AND** `operator` and `brand` SHALL be non-null, and empty when the found object carries no such
  information
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

### Requirement: POI search covers every loaded database

`searchPOIs` SHALL query every loaded non-basemap database whose area can hold the requested radius,
independently of how many results an earlier database already contributed. A database that alone
satisfied the requested result limit SHALL NOT prevent the remaining databases from being searched.
The requested limit SHALL apply to the merged result, not to an individual database.

#### Scenario: A hit that only a later database holds is found

- **GIVEN** two loaded databases whose areas both contain the search center
- **AND** only the database searched second contains a POI of the requested category within the radius
- **WHEN** `searchPOIs` is called for that category and center
- **THEN** the result SHALL contain that POI

#### Scenario: A database filling the limit does not hide nearer results

- **GIVEN** a loaded database that returns at least `limit` results on its own
- **AND** a second loaded database containing a POI nearer to the search center than the farthest
  entry the first database contributed
- **WHEN** `searchPOIs` is called with that `limit`
- **THEN** the result SHALL contain the nearer POI
- **AND** the result SHALL NOT contain more than `limit` entries

#### Scenario: The limit applies to the merged list

- **GIVEN** several loaded databases together containing more POIs of the category than `limit`
- **WHEN** `searchPOIs` is called with that `limit`
- **THEN** the result SHALL NOT contain more than `limit` entries

### Requirement: A POI present in several databases is returned once

A POI contained in more than one loaded database SHALL appear at most once in the result. The
surviving entry SHALL be the copy from the database whose area contains the search center; among
several such databases it SHALL be the copy of the database ordered first by the client.

#### Scenario: Duplicated POI collapses to one entry

- **GIVEN** two overlapping loaded databases that both contain the search center and both contain the
  same POI
- **WHEN** `searchPOIs` is called for the POI's category and that center
- **THEN** the result SHALL contain exactly one entry for that POI

#### Scenario: The copy from the database containing the search center survives

- **GIVEN** two overlapping loaded databases that both contain the same POI
- **AND** only one of them contains the search center
- **WHEN** `searchPOIs` is called for the POI's category and that center
- **THEN** the surviving entry SHALL be the copy of the database that contains the search center
- **AND** the resulting entry SHALL be the entry the same search returns with only that database
  loaded

### Requirement: POI search result order is deterministic

Results SHALL be ordered nearest first. Entries at the same distance SHALL be ordered by a criterion
that does not depend on the order in which the databases were discovered on disk, so that repeating
the same search over the same set of databases yields the same list.

#### Scenario: Repeated searches yield the same list

- **GIVEN** a set of loaded databases whose areas overlap around the search center
- **WHEN** the same `searchPOIs` call is issued twice
- **THEN** both calls SHALL return the same entries in the same order

#### Scenario: Same-distance entries have a stable order

- **GIVEN** two loaded databases each contributing a POI at the same distance from the search center
- **AND** the labels of the two POIs differ
- **WHEN** `searchPOIs` is called for their category and that center
- **THEN** the entry whose label sorts first SHALL be listed first
- **AND** the result order SHALL be the same for databases discovered in either order

### Requirement: POI results expose the operator and the brand of the found object

A POI search result SHALL expose the operator of the found object and the brand of the found object.
Each attribute SHALL be empty when the found object carries no such information; an empty attribute
SHALL NOT be an error and SHALL NOT be treated as a value. The two attributes SHALL be determined
independently of each other and independently of the display label, so that an object that has both
carries both, an object that has only one carries only that one, and an object that has neither is
still a valid result. Reading these attributes SHALL NOT change which objects a search returns, how
many it returns, or the order it returns them in.

#### Scenario: Operator and brand are carried by the result

- **GIVEN** a database region contains a POI that carries an operator and a brand in the map data
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL carry that operator
- **AND** the result SHALL carry that brand

#### Scenario: Only one of the two attributes has a value

- **GIVEN** a database region contains a POI that carries an operator but no brand
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL carry that operator
- **AND** the result SHALL report the brand as empty

#### Scenario: Neither attribute has a value

- **GIVEN** a database region contains a POI that carries neither an operator nor a brand
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL report both attributes as empty
- **AND** no error SHALL be raised

#### Scenario: The label fallback does not consume the operator

- **GIVEN** a POI whose display label is derived from its operator because the object carries no name
- **WHEN** a search returns that POI as a result
- **THEN** the result SHALL carry that operator in addition to the label

#### Scenario: The attributes do not influence which results are returned

- **GIVEN** a search over an area whose matching objects are a mix of objects that carry an operator
  or a brand and objects that carry neither
- **WHEN** the search is executed
- **THEN** every object that matches the category and the radius SHALL be returned regardless of
  whether it carries these attributes
- **AND** the returned results SHALL be ordered by distance ascending as before
