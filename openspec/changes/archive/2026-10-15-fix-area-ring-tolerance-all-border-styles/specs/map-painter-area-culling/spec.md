# Spec Delta

## ADDED Requirements

### Requirement: A ring's visibility tolerance covers every border style the ring resolves

The visibility decision of a ring SHALL extend the ring by a tolerance that covers every border style
the ring resolves rather than a single one of them: both the widest border the ring draws and the
reach of a border that is drawn at an offset from the ring. A ring whose drawn border reaches the
view SHALL NOT be rejected because the style the decision read is not the style that reaches it.

#### Scenario: A border drawn at an offset keeps its ring

- **GIVEN** a stylesheet that draws the border of a ring at an offset from the ring, and a frame whose projection resolves that offset and half of that border width to more than one pixel
- **WHEN** an area is placed outside the view such that its bounding box reaches the view by less than that offset
- **THEN** that ring SHALL be kept
- **AND** the frame SHALL contain the drawn border geometry of that ring

#### Scenario: The widest drawn border decides the tolerance

- **GIVEN** a ring that resolves several border styles of different widths, the widest of which is not the style the decision reads first
- **WHEN** that ring is placed outside the view such that its bounding box reaches the view by less than half of the widest of those widths
- **THEN** that ring SHALL be kept

#### Scenario: A ring whose drawn borders do not reach the view contributes nothing

- **GIVEN** a ring whose resolved borders are drawn at an offset, placed outside the view such that its bounding box reaches the view by more than that offset and half of its widest border width
- **WHEN** a frame is prepared
- **THEN** that ring SHALL contribute no prepared geometry to the frame

#### Scenario: A ring without an offset border keeps its tolerance

- **GIVEN** a stylesheet whose area border styles declare the same width and are drawn without an offset, and a view whose rings reach per-ring preparation as they do before this change
- **WHEN** that view is prepared after the change
- **THEN** the same rings SHALL reach per-ring preparation as before

## MODIFIED Requirements

### Requirement: The early rejection is conservative

The early rejection SHALL NOT discard an area that the painter's per-ring visibility decision
would have kept. The tolerance the early decision is allowed to extend an area by SHALL cover
the largest tolerance any per-ring decision can use for the loaded stylesheet and the current
zoom level, in the unit the decisions apply it, including the reach of a border style that is
drawn at an offset from its ring.

#### Scenario: No area that a per-ring decision would keep is rejected early

- **GIVEN** a view that is prepared with the early rejection in place and, for comparison, without it
- **WHEN** both preparations are performed on a stylesheet whose area borders are wider than the current zoom level resolves to less than one pixel
- **THEN** the set of areas that reach per-ring preparation SHALL NOT be smaller than the set of areas whose per-ring decision keeps at least one ring
- **THEN** no ring that is styled and visible without the early rejection SHALL be lost with it

#### Scenario: The tolerance grows with the stylesheet

- **GIVEN** two stylesheets that differ only in the border widths their area styles declare, so that the second one uses a larger per-ring tolerance
- **WHEN** the same view is prepared with the early rejection for both stylesheets
- **THEN** the early decision SHALL use the larger tolerance for the second stylesheet
- **AND** the prepared area entries SHALL be identical to those of the unculled pipeline in both cases

#### Scenario: The early tolerance follows the DPI of the frame

- **GIVEN** a view prepared with the early rejection and a stylesheet whose converted border half-width is more than one pixel
- **WHEN** the same view is prepared with projections that differ only in their DPI
- **THEN** the set of areas that reach per-ring preparation SHALL contain the areas within the larger converted tolerance of the higher-DPI preparation
- **AND** no area SHALL be rejected early in either preparation whose per-ring decision keeps a ring

#### Scenario: An area whose only reachable border is offset is not rejected early

- **GIVEN** an area placed outside the view such that its bounding box reaches the view by more than the converted half-width of the widest border the stylesheet declares, but by less than the reach of a border that stylesheet draws at an offset
- **WHEN** the frame is prepared with the early rejection in place
- **THEN** that area SHALL reach per-ring preparation
- **AND** an area whose bounding box lies outside the view by more than that offset reach too SHALL NOT reach per-ring preparation
