# search-ranking-coordinate Specification

## Purpose
Coordinate/GPS results rank above all object results, matching OSMScout2's `locationRank`. A query that parses as a lat/lon pair yields a coordinate result that is listed first.

## Requirements

### Requirement: Coordinate results rank first

Coordinate/GPS results must rank above all object results, matching OSMScout2's `locationRank` which returns rank 1 for coordinate type entries.

#### Scenario: Coordinate result listed first

Given a search that returns both a coordinate result and object results
When the results are sorted
Then the coordinate result is listed first
