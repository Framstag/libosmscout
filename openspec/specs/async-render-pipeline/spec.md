# Async Render Pipeline

## Purpose

Move all Cairo rendering (JNI calls) to a background thread with debounced request queue. Every render completes and writes its result (no cancellation — prevents starvation). UI thread notified via `Platform.runLater`.

## Requirements

### Requirement: Background render thread with job queue
The system SHALL use a single dedicated background thread for all Cairo rendering via JNI. Render jobs SHALL be submitted via an `AtomicReference<RenderJob>`. The thread SHALL poll the reference and process one job at a time. A new request SHALL overwrite the previous pending job atomically — no cancellation mechanism is used, every render the thread starts completes and writes its result. This prevents starvation where no render ever finishes under rapid input.

#### Scenario: Render job submitted
- **WHEN** `enqueueRenderJob()` is called
- **THEN** a `RenderJob` SHALL be created with the current view parameters
- **AND** the job SHALL be atomically set as the pending job
- **AND** any previous pending job SHALL be overwritten

#### Scenario: Render thread processes jobs sequentially
- **WHEN** the render thread is idle
- **THEN** it SHALL wait on a lock until notified (no busy-wait)
- **WHEN** a job is available
- **THEN** the thread SHALL take the job and call `executeRender()`
- **AND** the thread SHALL process the job to completion (no cancellation)

#### Scenario: Render completion notifies UI thread
- **WHEN** a render job completes successfully
- **THEN** the render thread SHALL schedule a `Platform.runLater` callback
- **AND** the callback SHALL swap buffers and blit to Canvas
- **AND** the callback SHALL notify view change listeners

### Requirement: Debounced render requests
The system SHALL debounce render requests that require a full JNI render. Pan events SHALL use a 50ms debounce. Zoom and rotation events SHALL use a 200ms debounce. Sub-region blits (which do not require JNI) SHALL execute immediately on every input event.

#### Scenario: Rapid pan events coalesced
- **WHEN** the user drags the map (continuous pan events)
- **THEN** each event SHALL execute a sub-region blit immediately (no debounce)
- **AND** if a full render is needed, only the last position SHALL trigger a render after 50ms of inactivity

#### Scenario: Rapid zoom events coalesced
- **WHEN** the user scrolls the mouse wheel rapidly (multiple zoom events)
- **THEN** only the final zoom level SHALL trigger a render after 200ms of inactivity

#### Scenario: Zoom triggers immediate scaled placeholder
- **WHEN** the user zooms in or out
- **THEN** the system SHALL scale the current front buffer to the new zoom level
- **AND** the scaled buffer SHALL be displayed immediately
- **AND** a full render SHALL be triggered asynchronously
- **AND** the full render SHALL replace the placeholder when complete
- **AND** no visual pause SHALL occur during zoom

### Requirement: Render error handling
The system SHALL handle render errors gracefully without crashing the UI.

#### Scenario: Render throws exception
- **WHEN** the JNI render call throws an exception
- **THEN** the system SHALL retry once after a 100ms delay
- **AND** if the retry also fails, the error SHALL be logged
- **AND** the render thread SHALL continue processing the next job
- **AND** the front buffer SHALL remain unchanged (previous frame still displayed)

#### Scenario: Render returns null pixels
- **WHEN** `client.render()` returns null
- **THEN** the error SHALL be logged
- **AND** no buffer swap SHALL occur
- **AND** the previous frame SHALL remain displayed

#### Scenario: Window resize triggers re-render
- **WHEN** the window is resized (larger or smaller)
- **THEN** the Canvas dimensions SHALL be updated to match the parent panel
- **AND** the canvas width/height properties SHALL be unbound before setting (layout system may bind them)
- **AND** the parent panel SHALL have its minimum size set to 0 to allow shrinking below canvas size
- **AND** the panel SHALL have a clip rectangle bound to its dimensions to prevent canvas overflow
- **AND** a full re-render SHALL be triggered at the new canvas size
- **AND** the status bar SHALL remain visible at all window sizes
