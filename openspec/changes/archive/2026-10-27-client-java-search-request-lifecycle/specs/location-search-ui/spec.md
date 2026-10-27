# Spec Delta

## Purpose

Describe the search overlay UI in JavaScout for querying locations and navigating the map.

## ADDED Requirements

### Requirement: The search overlay cancels the request it started

The overlay's place search SHALL begin a request for the place slot, SHALL remember its generation and pass
it into the search, and the Cancel button SHALL cancel exactly that generation. A cancellation that arrives
after a newer query began SHALL therefore be a no-op instead of stopping the newer search, and the overlay
SHALL stay usable.

#### Scenario: Cancel stops the running search

- **GIVEN** a running place search started by the overlay
- **WHEN** the user presses Cancel
- **THEN** that request's generation SHALL be cancelled
- **AND** its search SHALL stop at its next abort check

#### Scenario: A late cancel does not stop the newer search

- **GIVEN** a query that superseded an earlier one
- **WHEN** the earlier request's generation is cancelled
- **THEN** the newer search SHALL keep running
- **AND** its results SHALL still be shown
