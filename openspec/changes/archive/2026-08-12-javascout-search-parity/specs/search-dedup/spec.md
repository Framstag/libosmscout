# search-dedup

## ADDED Requirements

### Requirement: Near-identical results are deduplicated

The JavaScout result list must deduplicate near-identical entries, matching OSMScout2's `equals` logic: same object type, less than 300 m apart, and more than 3000 m from the search center.

#### Scenario: Duplicate street entries collapse to one

Given two results with the same object type, less than 300 m apart, and far from the search center
When the results are sorted
Then only one of the two entries remains in the list

#### Scenario: Distinct nearby results are kept

Given two results with the same object type, less than 300 m apart, but close to the search center
When the results are sorted
Then both entries remain in the list
