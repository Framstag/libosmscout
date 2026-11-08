# Spec Delta

## Purpose

Name the reason a route calculation produced no route, and resolve a position to a routable way for the end
of the route it is being resolved for.

## ADDED Requirements

### Requirement: A calculation that outruns the database open reports the not-yet-loaded kind

The bridge SHALL report `KIND_DATABASE_NOT_LOADED` when a route calculation finds no loaded database instance
because the open the caller issued has not been registered yet, distinct from `KIND_ROUTING_DATA_UNAVAILABLE`,
which a database that cannot be opened reports. Both route-calculation entry points SHALL report it the same
way.

#### Scenario: A calculation that outruns the database open reports that

- **GIVEN** a client that has just asked for a map database to be opened, and a calculation issued before the
  database list has been populated
- **WHEN** the calculation starts and finds no database instance
- **THEN** it SHALL report `KIND_DATABASE_NOT_LOADED`, not `KIND_ROUTING_DATA_UNAVAILABLE`
- **AND** the same calculation SHALL succeed when it is issued again after the list is populated
