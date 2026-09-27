## Purpose

Describe the location search API exposed by `OSMScoutClient` for querying map databases by free-text string.

## Requirements

### Requirement: Location search API on OSMScoutClient

`OSMScoutClient` SHALL expose a `searchLocations` method that queries the loaded database for locations matching a free-text string. Matching SHALL be separator-insensitive: a query's consecutive words SHALL match a consecutive run of the stored name's words in the same order, independently of whether either side separates those words with a space, hyphen, slash or dash, and independently of letter case and diacritic/sharp-s spelling. The rule SHALL apply to every named object the search covers — POIs, locations, administrative regions and postal areas. Only consecutive runs in query order SHALL match: a query whose words appear in the stored name reordered or interrupted by a stored-name word the query does not carry SHALL NOT match by this rule. Match quality SHALL reflect name coverage: a query covering the whole stored name SHALL be reported as a match, a query covering only part of it as a candidate. Matching SHALL remain additive: substring and word-prefix matches that the search returned before SHALL still be returned, unchanged and not reordered by this rule.

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

#### Scenario: Hyphen-joined name found by words spelled apart
- **GIVEN** a database whose index contains a POI named "Heinz-Hilpert-Theater Lünen"
- **WHEN** user calls `searchLocations("Hilpert Theater Lünen", 60)`
- **THEN** the result array SHALL contain that POI with its coordinates
- **AND** the result array SHALL NOT consist only of administrative-region entries

#### Scenario: Hyphen-joined location found by words spelled apart
- **GIVEN** a database whose index contains the location "August-Warkner-Platz"
- **WHEN** user calls `searchLocations("August Warkner Platz Eving", 60)`
- **THEN** the result array SHALL contain that location

#### Scenario: Words joined in the query instead of the name
- **GIVEN** a database whose index contains a location named "Am Birkenbaum"
- **WHEN** user calls `searchLocations("Am-Birkenbaum Dortmund", 60)`
- **THEN** the result array SHALL contain that location

#### Scenario: Transliteration tolerance across the separator
- **GIVEN** a database whose index contains a name joining a sharp-s or diacritic word with another word
- **WHEN** user calls `searchLocations` with the transliterated spelling of those words separated by a space
- **THEN** the result array SHALL contain that object

#### Scenario: Reordered or interrupted words do not match
- **GIVEN** a database whose index contains "Heinz-Hilpert-Theater Lünen"
- **WHEN** user calls `searchLocations` with "Theater Hilpert Lünen" or with a query missing the stored name's word "Theater"
- **THEN** that POI SHALL NOT be returned by the separator-insensitive rule

#### Scenario: Partial name coverage is a candidate
- **GIVEN** a database whose index contains "Heinz-Hilpert-Theater Lünen"
- **WHEN** user calls `searchLocations("Hilpert Theater Lünen", 60)`
- **THEN** the returned entry's match quality SHALL be candidate, not match

#### Scenario: Full name coverage is a match
- **GIVEN** a database whose index contains "Heinz-Hilpert-Theater Lünen"
- **WHEN** user calls `searchLocations("Heinz Hilpert Theater Lünen", 60)`
- **THEN** the returned entry's match quality SHALL be match

#### Scenario: Substring and prefix results unchanged
- **GIVEN** a query that matched entries by substring or word prefix before this rule existed
- **WHEN** user calls `searchLocations` with that query
- **THEN** the result array SHALL contain the same entries as before
- **AND** the separator-insensitive rule SHALL NOT remove or reorder them

### Requirement: LocationEntry data class

New Java class `com.framstag.libosmscout.client.LocationEntry` SHALL represent a single search result.

#### Scenario: LocationEntry structure
- **WHEN** a `LocationEntry` is returned from `searchLocations`
- **THEN** it SHALL expose fields: `label` (String), `type` (String), `objectType` (String), `lat` (double), `lon` (double), `region` (String[])

### Requirement: JNI bridge for location search

Native C++ code in `libosmscout-client-java/src/` SHALL bridge `LocationService::SearchForLocationByString()` and the form-based address search to the Java search methods. Both bridge paths SHALL use the same name matching, so a hyphen-joined name is found identically whether it is queried as a free-text string or as parsed address components (city, street, house number). Every name comparison on the search path SHALL use that matching: the query-string search, the admin-region resolution, the form search and the classification of a free-text text-index hit as a match or a candidate. The text-index classification SHALL be additive: it SHALL NOT change which entries the search returns, and it SHALL NOT turn a match into a candidate.

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

#### Scenario: Form search finds a hyphen-joined street
- **GIVEN** a database whose index contains the location "August-Warkner-Platz"
- **WHEN** Java `searchLocationByForm` is called with the city and the street spelled "August Warkner Platz"
- **THEN** the result array SHALL contain that location

#### Scenario: A free-text hit is classified with the same matching
- **GIVEN** a database whose text index returns a named object whose stored name joins its words with a separator
- **WHEN** user calls `searchLocations` with a query whose words spell that stored name completely apart
- **THEN** that entry's match quality SHALL be match, not candidate
- **AND** the entry SHALL be returned whether or not its match quality changed

#### Scenario: A free-text hit keeps its quality when the matching does not apply
- **GIVEN** a database whose text index returns a named object for a query that is only a prefix of the stored name
- **WHEN** user calls `searchLocations` with that query
- **THEN** that entry's match quality SHALL be candidate, as it was before
