## Context

User runs DrawMapOpenGL with valid CLI params (font file exists, shaders correct, valid OSM data), but output PPM shows zero text labels. Problem predates recent text handling changes — regression.

Current text pipeline:

1. `DrawMapOpenGL` → parses `--fontName`, `--fontSize`, passes to `MapPainterOpenGL(fontPath, ..., parameter)`
2. `MapPainterOpenGL` ctor → creates `TextLoader(fontPath, parameter.GetFontSize(), dpi)` which loads FreeType face
3. `ProcessNodes()` → for each node, calls `ProcessNode()` which:
   - Gets text styles via `styleConfig->GetNodeTextStyles()`
   - For each text style with non-empty label, renders glyph → texture atlas
4. `DrawMap()` → binds text shaders, loads textures, draws quads with glyph atlas

Key bug areas identified:

### A. autoSize labels unconditionally skipped

`ProcessNode()` line ~983-987:
```cpp
} else if (textStyle->GetAutoSize()) {
    //TODO
    continue;  // SKIPS LABEL ENTIRELY
}
```

Any text style with `autoSize: true` (used heavily in `standard.oss` for area labels) is skipped. This is a known TODO, but the regression means area text labels never rendered even before — however node labels should still work.

### B. scaleAndFadeMag threshold may block labels

All text rendering requires magnification > scaleAndFadeMag when drawFadings=true (default). If scaleAndFadeMag defaults to a high level (e.g., `magVeryClose` equivalent to zoom ~400000), labels won't render at user's zoom 160000.

### C. No CLI validation for font path / font size

- No check that `--fontName` file exists and is readable before constructing painter
- No validation that `--fontSize` is reasonable (> 0)
- No warning if `TextLoader` loads zero glyphs

## Goals / Non-Goals

**Goals:**
- Add CLI input validation in DrawMapOpenGL for font path, font size
- Fix regression so text labels render at typical zoom levels
- Add logging/diagnostics when text rendering is skipped or fails
- Ensure warnings when autoSize labels are skipped (known limitation)

**Non-Goals:**
- Implement autoSize text rendering for OpenGL backend (large separate feature)
- Add way/area text rendering to OpenGL backend
- Change text rendering in other backends (AGG, Cairo, Qt)
- Performance optimization of glyph atlas

## Decisions

### Decision 1: Validate font path at CLI parsing time

**Chosen:** Add file-exists check in DrawMapOpenGL.cpp after parsing, before constructing painter.

**Rationale:** Better UX — fail fast with clear error message instead of silent failure. Other backends (DrawMapAgg etc.) already do similar validation implicitly via error returns.

### Decision 2: Fix scaleAndFadeMag default vs. zoom

**Chosen:** Ensure text renders when magnification level ≥ textStyle scaleAndFadeMag. If scaleAndFadeMag is at default (magCruising or similar high level), labels at zoom 160000 (~magCity) should still render. Debug logging shows which branch each text style takes.

**Rationale:** The current condition `magnification > scaleAndFadeMag` may block rendering for many node styles depending on default. Need to verify and potentially adjust.

### Decision 3: Add debug logging throughout text pipeline

Flow:
- `fontPath` validated ✓ / ✗
- `TextLoader::LoadFace()` success/failure
- `ProcessNode()` per text style: which branch taken (scale/fade / autoSize-skip / normal), fontSize, alpha, label text
- `AddCharactersToTextureAtlas()`: glyph count loaded
- `textRenderer.Draw()`: vertex count, texture dimensions

### Decision 4: Remove or warn on autoSize skip

Add `log.Warn()` when skipping autoSize text style, noting this is a known TODO. Clear that area text labels won't render.

## Risks / Trade-offs

| Risk | Mitigation |
|------|------------|
| **Missing text even with fixes** — node labels still not visible | Add per-node text style debug output to identify exactly which branch blocks rendering |
| **scaleAndFadeMag default unknown** — may behave differently per style | Read default value from TextStyle; verify against zoom 160000 |
| **Font size too small** — glyphs render but invisible at rendered scale | Validate fontSize > 0, log actual FreeType pixel size |
| **TextLoader atlas creation bug** — texture atlas data corrupted | Validate atlas dimensions and pixel data non-empty; add unit test |