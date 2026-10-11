## ADDED Requirements

### Requirement: The per-ring work of a painter follows the rings it keeps

A painter that prepares the areas of a frame SHALL decide whether a ring is visible before it performs
per-ring geometry work for that ring, whatever its preparation looks like, and SHALL NOT perform that
work for a ring its decision discards. The per-call heap allocation of its area step SHALL NOT grow
with the number of loaded areas.

#### Scenario: A ring the decision discards costs no per-ring geometry work

- **GIVEN** a painter that prepares the rings of a frame itself, and a view that loads an area whose rings all lie outside the view's visible area
- **WHEN** the frame is prepared
- **THEN** the frame's prepared geometry SHALL contain none of those rings
- **AND** the number of rings for which the step performs per-ring geometry work SHALL equal the number of rings the decision keeps

#### Scenario: The allocation of the step does not grow with the loaded areas

- **GIVEN** a view whose kept rings are fixed and whose loaded areas outside the visible area are increased
- **WHEN** the frame is prepared
- **THEN** the heap allocation of the area step SHALL NOT grow with the added loaded areas

#### Scenario: Rings the decision keeps are prepared as before

- **GIVEN** a view of a stylesheet whose areas are styled and visible
- **WHEN** the frame is rendered by the same build before and after this change, with the same draw parameters
- **THEN** the rendered output strictly inside the view SHALL be identical
