# search-cancellation Specification

## Purpose
Allow long-running location searches to be cancelled via a breaker, matching OSMScout2's `Breaker` support. A new query cancels the previously running search; an explicit cancel aborts the current one.

## Requirements

### Requirement: Search can be cancelled

The JavaScout search API must accept a breaker and pass it to the location search, matching OSMScout2's `Breaker` support. A cancelled search stops early and reports cancellation.

#### Scenario: Cancelled search stops early

Given a long-running search with a breaker
When the breaker is triggered
Then the search stops and returns partial or no results without blocking

#### Scenario: Search without breaker

Given a search without a breaker
When the user searches
Then the search runs to completion as before
