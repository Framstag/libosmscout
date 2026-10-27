# Spec Delta

## MODIFIED Requirements

### Requirement: Free-text results merged with structured results

Free-text results and structured location results must be merged into a single result list, deduplicated by object reference, and truncated to the requested limit. A free-text hit whose object no structured result of the same database reports SHALL be part of the result list; only a hit whose object a structured result already reports SHALL be dropped.

#### Scenario: Same object found by both searches appears once

Given a database where an object matches both the structured search and the free-text search
When the user searches for the object's name
Then the object appears exactly once in the result list

#### Scenario: A hit only the text index reports is returned

Given a database whose text index names an object the structured search does not resolve for the query
When the user searches for that name
Then the result list contains the object
And its entry reports that the text index matched it

#### Scenario: Result list respects limit

Given a search that would return more results than the requested limit
When the user searches with a limit of N
Then the result list contains at most N entries
