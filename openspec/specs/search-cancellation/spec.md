# search-cancellation Specification

## Purpose
Allow long-running location searches to be cancelled via a breaker, matching OSMScout2's `Breaker` support. A new query cancels the previously running search; an explicit cancel aborts the current one.

## Requirements

### Requirement: Search can be cancelled

A search SHALL be cancellable by a request that names it. A searcher SHALL begin a request for its surface's
slot, which returns the request's generation, SHALL pass that generation into the search, and SHALL cancel
that generation when it wants the search to stop. Beginning a request SHALL supersede the slot's previous
request, so a surface's searches replace each other while another surface's stay untouched. A cancellation
whose request is already gone — superseded, cancelled or ended — SHALL be a no-op. An aborted search SHALL
stop at its next abort check and report no results rather than a partial list.

#### Scenario: Cancelled search stops early

Given a long-running search whose generation is cancelled
When the search reaches its next abort check
Then the search stops and returns an empty result array without blocking

#### Scenario: Search without breaker

Given a caller that manages no request of its own and uses a convenience search entry point
When the user searches
Then the entry point begins and ends the place slot's request itself
And the search runs to completion as before

#### Scenario: A newer search of the same slot supersedes the running one

Given a running search of a slot
When a newer search of that same slot begins
Then the older request SHALL be aborted
And the newer one SHALL NOT be aborted by beginning

#### Scenario: A search of another slot is not disturbed

Given a running places search
When a car search begins
Then the places request SHALL stay live and SHALL NOT be aborted

#### Scenario: A late cancellation is harmless

Given a request that a newer request of its slot has already superseded
When that older generation is cancelled
Then the cancellation SHALL report that nothing was live
And the newer search SHALL keep running

### Requirement: The request lifecycle is a per-slot generation registry

The lifecycle SHALL be a dependency-free registry that a host test can exercise: one slot per search surface,
generations that are unique across slots, and one abort predicate — a request is aborted exactly when its
generation is no longer live. A live request SHALL leave the live set when a newer request of its slot
begins, when it is cancelled, or when it ends. Generation 0 SHALL mean "no request", and an unknown slot name
SHALL be a slot of its own rather than a shared one.

#### Scenario: Beginning returns a fresh generation

- **GIVEN** a registry
- **WHEN** a request is begun for a slot
- **THEN** the returned generation SHALL be non-zero and SHALL not have been returned before

#### Scenario: A request is aborted exactly when it is not live

- **GIVEN** a live request
- **WHEN** the abort predicate is asked for its generation
- **THEN** it SHALL report not aborted
- **AND** it SHALL report aborted once the request was superseded, cancelled or ended

#### Scenario: Cancelling reports whether the request was live

- **GIVEN** a live request and a request that was already superseded
- **WHEN** each generation is cancelled
- **THEN** the live one SHALL report that it was cancelled
- **AND** the superseded one SHALL report that nothing was live

#### Scenario: Ending a request makes a later cancellation a no-op

- **GIVEN** a search that ended its request
- **WHEN** its generation is cancelled afterwards
- **THEN** the cancellation SHALL report that nothing was live

#### Scenario: Slots are independent

- **GIVEN** requests begun for several slots
- **WHEN** one slot begins a newer request and another one is cancelled
- **THEN** only the requests of those slots SHALL be aborted
- **AND** the requests of the other slots SHALL stay live

#### Scenario: An unknown slot is a slot of its own

- **GIVEN** a registry
- **WHEN** two requests are begun under two different names
- **THEN** the first request SHALL stay live while the second is begun
- **AND** a further request under the first name SHALL supersede only the first
