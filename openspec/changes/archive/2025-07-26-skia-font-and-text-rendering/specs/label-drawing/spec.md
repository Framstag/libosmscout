## ADDED Requirements

### Requirement: Label drawing pipeline

The Skia backend SHALL implement `DrawLabels()` to drive the label layouter: call `Layout()`, `DrawLabels()` on the layouter, then `Reset()`.

#### Scenario: DrawLabels drives the layouter pipeline
- **WHEN** `DrawLabels()` is called
- **THEN** the backend SHALL call `labelLayouter.Layout()`, then `labelLayouter.DrawLabels()`, then `labelLayouter.Reset()`
