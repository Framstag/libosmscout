## ADDED Requirements

### Requirement: Text rendering path logged in debug mode
When `--debug` is enabled, `ProcessNode()` SHALL log which branch each text style takes during text rendering. This SHALL distinguish between the three branches: scale/fade rendering, autoSize skip, and normal rendering.

#### Scenario: Debug output for scale/fade branch
- **WHEN** debug is enabled and a node text style enters the scale/fade branch (magnification > scaleAndFadeMag)
- **THEN** `log.Debug()` SHALL output text style details including label text, computed fontSize, alpha value, and magnification level

#### Scenario: Debug output for normal branch
- **WHEN** debug is enabled and a node text style uses normal rendering (no fading, no autoSize)
- **THEN** `log.Debug()` SHALL output text style details including label text and fontSize

#### Scenario: Debug output for autoSize skip
- **WHEN** debug is enabled and a node text style has autoSize=true and is skipped
- **THEN** `log.Debug()` SHALL output a message containing "autoSize" and the label text, noting this is a known limitation

### Requirement: TextLoader glyph count logged
After `TextLoader::AddCharactersToTextureAtlas()` processes all characters for a label, the number of glyphs loaded SHALL be logged at `log.Debug()` level when debug is enabled. If zero glyphs loaded for a non-empty label, a warning SHALL be emitted via `log.Warn()`.

#### Scenario: Glyphs loaded successfully
- **WHEN** a label with 5 characters is added to the texture atlas
- **THEN** `log.Debug()` SHALL report "Loaded N glyphs" (N=5)

#### Scenario: Zero glyphs warning
- **WHEN** a non-empty label text results in zero glyphs loaded (e.g., all characters produce errors)
- **THEN** `log.Warn()` SHALL report "Failed to load glyphs for label:" with the label text

### Requirement: Texture atlas dimensions logged in debug mode
When debug is enabled, the final texture atlas dimensions (width, height) and vertex count for text rendering SHALL be logged during `DrawMap()` execution.

#### Scenario: Atlas dimensions reported
- **WHEN** debug is enabled and text is rendered
- **THEN** `log.Debug()` SHALL output texture atlas width and height, plus the number of text vertices drawn

### Requirement: autoSize labels produce warning
When a text style with autoSize=true is encountered (and therefore skipped), a `log.Warn()` message SHALL be emitted once per unique text style noting that autoSize text rendering is not supported in the OpenGL backend.

#### Scenario: Warning on first autoSize skip
- **WHEN** any text style with autoSize=true is skipped
- **THEN** `log.Warn()` SHALL emit "OpenGL backend does not support autoSize text rendering, skipping label: <label>"
