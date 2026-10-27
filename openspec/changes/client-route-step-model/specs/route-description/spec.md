# Spec Delta

## ADDED Requirements

### Requirement: The route description list renders from structured steps

The JavaScout route description list SHALL render one row per published route step, taken from the route
result's structured steps, so that every manoeuvre the route description holds appears as a row. All existing
rows SHALL keep their content, order and per-row distance and time.

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
