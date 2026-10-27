# Spec Delta

## MODIFIED Requirements

### Requirement: Long press shows candidate list

The system SHALL display a candidate picker dialog listing all reasonable objects found at the
long-pressed coordinate, formatted like search results and naming each candidate's object type by its
description.

#### Scenario: Multiple candidates at long-pressed location

- **GIVEN** the map is loaded with OSM data
- **WHEN** the user long-presses at a location with multiple candidate objects that have description data
- **THEN** the system SHALL show a candidate picker dialog
- **AND** the dialog SHALL list each candidate as a selectable entry

#### Scenario: Candidate entries use search result description format

- **GIVEN** a candidate picker dialog is shown
- **WHEN** the dialog renders the candidate entries
- **THEN** each entry SHALL use the same description format as entries in search results
- **AND** each entry SHALL show the object's name and, in place of the technical type name, the
  description of the object type in the language the application displays

#### Scenario: A candidate without a type description falls back

- **GIVEN** a candidate picker dialog is shown
- **WHEN** a candidate's object type carries no description in the language the application displays
- **THEN** the entry SHALL show the technical type name in place of the description
- **AND** the entry SHALL be displayed without an error

#### Scenario: Candidates ordered by ranking

- **GIVEN** a candidate picker dialog is shown with multiple candidates
- **WHEN** the dialog lists the candidates
- **THEN** the entries SHALL appear in ranking order (has description data, visible at current zoom, proximity to the pressed coordinate)
