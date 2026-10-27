# Spec Delta

## Purpose

JNI bridge to libosmscout routing engine for async route calculation between two coordinates.

## MODIFIED Requirements

### Requirement: RouteEntry Java data class

A new Java class `RouteEntry` SHALL be added to `libosmscout-client-java/java/com/framstag/libosmscout/client/` with public fields: `double[] latitudes`, `double[] longitudes`, `double distance` (meters), `double duration` (seconds), `String[] descriptions` (turn-by-turn description lines), and the per-instruction arrays `double[] instructionLats`, `double[] instructionLons`, `double[] instructionDistances` (meters) and `double[] instructionTimes` (seconds).

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
