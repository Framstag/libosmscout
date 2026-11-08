## Purpose

A language-agnostic, complete model of a route's manoeuvres, published by the client library, so that a
client renders and manages a route from data instead of reconstructing it from formatted text.

## ADDED Requirements

### Requirement: A route step carries every description its route node holds

A step SHALL carry all descriptions the route description attaches to its node, together and in one record,
rather than a curated subset. A description that the route description does not carry for that node SHALL be
absent from the step, and SHALL NOT be represented by an empty or default-valued placeholder.

#### Scenario: A motorway exit step carries the exit and the sign destination

- **GIVEN** a route node that carries a motorway-leave description, a motorway-junction description with both
  a junction name and an exit reference, and a destination description
- **WHEN** the step for that node is published
- **THEN** the step SHALL carry the exit reference, the junction name and the destination text
- **AND** each of them SHALL be readable without inspecting the step's prose

#### Scenario: Several descriptions of one node arrive together

- **GIVEN** a route node that carries a turn, a direction with turn and curve angle, a crossing-ways
  description and a way name
- **WHEN** the step for that node is published
- **THEN** one step SHALL carry all of them

#### Scenario: An absent description is absent

- **GIVEN** a route node that carries no max-speed description
- **WHEN** the step for that node is published
- **THEN** the step SHALL report no max speed
- **AND** it SHALL NOT report a speed of zero as if one were known

### Requirement: A route step identifies its route node

A step SHALL carry the identity of the route node it describes: the database it belongs to, the offset of the
object it travels along, and the index of that object's node. The identity SHALL be sufficient for a client to
name the node again when it asks the navigation engine for the next instruction or reports a position.

#### Scenario: A client can address the node behind a step

- **GIVEN** a published step
- **WHEN** a client reads the step's node identity
- **THEN** it SHALL obtain the database, the object offset and the node index of that node
- **AND** two steps of the same route at different nodes SHALL have different identities

#### Scenario: Steps of a route spanning two databases are distinguishable

- **GIVEN** a route whose nodes belong to two databases
- **WHEN** the steps are published
- **THEN** each step's identity SHALL name its own database

### Requirement: A route step carries no translated or formatted text

A step SHALL carry values as they exist in the map data, and SHALL NOT carry sentences, translated words,
units or number formatting. Every label a user reads SHALL be produced by the client that displays it.

#### Scenario: The model carries data, not a sentence

- **GIVEN** a manoeuvre onto a named road of a known way type
- **WHEN** the step is published
- **THEN** the road's name, its reference and its way type SHALL be separate values
- **AND** no composed instruction sentence SHALL be part of the step

#### Scenario: Two clients present the same step in different languages

- **GIVEN** the same published step
- **WHEN** two clients render it in different languages and unit systems
- **THEN** both SHALL render from the same values
- **AND** neither SHALL need a value that is missing from the step because it was already formatted

### Requirement: A route publishes the references its steps point into

A route's published steps SHALL be accompanied by the mapping from database identity to map directory, and a
step SHALL carry the objects that intersect its node, each with its object type and its offset.

#### Scenario: A database identity resolves to a map directory

- **GIVEN** a route published from two databases
- **WHEN** a client resolves a step's database identity through the published mapping
- **THEN** it SHALL obtain the map directory that step belongs to

#### Scenario: Intersecting objects of a node are listed

- **GIVEN** a route node where other ways cross the route
- **WHEN** the step for that node is published
- **THEN** the step SHALL list those objects with their type and offset
- **AND** a node where nothing crosses SHALL list no objects

### Requirement: A route description converts into one step per manoeuvre

Conversion of a route description SHALL produce one step per route node that carries at least one
description, in route order, and SHALL produce no step for a node that carries none. A step's leg values SHALL
be the difference to the preceding step, and the first step's leg SHALL be the values of its own node.

#### Scenario: Nodes without descriptions produce no step

- **GIVEN** a route description whose intermediate nodes carry no description
- **WHEN** the description is converted
- **THEN** only the described nodes SHALL produce a step
- **AND** the steps SHALL remain in route order

#### Scenario: Leg values belong to the step that ends a leg

- **GIVEN** two consecutive steps whose nodes are 3.4 km and 120 s apart
- **WHEN** the steps are published
- **THEN** the later step's leg distance SHALL be 3.4 km and its leg time 120 s
- **AND** the sum of all leg values SHALL equal the route's total distance and duration

#### Scenario: A description kind that no conversion handles is reported

- **GIVEN** a route description carrying a description kind that the conversion does not handle
- **WHEN** the route is converted
- **THEN** the conversion SHALL report the unhandled kind instead of silently omitting it

### Requirement: Live guidance extends the same step with progress

The step produced for the manoeuvre ahead SHALL be the same record as a published step, extended with the
progress facts of the current position: the remaining distance and time to the manoeuvre, and the following
manoeuvre when it is close enough to be announced. Published step list entries SHALL carry absolute distance
and time from the route start, and SHALL NOT carry remaining values.

#### Scenario: Remaining values shrink as the position advances

- **GIVEN** a route with a manoeuvre ahead and two positions on the segment before it, the second further along
- **WHEN** the step ahead is published for each position
- **THEN** the later step SHALL carry the smaller remaining distance and the smaller remaining time
- **AND** neither remaining value SHALL be negative

#### Scenario: List entries carry absolute values

- **GIVEN** the full list of a route's steps
- **WHEN** a client reads a step's distance
- **THEN** it SHALL be that node's distance from the route start
- **AND** it SHALL NOT be a remaining distance

#### Scenario: The arrival is a step

- **GIVEN** a position beyond the last manoeuvre but before the destination
- **WHEN** the step ahead is published
- **THEN** it SHALL describe the arrival
- **AND** it SHALL carry the destination's distance from the route start

### Requirement: The new model does not shadow the Qt client's route step

The model's type names SHALL NOT be identical to the names the Qt client library already exports for its route
step, so a translation unit that uses both SHALL have to name them explicitly and SHALL NOT silently bind to
the wrong one. The Qt client library's existing route step type SHALL keep its names, its members and its
behaviour.

#### Scenario: Both types in one translation unit compile only with qualification

- **GIVEN** a translation unit that includes both the new model and the Qt client library's route step header
- **WHEN** it refers to a route step without qualification
- **THEN** it SHALL fail to compile, because the name is ambiguous or not found unqualified
- **AND** it SHALL compile when either type is named explicitly

#### Scenario: Existing Qt behaviour is unchanged

- **GIVEN** a Qt client built against the unchanged Qt client library
- **WHEN** it calculates and describes a route
- **THEN** its route steps SHALL carry the same values as before this change
