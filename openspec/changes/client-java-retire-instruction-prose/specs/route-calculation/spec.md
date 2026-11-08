# Spec Delta

## REMOVED Requirements

### Requirement: RouteEntry Java data class

## ADDED Requirements

### Requirement: RouteEntry carries the structured steps

A Java class `RouteEntry` SHALL carry `double[] latitudes`, `double[] longitudes`, `double distance`,
`double duration`, `RouteStep[] steps` and the route's database-to-map-directory mapping. It SHALL NOT carry
prose description lines or per-instruction arrays; the manoeuvres are the structured steps. `distance` SHALL
be the last step's cumulative distance when a step list exists, else the published geometry's length.

#### Scenario: RouteEntry holds route geometry
- **WHEN** a route is calculated
- **THEN** `latitudes` and `longitudes` arrays contain the route waypoints in order from start to destination

#### Scenario: RouteEntry holds route metadata
- **WHEN** a route is calculated
- **THEN** `distance` contains the total route length in meters and `duration` contains estimated travel time in seconds

#### Scenario: A calculated route publishes one step per described node
- **WHEN** a route is calculated with a description
- **THEN** `steps` SHALL carry one entry per described route node, in route order
- **AND** each entry SHALL carry that node's manoeuvre data without reading any prose line

#### Scenario: The steps carry the route's database mapping
- **WHEN** a route is calculated from maps of more than one region
- **THEN** each step naming a database SHALL be resolvable to a map directory through the published mapping
