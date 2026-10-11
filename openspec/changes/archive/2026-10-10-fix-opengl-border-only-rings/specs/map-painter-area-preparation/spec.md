## ADDED Requirements

### Requirement: A ring a style sheet draws by a border is kept

A painter that prepares the areas of a frame SHALL keep a ring whose loaded style sheet resolves a border
style and no fill style, and SHALL prepare that ring for the border its style sheet declares, instead of
dropping the ring because it has no fill style. A ring that resolves neither a fill style nor a border
style SHALL stay outside the frame.

#### Scenario: A ring a style sheet draws by a border only is kept

- **GIVEN** a view that loads an area whose outer ring lies inside the visible area and for which the loaded style sheet resolves a border style of nonzero width and no fill style
- **WHEN** the frame is prepared
- **THEN** the area step SHALL keep that ring and SHALL prepare its geometry for the border

#### Scenario: A ring a style sheet draws by nothing is still not prepared

- **GIVEN** a view that loads an area whose outer ring lies inside the visible area and for which the loaded style sheet resolves neither a fill style nor a border style
- **WHEN** the frame is prepared
- **THEN** the visibility decision SHALL NOT be applied to that ring
- **THEN** the area step SHALL NOT keep that ring
