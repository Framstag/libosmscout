# search-context-region

## ADDED Requirements

### Requirement: Search scoped to current map region

The JavaScout search API must accept a default admin region and pass it to the location search parameter, matching OSMScout2's `SetDefaultAdminRegion`. The default region is derived from the current map view.

#### Scenario: Search prefers results in current region

Given the map is centered on a known admin region
When the user searches for a common name
Then results within the current admin region rank higher than results elsewhere

#### Scenario: Search without region context

Given no default admin region is available
When the user searches
Then the search still returns results across the whole database
