## 1. CLI Validation

- [x] 1.1 Add font path file-exists check in DrawMapOpenGL.cpp before painter construction (spec: opengl-text-validation, effort: 1)
- [x] 1.2 Add font path empty-string check in DrawMapOpenGL.cpp (spec: opengl-text-validation, effort: 1)
- [x] 1.3 Add path-is-directory check for font path (spec: opengl-text-validation, effort: 1)
- [x] 1.4 Add font size positive-value check in DrawMapOpenGL.cpp (spec: opengl-text-validation, effort: 1)
- [x] 1.5 Add clear error messages for each validation failure with non-zero exit code (spec: opengl-text-validation, effort: 2)

## 2. Investigate & Fix scaleAndFadeMag Label Skip

- [x] 2.1 Add debug logging to ProcessNode() showing which branch each text style enters (scale/fade, autoSize-skip, normal) (spec: opengl-text-rendering-diagnostics, effort: 3)
- [x] 2.2 Run DrawMapOpenGL with debug output at zoom 160000 and verify which styles are blocked by scaleAndFadeMag threshold (spec: opengl-text-rendering-diagnostics, effort: 2)
- [x] 2.3 Fix or adjust scaleAndFadeMag condition so node labels render at typical zoom levels (effort: 5)

## 3. autoSize Warning & Known Limitation

- [x] 3.1 Add `log.Warn()` when autoSize text style is skipped in ProcessNode() (spec: opengl-text-rendering-diagnostics, effort: 1)
- [x] 3.2 Add `log.Debug()` output for autoSize skip containing label text (spec: opengl-text-rendering-diagnostics, effort: 1)

## 4. TextLoader Diagnostics

- [x] 4.1 Add debug log in AddCharactersToTextureAtlas() reporting glyph count loaded per label (spec: opengl-text-rendering-diagnostics, effort: 1)
- [x] 4.2 Add warning when non-empty label results in zero glyphs loaded (spec: opengl-text-rendering-diagnostics, effort: 1)
- [x] 4.3 Add debug log for texture atlas dimensions and text vertex count in DrawMap() (spec: opengl-text-rendering-diagnostics, effort: 2)

## 5. Integration Test

- [x] 5.1 Build and run DrawMapOpenGL with test data, verify text labels appear in output PPM (effort: 3)
- [x] 5.2 Verify CLI validation rejects bad font path/size with correct error messages (effort: 1)
- [x] 5.3 Verify debug output shows text rendering pipeline details (effort: 1)