# Spec Delta

## Purpose

Turn-by-turn route description display for JavaScout.

## ADDED Requirements

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
