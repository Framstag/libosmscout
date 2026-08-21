# Label Registration

## Purpose

The Skia backend implements `RegisterRegularLabel()` and `RegisterContourLabel()` to register labels with the `labelLayouter` for layout and overlap resolution.

## Requirements

### Requirement: Label registration

The Skia backend SHALL implement `RegisterRegularLabel()` and `RegisterContourLabel()` to register labels with the `labelLayouter` for layout and overlap resolution.

#### Scenario: Regular label is registered with layouter
- **WHEN** `RegisterRegularLabel()` is called with label data and position
- **THEN** the label SHALL be forwarded to `labelLayouter.RegisterLabel()`

#### Scenario: Contour label is registered with layouter
- **WHEN** `RegisterContourLabel()` is called with path label data and label path
- **THEN** the label SHALL be forwarded to `labelLayouter.RegisterContourLabel()`
