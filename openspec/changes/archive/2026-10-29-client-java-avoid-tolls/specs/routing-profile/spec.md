# Spec delta — client-java-avoid-tolls

## ADDED Requirements

### Requirement: Toll ways are recorded in the routing index

A way whose tags declare that using it costs a toll SHALL be recorded in the routing index in a way a
routing profile can see during the search, and the record SHALL be distinguishable from "this database
carries no toll information". The index file's layout SHALL NOT change: a database imported before this
capability reads back as carrying no toll information. The road grade SHALL keep its own value wherever it
is read, whether or not a toll is recorded.

#### Scenario: An importer records a toll way
- **WHEN** a way tagged as charging a toll is imported into a routing index
- **THEN** the object variant recorded for it carries the toll signal
- **AND** the grade recorded for it still selects the same speed it would without the signal

#### Scenario: A way without the tag carries no toll signal
- **WHEN** a way is imported whose tags do not declare a toll
- **THEN** its object variant carries no toll signal

#### Scenario: A database without the signal still loads and routes
- **GIVEN** a database imported before this capability existed
- **WHEN** it is opened and a route is calculated with and without `avoidTolls`
- **THEN** the database loads, the route is calculated, and both calculations produce the same route
- **AND** no error is reported

#### Scenario: The grade keeps its value next to the toll signal
- **WHEN** an object variant records both a toll and a grade
- **THEN** reading its grade yields the grade the importer recorded

### Requirement: avoidTolls prices a toll way higher

A route calculation whose profile requests `avoidTolls` SHALL price an object variant carrying the toll
signal higher than the same variant without the flag, so that a detour is chosen when one is reasonable,
and SHALL still calculate a route when the toll way is the only option.

#### Scenario: A toll way costs more with the flag
- **GIVEN** a database that records a way as charging a toll
- **WHEN** the same origin and destination are calculated with `avoidTolls` false and true
- **THEN** the toll way's contribution to the second calculation's cost is higher
- **AND** the second calculation still returns a route

#### Scenario: A toll way stays routable when it is the only option
- **GIVEN** an origin and a destination connected only by a way that charges a toll
- **WHEN** the calculation is made with `avoidTolls` true
- **THEN** a route is returned rather than a routing failure
