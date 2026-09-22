## MODIFIED Requirements

### Requirement: The early rejection is conservative

The early rejection SHALL NOT discard an area that the painter's per-ring visibility decision
would have kept. The tolerance the early decision is allowed to extend an area by SHALL cover
the largest tolerance any per-ring decision can use for the loaded stylesheet and the current
zoom level, in the unit the decisions apply it.

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

### Requirement: Prepared entries, clipping geometry, orders and rendered output are unchanged

Within one build, for any view, the painter's early rejection SHALL prepare the same area entries
in the same preparation order, SHALL keep the same clipping geometry for the prepared entries,
SHALL prepare the same ways, and SHALL produce the same rendered output as that build's preparation
without the early rejection. The value of a visibility tolerance is not part of this comparison: it
is a screen-space length of the frame, and a change to it changes which borderline geometry is
prepared, intentionally and on both sides of the comparison.

#### Scenario: The prepared area entries are unchanged

- **GIVEN** a view whose prepared area entries are known from a preparation of the same build without the early rejection
- **WHEN** that view is prepared with the early rejection in place
- **THEN** the number of prepared area entries SHALL be identical to the known count
- **AND** each entry's type, role and transformed coordinates SHALL be identical

#### Scenario: Clipping geometry is unchanged

- **GIVEN** a view that prepares at least one entry whose inner rings are clipping regions
- **WHEN** the frame is prepared with the early rejection in place
- **THEN** every prepared entry SHALL reference the same valid clipping ranges as a preparation of the same build without the early rejection
- **AND** the rendered output SHALL show the same clipping

#### Scenario: Prepared ways and draw order are unchanged

- **GIVEN** a view whose prepared ways and prepared area draw order are known from a preparation of the same build without the early rejection
- **WHEN** that view is prepared with the early rejection in place
- **THEN** the number of prepared ways SHALL be identical to the known count
- **AND** the enumeration of the prepared areas in draw order SHALL be identical, including for areas whose ordering criteria compare equal

#### Scenario: Rendered output is unchanged

- **GIVEN** a view rendered with the early rejection in place, with a given stylesheet and render parameters
- **WHEN** the same view is rendered from the same build without the early rejection, with the same stylesheet and parameters
- **THEN** the rendered output SHALL be identical

## ADDED Requirements

### Requirement: A tolerance derived from a style-sheet width is a screen-space length

A visibility tolerance the painter derives from a width that a stylesheet declares SHALL be
converted from millimetres to the frame's pixels before it is applied, both in the per-ring
visibility decision and in the early rejection. The tolerance SHALL therefore grow with the DPI
of the frame, as the geometry the decision has to cover does.

#### Scenario: The same stylesheet yields a larger tolerance at a higher DPI

- **GIVEN** two frames of the same view that differ only in the DPI of their projection
- **WHEN** both frames are prepared with the same stylesheet
- **THEN** the tolerance applied in the higher-DPI frame SHALL be the tolerance of the lower-DPI frame scaled by the ratio of their DPIs

#### Scenario: An area within the converted border tolerance is not rejected

- **GIVEN** a stylesheet whose area border is `w` millimetres wide and a frame whose projection converts `w` to more than one pixel
- **WHEN** an area is placed outside the view such that its bounding box reaches the view by less than the converted half-width of that border
- **THEN** that area SHALL reach per-ring preparation
- **AND** an area whose bounding box reaches the view by more than the converted half-width SHALL NOT reach per-ring preparation
