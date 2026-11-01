# Spec Delta

## ADDED Requirements

### Requirement: The Java navigation listener publishes route steps

The Java navigation listener SHALL publish live guidance as the route step model: its callbacks SHALL carry
the same `RouteStep` record a calculated route publishes, in route order, and SHALL NOT carry an ad-hoc prose
instruction record. The step ahead SHALL carry the position's remaining distance and time, and the following
manoeuvre as a nested step when it is close enough to be announced.

#### Scenario: The listener receives steps, not prose

- **GIVEN** a navigation session on a route with manoeuvres
- **WHEN** the engine publishes the route's manoeuvres
- **THEN** the listener SHALL receive them as `RouteStep` records
- **AND** each of them SHALL be the record a calculated route publishes for that node

#### Scenario: The step ahead carries the progress values

- **GIVEN** a position on the segment before a manoeuvre
- **WHEN** the engine publishes the manoeuvre ahead
- **THEN** the listener SHALL receive one `RouteStep`
- **AND** it SHALL carry the remaining distance and time to that manoeuvre

#### Scenario: The following manoeuvre arrives as a nested step

- **GIVEN** a manoeuvre whose following manoeuvre is close enough to be announced
- **WHEN** the step ahead is published to the listener
- **THEN** it SHALL carry the following manoeuvre as a nested `RouteStep`

#### Scenario: A list the bridge cannot fully build is withheld

- **GIVEN** a manoeuvre list one of whose steps the bridge cannot build
- **WHEN** the list is published to the listener
- **THEN** the listener SHALL receive no list rather than one carrying an absent element
