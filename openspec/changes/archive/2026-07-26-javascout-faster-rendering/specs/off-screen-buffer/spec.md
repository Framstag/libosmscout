## ADDED Requirements

### Requirement: Off-screen buffer for flicker-free rendering
The system SHALL maintain a persistent off-screen image buffer for map rendering. All Cairo rendering SHALL target this buffer. On render completion, the buffer SHALL be swapped to the front (display) buffer and blitted to the JavaFX Canvas in a single `Platform.runLater` call.

#### Scenario: First render populates off-screen buffer
- **WHEN** the first render request completes after database load
- **THEN** the off-screen buffer SHALL contain the rendered pixel data
- **AND** the buffer SHALL be blitted to the Canvas
- **AND** no flicker SHALL be visible (no intermediate clear/repaint)

#### Scenario: Subsequent render swaps buffer
- **WHEN** a new render completes while a previous frame is displayed
- **THEN** the new buffer SHALL replace the old buffer atomically
- **AND** the old buffer SHALL be retained for potential reuse (double buffering)

#### Scenario: Buffer dimensions match rendered area
- **WHEN** a render is triggered with overrun dimensions (2.5× screen)
- **THEN** the off-screen buffer SHALL be sized to the overrun dimensions
- **AND** the buffer SHALL be re-allocated only when dimensions change

#### Scenario: Buffer discarded on zoom or rotation
- **WHEN** zoom level or rotation angle changes
- **THEN** the off-screen buffer SHALL be invalidated
- **AND** a full re-render SHALL be triggered

#### Scenario: Buffer resized on window resize
- **WHEN** the window is resized (larger or smaller)
- **THEN** the Canvas dimensions SHALL be updated to match the parent panel
- **AND** the canvas width/height properties SHALL be unbound before setting (layout system may bind them)
- **AND** the parent panel SHALL have its minimum size set to 0 to allow shrinking below canvas size
- **AND** the panel SHALL have a clip rectangle bound to its dimensions to prevent canvas overflow
- **AND** a full re-render SHALL be triggered at the new canvas size
- **AND** the status bar SHALL remain visible at all window sizes

### Requirement: Double buffering with atomic swap
The system SHALL use two buffers: a back buffer (currently being rendered into) and a front buffer (currently displayed). On render completion, the buffers SHALL be swapped atomically under a lock.

#### Scenario: Front buffer always contains a complete frame
- **WHEN** the UI thread requests a paint
- **THEN** the front buffer SHALL always be available (may be stale but never null after first render)
- **AND** the UI thread SHALL never block on render-in-progress

#### Scenario: Swap does not tear
- **WHEN** a render completes and buffers are swapped
- **THEN** the swap SHALL be protected by a lock or atomic reference
- **AND** the UI thread SHALL read the front buffer reference under the same lock
