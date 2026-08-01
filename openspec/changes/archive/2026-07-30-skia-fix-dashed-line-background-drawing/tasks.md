## 1. Fix Dash Width Scaling

- [x] 1.1 In `MapPainterSkia::SetLineAttributes()`, multiply each dash interval by `width` before passing to `SkDashPathEffect::Make()`
- [x] 1.2 Remove spurious `[0,0]` prefix from Qt dash pattern in `MapPainterQt::DrawPath()`
- [x] 1.3 Verify build compiles with no errors or warnings

## 2. Verify Correctness

- [x] 2.1 Run existing Skia tests (`Tests/src/MapPainterSkiaTest.cpp`) to confirm no regressions
- [x] 2.2 Run full test suite: `cd build && ctest -j 2 --output-on-failure`
- [x] 2.3 Visual comparison: render a dashed line with gap color using Skia backend and confirm output matches Cairo/AGG
