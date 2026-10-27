# Spec Delta

## Purpose

Describe the location search API exposed by `OSMScoutClient` for querying map databases by free-text string.

## ADDED Requirements

### Requirement: Location search entry points carry the request's generation

Every location search entry point that a caller can cancel SHALL carry the request's generation: the
admin-region-handle form, the region-name form and the form search. The convenience overloads SHALL begin
the place slot's request, run the search with it and end it. The parameterless `cancelSearch()` and the
boolean-cancel overload SHALL be gone: a caller cancels with `cancelSearch(generation)`.

#### Scenario: A scoped search carries the generation

- **GIVEN** a caller that began a place request and resolved an admin region
- **WHEN** it calls the generation-carrying search
- **THEN** every abort check of the search SHALL ask that generation
- **AND** cancelling it SHALL stop the search

#### Scenario: The convenience overload manages its own request

- **GIVEN** a caller that uses the two-argument overload
- **WHEN** it searches twice in a row
- **THEN** each call SHALL begin and end the place slot's request itself
- **AND** the second call SHALL not be blocked by the first

#### Scenario: A cancelled search returns an empty array

- **GIVEN** a search whose generation is cancelled mid-run
- **WHEN** the entry point returns
- **THEN** it SHALL return an empty array, not a partial result list

#### Scenario: The diagnostics line names the request

- **GIVEN** a search run with a generation
- **WHEN** the search reports its diagnostics
- **THEN** the line SHALL carry the slot, the generation, the abort state and the duration
