# Label Collision Stability

## Purpose

Defines the contract for stable label conflict resolution across layout rounds: a label that was visible in the previous round keeps its space first, so that the visibility of labels inside the visible region does not flip when the candidate set changes between renders.

## ADDED Requirements

### Requirement: Previous visibility precedence in collision resolution

The label layouter SHALL resolve collisions in two ordered groups: labels that were visible in the previous layout round are processed before all labels that were not visible in the previous round. Within each group the existing priority ordering applies.

#### Scenario: Previously visible label precedes new candidate

- **GIVEN** a layout ran once and a label L was visible
- **WHEN** a second layout is executed with the same visible content plus a new candidate N whose rectangle overlaps L's rectangle
- **THEN** L keeps its visibility and N is hidden (or partially resolved) independent of the priority values of L and N

#### Scenario: New candidates resolve between themselves by priority

- **GIVEN** a layout ran once with labels A and B overlapping (A visible, B hidden)
- **WHEN** a new label C overlapping B arrives while A stays registered
- **THEN** the relative order of A and B is unchanged
- **AND** C is resolved against the space already claimed by previously visible labels and by remaining newcomers by priority

#### Scenario: Previously visible labels survive rasterization jitter

- **GIVEN** two labels laid out in a session whose mask rectangles are adjacent (no overlap) in one round
- **WHEN** the same scene is laid out again with a fractional position shift under which integer truncation of the mask rectangles produces an overlap of at most 2 pixels
- **THEN** both labels stay visible in that round
- **AND** overlaps larger than the tolerance are still resolved by priority

### Requirement: Previous visibility state survives the per-draw reset

The layouter SHALL keep the previous-round visibility state when the layout instances are reset for the next draw, and SHALL refresh it to the visible set of the most recent layout.

#### Scenario: Reset keeps state, next round refreshes it

- **GIVEN** a label L was visible in the last layout
- **WHEN** the layout results are reset and a new layout is executed in which L is fully inside the viewport
- **THEN** L is treated as previously visible for the collision resolution of that new round

#### Scenario: Unregistered labels lose their state

- **GIVEN** a label L was visible in the last layout
- **WHEN** the next layout runs without L registered (it left the viewport or the render request)
- **THEN** L's previous visibility entry is not carried beyond that round
- **THEN** the visible set of that layout defines the state for the following round

### Requirement: Rendering uses complete tile data

The plane map renderer SHALL NOT exchange its finished render with a render produced from a partially loaded tile set while a previously finished render exists. Rendering SHALL be retried when more tile data has arrived (tile state change or update timer).

#### Scenario: Tile load churn does not replace the finished render

- **GIVEN** a finished render exists and a new render request is loading its tile data
- **WHEN** re-rendering is triggered while some requested tiles are still loading
- **THEN** the finished render stays in place
- **AND** rendering happens once the load job reports all tiles loaded

#### Scenario: First render works on partial data

- **GIVEN** no finished render exists yet
- **WHEN** the first render request is triggered while tiles are still loading
- **THEN** rendering proceeds with the available data (first image must appear quickly)

### Requirement: Deterministic and translation-invariant sticky resolution

The combined first-group/second-group resolution SHALL stay deterministic and translation invariant: the same label scene laid out twice in the same session yields identical results, and panning the scene does not change any visibility decision apart from positions.

#### Scenario: Session stability under pan

- **GIVEN** a session of two layout rounds over the same label set
- **WHEN** in the second round the viewport and all label points are shifted by a common offset
- **THEN** the visible label set and the relative positions are identical to the first round

#### Scenario: Repeated rounds are identical

- **GIVEN** a session with identical rounds
- **WHEN** the same round is executed repeatedly
- **THEN** every round reports the same visible set and positions

### Requirement: Stability under changed layout candidate set (pinned from label-layout-stability-tests)

In a dense label scene, the visibility of a label inside the visible region SHALL depend only on its own claim and the claims of labels that overlap it, not on label content that merely coexists in the same layout round. Adding further candidates must not flip the visibility of previously resolved, non-overlapping labels.

#### Scenario: Dense scene survives candidate growth

- **GIVEN** a dense 9x7 label grid laid out in a session, and a second round of the same session with additional border labels whose rectangles reach into the visible region
- **THEN** every label visible after the first round whose rectangle does not overlap any of the added border labels is still visible after the second round
- **AND** the diagnostic test of the change `label-layout-stability-tests` passes