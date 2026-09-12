## 1. SVG dashed border implementation (spec: svg-symbol-renderer)

- [ ] 1.1 In `SymbolRendererSVG::SetBorder()`, compute the dash array from `borderStyle->GetDash()` scaled by the converted border width when dashes are present, store it as renderer state, and remove the stale comment; verify solid borders keep current output (no dash attribute)
- [ ] 1.2 Emit `stroke-dasharray` in the stroke writer (`WriteFillAndStroke()`) when a dash pattern is set; verify a dashed-border symbol renders with dashes in SVG output
- [ ] 1.3 Add test cases to `Tests/src/SymbolRendererSVGTest.cpp` covering the spec scenarios (dashed border emits `stroke-dasharray="4 4"` with `stroke-width="2"`; solid border emits no `stroke-dasharray`), and verify they pass

## 2. Spec update

- [ ] 2.1 Update `openspec/specs/svg-symbol-renderer/spec.md` with the dashed border requirement, and verify `openspec validate` passes

## 3. Verification

- [ ] 3.1 Rebuild the affected targets with cmake/Ninja (libosmscout-map-svg, `SymbolRendererSVGTest`), and verify the build compiles without errors
- [ ] 3.2 Run `SymbolRendererSVGTest` via ctest, and verify all cases pass
- [ ] 3.3 Run the existing test suite via ctest, and verify no regressions (e.g. symbol-dependent tests, SVG icon tests)
