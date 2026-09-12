## 1. SVG symbol renderer fixes (spec: svg-symbol-renderer)

- [x] 1.1 Convert border width from mm to pixels in `SymbolRendererSVG::SetBorder()` by multiplying the border width with the `screenMmInPixel` factor, and verify the test cases "SetBorder converts border width from mm to pixels" and "SetBorder converts mm width to pixels via screenMmInPixel" in `Tests/src/SymbolRendererSVGTest.cpp` pass
- [x] 1.2 Emit a closed `<polygon>` element in `SymbolRendererSVG::DrawPolygon()` instead of `<polyline>`, and verify the test cases "DrawPolygon outputs polygon element with points" and "Render with polygon primitive" in `Tests/src/SymbolRendererSVGTest.cpp` pass

## 2. Qt symbol renderer fix

- [x] 2.1 Close the polygon path with `closeSubpath()` in `SymbolRendererQt::DrawPolygon()` so stroked polygon outlines include the closing segment, and verify a stroked polygon symbol renders with a closed outline in the Qt backend output

## 3. Spec update

- [x] 3.1 Update `openspec/specs/svg-symbol-renderer/spec.md` with the border width conversion and closed polygon requirements, and verify `openspec validate --change fix-symbol-rendering-svg-qt` passes

## 4. Verification

- [x] 4.1 Configure and build the affected targets with cmake (libosmscout-map-svg, libosmscout-map-qt, Tests), and verify the build compiles without errors
- [x] 4.2 Run `SymbolRendererSVGTest` via ctest, and verify all cases pass
- [x] 4.3 Run the existing test suite via ctest (110/110 passed), and verify no regressions in unrelated areas
