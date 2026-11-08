# route-description

Turn-by-turn route description display for JavaScout.

## Purpose

Display a columnar turn-by-turn route description below the route parameter box after successful route calculation. Generated via `RouteDescriptionPostprocessor` (matching `Demos/Routing.cpp` pattern). Includes distance, time, street names, turn directions, and waypoint list.

## Requirements

### Requirement: Route description with turn-by-turn instructions

JavaScout SHALL display a turn-by-turn route description below the route parameter box after successful route calculation. The description SHALL be generated using `RouteDescriptionPostprocessor` (matching `Demos/Routing.cpp` pattern) and include: distance, estimated duration, street names, turn directions, and waypoint list. The separate Info toggle button is dropped — route info is part of the description.

#### Scenario: Description shows after route calculation
- **WHEN** a route is successfully calculated
- **THEN** a list of turn-by-turn instructions is displayed below the route parameter box
- **AND** each line is formatted as columnar text with monospace font: total distance | segment distance | total time | segment time | instruction text
- **AND** the first line shows the start instruction with distance 0.0km and time 0:00
- **AND** each subsequent line shows a turn instruction with accumulated distance/time and segment distance/time

#### Scenario: Description updates on recalculation
- **WHEN** a new route is calculated
- **THEN** the description list is replaced with the new route's instructions

#### Scenario: Description clears when route cleared
- **WHEN** the route is cleared
- **THEN** the description list is cleared

### Requirement: Description generation via RouteDescriptionPostprocessor

The route description SHALL be generated in C++ JNI code using `RouteDescriptionPostprocessor::GenerateDescription()` with a custom `Callback` that overrides `BeforeNode(const Node&)` to extract `GetDistance()` and `GetTime()` from each route node.

#### Scenario: Distance and time extracted from route nodes
- **WHEN** the description is generated
- **THEN** `BeforeNode` reads `node.GetDistance().AsMeter()/1000.0` for distance in km and `node.GetTime()` for duration
- **AND** these values are formatted as columns in the output

### Requirement: A per-step time below a minute is printed in seconds

A description line SHALL print its per-step time in hours and minutes when the time is at least a minute, in
seconds when the time is at least a second and below a minute, and SHALL print no time at all when the time
is below a second. The rule SHALL apply to every path that produces a description line, and the other columns
of the line SHALL be unchanged.

#### Scenario: A step of several minutes is printed in minutes

- **GIVEN** a description line whose per-step time is at least a minute and below an hour
- **WHEN** the line is produced
- **THEN** it SHALL carry the time in whole minutes

#### Scenario: A step of several hours is printed in hours and minutes

- **GIVEN** a description line whose per-step time is an hour or more
- **WHEN** the line is produced
- **THEN** it SHALL carry the time in whole hours followed by the remaining whole minutes

#### Scenario: A step below a minute is printed in seconds

- **GIVEN** a description line whose per-step time is at least a second and below a minute
- **WHEN** the line is produced
- **THEN** it SHALL carry the time in whole seconds
- **AND** it SHALL NOT report a time of zero minutes

#### Scenario: A step below a second carries no time

- **GIVEN** a description line whose per-step time is below a second
- **WHEN** the line is produced
- **THEN** it SHALL carry no time
- **AND** it SHALL NOT carry a zero-valued time

#### Scenario: Both description paths print the same times

- **GIVEN** the same route description produced through the calculate path and through the recalculation path
- **WHEN** the two line lists are compared
- **THEN** the time column of corresponding lines SHALL be equal

### Requirement: The route description list renders from structured steps

The JavaScout route description list SHALL render one row per published route step that describes a
manoeuvre, and one row for a route step that carries a point of interest, taken from the route
result's structured steps. A published step that carries only segment facts SHALL NOT produce a row.
All existing rows SHALL keep their content, order and per-row distance and time.

#### Scenario: A step with only segment facts is not a row

- **GIVEN** a calculated route whose nodes carry max-speed and lane descriptions along the way
- **WHEN** the route description list is shown
- **THEN** it SHALL NOT contain a row for those nodes
- **AND** the steps SHALL still publish those facts

#### Scenario: A point of interest on the route appears as a row

- **GIVEN** a calculated route with a point of interest on it
- **WHEN** the route description list is shown
- **THEN** it SHALL contain a row for that point of interest
- **AND** the row SHALL name the point of interest

#### Scenario: Every manoeuvre of a motorway route appears

- **GIVEN** a calculated route that enters, changes and leaves a motorway
- **WHEN** the route description list is shown
- **THEN** it SHALL contain a row for entering, for changing and for leaving the motorway
- **AND** the row of leaving the motorway SHALL show the junction's exit reference and its name when the data
  carries them

#### Scenario: Roundabout manoeuvres appear

- **GIVEN** a calculated route through a roundabout
- **WHEN** the route description list is shown
- **THEN** it SHALL contain a row for entering the roundabout and a row for taking an exit
- **AND** the exit row SHALL show the ordinal of the exit taken

#### Scenario: Rows keep their order and their leg values

- **GIVEN** a route whose steps are published in route order with their leg values
- **WHEN** the list is rendered
- **THEN** the rows SHALL appear in that order
- **AND** each row's distance and time SHALL be its step's leg values

#### Scenario: A route without steps falls back to the description lines

- **GIVEN** a route result that carries no structured steps
- **WHEN** the description list is shown
- **THEN** it SHALL show the description lines as before
- **AND** no error SHALL be raised

### Requirement: The application does not interpret description text

JavaScout SHALL NOT obtain routing facts — turn type, road type, road name, exit number, destination, distance
or time — by reading the content of a description line. It SHALL obtain them from the description line's
route step. A line SHALL NOT be classified, split or filtered by its text content.

#### Scenario: No fact is recovered from the line's characters

- **GIVEN** a description line whose text contains a turn word, a way type token, a bracketed distance and a
  header marker
- **WHEN** the list is rendered
- **THEN** the turn icon, the road name, the way type, the distance and the time SHALL come from the step
- **AND** no part of the line's text SHALL be inspected to obtain them

#### Scenario: A header line is not recognised by its text

- **GIVEN** a description list that carries a header line
- **WHEN** the list is rendered
- **THEN** the header SHALL NOT be told apart from a manoeuvre row by comparing its text to a literal
