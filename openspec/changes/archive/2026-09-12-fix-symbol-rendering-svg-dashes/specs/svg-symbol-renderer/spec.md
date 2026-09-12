## ADDED Requirements

### Requirement: Dashed borders emit a scaled dash pattern
The system SHALL render dashed borders on SVG symbols with a dash pattern scaled to the converted border width, and SHALL emit no dash pattern for solid borders.

#### Scenario: Dashed border emits scaled dash array
- **GIVEN** a `BorderStyle` with width `1.0` and dash values `{2.0, 2.0}`
- **WHEN** `SetBorder()` is applied with `screenMmInPixel = 2.0` before drawing a primitive
- **THEN** the resulting SVG element SHALL contain `stroke-width="2"` and a dash array whose values equal the dash values scaled by the converted width, i.e. `stroke-dasharray="4 4"`

#### Scenario: Solid border emits no dash array
- **GIVEN** a `BorderStyle` without dashes and a visible width
- **WHEN** `SetBorder()` is applied before drawing a primitive
- **THEN** the resulting SVG element SHALL NOT contain a `stroke-dasharray` attribute
