## 1. SVG dashed border implementation (spec: svg-symbol-renderer)

- [x] 1.1 In `SymbolRendererSVG::SetBorder()`, compute the dash array from `borderStyle->GetDash()` scaled by the converted border width when dashes are present, store it as renderer state, and remove the stale comment; verified solid borders keep current output (no dash attribute)
- [x] 1.2 Emit `stroke-dasharray` in the stroke writer (`WriteFillAndStroke()`) when a dash pattern is set; verified a dashed-border symbol renders with dashes in SVG output (test "SetBorder with dashes emits scaled dash array")
- [x] 1.3 Add test cases to `Tests/src/SymbolRendererSVGTest.cpp` covering the spec scenarios (dashed border emits `stroke-dasharray="4 4"` with `stroke-width="2"`; solid border emits no `stroke-dasharray`) — verified: both pass via ctest

## 2. Spec update

- [x] 2.1 Update `openspec/specs/svg-symbol-renderer/spec.md` with the dashed border requirement — verified: `openspec validate` passes (121 specs)

## 3. Verification

- [x] 3.1 Rebuild the affected targets with cmake/Ninja (libosmscout-map-svg, `SymbolRendererSVGTest`) — compiles without errors
- [x] 3.2 Run `SymbolRendererSVGTest` via ctest — all cases pass (incl. 2 new)
- [x] 3.3 Run the existing test suite via ctest — 111/111 passed, no regressions
