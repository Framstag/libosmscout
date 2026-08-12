# search-free-text Specification

## Purpose
Free-text search over the text search index (MARISA) for POIs, locations, regions and other named objects, in addition to the structured location search. Results are merged with structured results, deduplicated by object reference, and truncated to the requested limit.

## Requirements

### Requirement: Free-text search over text index

The JavaScout search API must search the text search index (MARISA) for POIs, locations, regions and other objects, in addition to the structured location search. The search must use transliteration, matching OSMScout2's `TextSearchIndex::Search` usage.

#### Scenario: Search finds POI by name

Given a database with a text search index containing a POI named "Café Central"
When the user searches for "cafe central"
Then the result list contains the POI "Café Central" with its coordinates

#### Scenario: Search falls back gracefully without text index

Given a database without a text search index
When the user searches for a location
Then the search still returns structured location results and logs a warning, without failing

#### Scenario: Search covers all object groups

Given a database with a text search index
When the user searches for a term
Then POIs, locations, regions and other objects are all searched, matching OSMScout2's `searchPOIs/searchLocations/searchRegions/searchOther` flags

### Requirement: Free-text results merged with structured results

Free-text results and structured location results must be merged into a single result list, deduplicated by object reference, and truncated to the requested limit.

#### Scenario: Same object found by both searches appears once

Given a database where an object matches both the structured search and the free-text search
When the user searches for the object's name
Then the object appears exactly once in the result list

#### Scenario: Result list respects limit

Given a search that would return more results than the requested limit
When the user searches with a limit of N
Then the result list contains at most N entries
