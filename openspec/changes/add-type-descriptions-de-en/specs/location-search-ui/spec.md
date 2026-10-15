# Spec Delta

## MODIFIED Requirements

### Requirement: Result list display

Search results SHALL be displayed in a list below the search text field.

#### Scenario: Results shown sorted

- **WHEN** search results are returned
- **THEN** they SHALL be displayed in the result list sorted by relevance (highest first)
- **AND** each entry SHALL show the location label and, in place of the technical type name, the
  description of the object type in the language the application displays

#### Scenario: An entry without a type description falls back

- **WHEN** a result is displayed for an object whose type carries no description in the language the
  application displays
- **THEN** the entry SHALL show the technical type name in place of the description
- **AND** the entry SHALL be displayed without an error

#### Scenario: Full-width on small screens

- **WHEN** the window width is less than 600px
- **THEN** the result list SHALL span the full width of the window

#### Scenario: Popup on large screens

- **WHEN** the window width is 600px or more
- **THEN** the result list SHALL appear as a popup/dropdown anchored to the search bar
- **AND** SHALL NOT span the full window width
