## ADDED Requirements

### Requirement: Location search API on OSMScoutClient

`OSMScoutClient` SHALL expose a `searchLocations` method that queries the loaded database for locations matching a free-text string.

#### Scenario: Search returns sorted results
- **WHEN** user calls `searchLocations("Berlin", 50)` on an open database containing Berlin
- **THEN** result array SHALL contain entries sorted by relevance (exact match > prefix > partial, closer to search center ranked higher)
- **AND** result array SHALL NOT exceed 50 entries

#### Scenario: Search with empty query
- **WHEN** user calls `searchLocations("", 50)`
- **THEN** result array SHALL be empty

#### Scenario: Search on uninitialized client
- **WHEN** user calls `searchLocations("Berlin", 50)` before `openDatabase()` or after `close()`
- **THEN** result array SHALL be empty

#### Scenario: LocationEntry fields populated
- **WHEN** user calls `searchLocations("Dortmund", 10)` on a database containing Dortmund
- **THEN** each returned `LocationEntry` SHALL have non-null `label`, `type`, `lat`, `lon`
- **AND** `region` array SHALL contain the admin region hierarchy if available

### Requirement: LocationEntry data class

New Java class `com.framstag.libosmscout.client.LocationEntry` SHALL represent a single search result.

#### Scenario: LocationEntry structure
- **WHEN** a `LocationEntry` is returned from `searchLocations`
- **THEN** it SHALL expose fields: `label` (String), `type` (String), `objectType` (String), `lat` (double), `lon` (double), `region` (String[])

### Requirement: JNI bridge for location search

Native C++ code in `libosmscout-client-java/src/` SHALL bridge `LocationService::SearchForLocationByString()` to the Java `searchLocations` method.

#### Scenario: Native search delegation
- **WHEN** Java `searchLocations` is called
- **THEN** native code SHALL create a `LocationStringSearchParameter` with the query string and limit
- **AND** call `LocationService::SearchForLocationByString()` on the stored `DatabaseRef`
- **AND** convert each `LocationSearchResult::Entry` to a Java `LocationEntry` object
- **AND** return the array to Java

#### Scenario: Memory cleanup
- **WHEN** native code converts results to Java objects
- **THEN** all intermediate C++ objects SHALL be released before returning
- **AND** no native heap SHALL leak per call
