# Mixed Cap Style Handling

## ADDED Requirements

### Requirement: Render paths with mixed start/end cap styles

The system SHALL render paths where `startCap` and `endCap` have different values.

The system SHALL use the more restrictive cap (Butt > Square > Round) for the main path stroke.

The system SHALL draw round caps at path endpoints that specify `Round` cap when the main stroke uses a different cap.

#### Scenario: Round start, Butt end

- **WHEN** a path has `startCap=Round` and `endCap=Butt`
- **THEN** the start of the path SHALL have a rounded end
- **AND** the end of the path SHALL have a flat end

#### Scenario: Butt start, Round end

- **WHEN** a path has `startCap=Butt` and `endCap=Round`
- **THEN** the start of the path SHALL have a flat end
- **AND** the end of the path SHALL have a rounded end

#### Scenario: Round start, Square end

- **WHEN** a path has `startCap=Round` and `endCap=Square`
- **THEN** the start of the path SHALL have a rounded end
- **AND** the end of the path SHALL have a square end

#### Scenario: Uniform caps unchanged

- **WHEN** both caps are the same (`Round/Round`, `Butt/Butt`, `Square/Square`)
- **THEN** the rendering SHALL be identical to the current behavior

### Requirement: Round caps as filled circles

The system SHALL draw round caps as filled circles centered at the path endpoint with diameter equal to the stroke width.

The system SHALL use the same color as the path stroke for the round cap circles.

#### Scenario: Round cap circle dimensions

- **WHEN** a round cap is drawn at a path endpoint with stroke width `W`
- **THEN** the circle SHALL have radius `W / 2`
- **AND** the circle SHALL be centered at the path endpoint coordinates
