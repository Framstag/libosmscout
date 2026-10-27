# Spec Delta

## MODIFIED Requirements

### Requirement: RouteEntry Java data class

A new Java class `RouteEntry` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with public fields: `double[] latitudes`, `double[] longitudes`, `double distance` (meters), `double duration` (seconds), `String[] descriptions` (turn-by-turn description lines), and the per-instruction arrays `double[] instructionLats`, `double[] instructionLons`, `double[] instructionDistances` (meters) and `double[] instructionTimes` (seconds). It SHALL additionally carry the route's manoeuvres as structured steps in `RouteStep[] steps`, each step carrying the manoeuvre's data as published by the client library's route step model, and the route's database-to-map-directory mapping.

#### Scenario: RouteEntry holds route geometry
- **WHEN** a route is calculated
- **THEN** `latitudes` and `longitudes` arrays contain the route waypoints in order from start to destination

#### Scenario: RouteEntry holds route metadata
- **WHEN** a route is calculated
- **THEN** `distance` contains the total route length in meters and `duration` contains estimated travel time in seconds

#### Scenario: RouteEntry holds turn-by-turn description
- **WHEN** a route is calculated
- **THEN** `descriptions` array contains columnar text lines with distance, time, and turn instructions

#### Scenario: Per-instruction positions are index-aligned with the instruction lines
- **WHEN** a route is calculated with a description
- **THEN** `instructionLats` and `instructionLons` SHALL carry the position of each instruction line's manoeuvre
- **AND** they SHALL be index-aligned with the instruction lines of `descriptions`, the description's header not being an instruction and not counted

#### Scenario: Per-step leg values are the legs of the route
- **WHEN** a route is calculated with a description
- **THEN** `instructionDistances` SHALL carry the leg that ends at each instruction line's manoeuvre, measured from the instruction preceding it
- **AND** `instructionTimes` SHALL carry that leg's own estimated travel time, as the description reports it
- **AND** the entries of `instructionDistances` SHALL add up to `distance`
- **AND** the entries of `instructionTimes` SHALL NOT be required to add up to `duration`: the description's per-leg times follow the road speeds of the route, while `duration` stays the length-derived estimate of `client-java-route-length` at one average speed

#### Scenario: The arrays are dropped as a set when they cannot be aligned
- **WHEN** the per-instruction values cannot be aligned one-to-one with the instruction lines
- **THEN** all four arrays SHALL be null
- **AND** `descriptions`, `distance` and `duration` SHALL be reported as before

#### Scenario: A calculated route publishes structured steps

- **WHEN** a route is calculated with a description
- **THEN** `steps` SHALL carry one entry per described route node, in route order
- **AND** each entry SHALL carry that node's manoeuvre data without the client having to read `descriptions`
- **AND** the count of `steps` SHALL NOT depend on the count of description lines, so that a mismatch in one SHALL NOT discard the other

#### Scenario: The steps carry the route's database mapping

- **WHEN** a route is calculated from maps of more than one region
- **THEN** each step naming a database SHALL be resolvable to a map directory through the published mapping
- **AND** the mapping SHALL be available even when the description lines carry no information about it

#### Scenario: Existing description lines are unchanged

- **WHEN** a route is calculated after this change
- **THEN** `descriptions` SHALL carry the same lines as before
- **AND** `distance`, `duration` and the four per-instruction arrays SHALL carry the same values as before

## ADDED Requirements

### Requirement: A step exposes the manoeuvre facts the description holds

A route's published steps SHALL expose the manoeuvre facts that the route description carries for their nodes, including the motorway junction's name and exit reference, the destination sign text, the way type and the maximum speed, so that a client SHALL NOT need to parse description lines to obtain them.

#### Scenario: A motorway exit is readable as data

- **GIVEN** a calculated route that leaves a motorway at a junction that carries a reference and a name
- **WHEN** a client reads the route's steps
- **THEN** the step for that manoeuvre SHALL carry the exit reference and the junction name
- **AND** it SHALL carry the sign destinations of that junction

#### Scenario: A motorway manoeuvre appears among the steps

- **GIVEN** a calculated route that enters, changes and leaves a motorway
- **WHEN** a client reads the route's steps
- **THEN** the route SHALL publish a step for each of those manoeuvres
- **AND** the description lines SHALL keep their existing content
