# Label Layout Stability Tests

## Purpose

Defines the executable contract for verifying label layout results without any rendering backend: the label layouter must be drivable with synthetic label data and produce verifiable visibility and position results, and equal label content laid out under different layout viewports must expose its stability properties.

## ADDED Requirements

### Requirement: Backend-independent label layout execution

The test suite SHALL be able to execute the label layouter on synthetic label data (labels with type, priority, text and fixed font metrics) without instantiating any rendering backend or drawing any pixels.

#### Scenario: Layout runs without a rendering backend

- **GIVEN** a set of synthetic text labels with registered priorities and a viewport of 800x600 pixels
- **WHEN** the label layout is executed
- **THEN** it returns a set of visible labels determined only by label data, priorities, and the viewport
- **AND** no rendering backend, graphics context, or font rendering subsystem was instantiated

#### Scenario: Layout result is queryable

- **WHEN** the label layout has been executed
- **THEN** the result reports for each label whether it is visible
- **AND** for each visible label its top-left pixel position and its measured width and height are available

### Requirement: Deterministic layout for identical inputs

The label layouter SHALL produce identical layout results when the same label set is laid out twice with the same viewport.

#### Scenario: Repeated layout produces identical result

- **GIVEN** a synthetic label set with at least two labels competing for the same space
- **WHEN** the layout is executed twice with the identical viewport
- **THEN** both runs report the same set of visible labels with the same positions

### Requirement: Stability of fully visible labels under panning

For a given label set, every label that is fully contained in the visible area in both of two panned viewports SHALL keep its visibility and its map-relative position in both layouts.

#### Scenario: Horizontal pan preserves visible labels

- **GIVEN** a synthetic label set with non-competing labels in the center region and a viewport of 800x600 pixels
- **WHEN** the layout is executed once with viewport origin (0,0) and once with viewport origin (50,0)
- **THEN** every label that is fully inside both viewports is reported visible in both runs
- **AND** each such label's position in the second run equals its position in the first run shifted by exactly (-50, 0)

#### Scenario: Vertical pan preserves visible labels

- **GIVEN** a synthetic label set with non-competing labels in the center region and a viewport of 800x600 pixels
- **WHEN** the layout is executed once with viewport origin (0,0) and once with viewport origin (0,50)
- **THEN** every label that is fully inside both viewports is reported visible in both runs
- **AND** each such label's position in the second run equals its position in the first run shifted by exactly (0, -50)

### Requirement: Stability under enlarged layout viewport

When the layout viewport is enlarged around an unchanged visible center, the layout of labels within the original visible viewport SHALL NOT change if the enlarged viewport introduces no map content other than empty space around the original viewport.

#### Scenario: Enlarged canvas preserves visible labels

- **GIVEN** a synthetic label set placed inside an 800x600 visible viewport with no labels outside that viewport
- **WHEN** the layout is executed with the 800x600 viewport and again with a 1200x900 viewport centered on the same central point
- **THEN** all labels fully inside the 800x600 viewport keep their visibility and their positions shifted by the equal central offset

### Requirement: Stability under changed layout candidate set

In a dense label scene, the visibility of a label inside the visible region SHALL depend only on its priority and the labels that overlap it, not on label content that is absent from the layout run. When the layout candidate set is extended without moving any already-present label, labels that were visible in the smaller candidate set and do not overlap any of the newly introduced labels SHALL stay visible.

#### Scenario: Dense scene with additionally available border labels

- **GIVEN** a dense label scene with labels covering an 800x600 visible viewport, and additional labels near the viewport border of a 1600x1200 viewport centered on the same central point, whose rectangles reach into the visible region and overlap labels of the dense scene
- **WHEN** the layout is executed with the 800x600 viewport over the dense scene alone and again with the 1600x1200 viewport over the extended candidate set
- **THEN** labels that are visible in the first run, are fully inside the 800x600 viewport, and do not overlap any of the additional labels keep their visibility and their positions apart from the central offset
- **AND** labels that additionally lose their visibility through the new candidates are reported, so that the number of labels whose visibility changed by the candidate set extension is observable

### Requirement: Stability of competing labels under viewport shift

For a given label set whose labels collide, the winner of a collision SHALL be determined by label priority and shall not flip as a function of the viewport origin.

#### Scenario: Collision winner independent of pan

- **GIVEN** a synthetic label set with two competing labels of distinct priorities, both fully inside two viewports 800x600 that are offset horizontally by 50 pixels
- **WHEN** the layout is executed for both viewports
- **THEN** the same label (the one with the higher priority) is reported visible in both runs
- **AND** the lower-priority label is reported hidden in both runs