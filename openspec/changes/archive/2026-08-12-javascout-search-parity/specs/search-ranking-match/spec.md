# search-ranking-match

## ADDED Requirements

### Requirement: Match quality boosts ranking

The JavaScout result ranking must include a match-quality component, matching OSMScout2's `matchRank`: an exact label match ranks highest, a prefix match ranks second, and fuzzy candidates rank lowest.

#### Scenario: Exact match ranks above prefix match

Given two results of the same type for the query "berlin"
When one result's label is exactly "berlin" and the other's label starts with "berlin"
Then the exact match is listed first

#### Scenario: Prefix match ranks above fuzzy match

Given two results of the same type for the query "berlin"
When one result's label starts with "berlin" and the other only contains "berlin"
Then the prefix match is listed first

## ADDED Requirements

### Requirement: Match quality preserved in result data

The match quality of each result (match/candidate) must be available to the ranking logic, matching OSMScout2's `LocationSearchResult::Entry` match quality fields.

#### Scenario: Candidate results sort after matches

Given results with mixed match quality
When the results are sorted
Then all "match" results are listed before any "candidate" results
