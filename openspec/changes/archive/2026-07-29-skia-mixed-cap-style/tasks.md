# Tasks — Mixed Cap Style Handling

## Task 1: Implement mixed cap handling in DrawPath() [x]

Modify `MapPainterSkia::DrawPath()` in `MapPainterSkia.cpp`:

1. Determine the more restrictive cap from `startCap` and `endCap` (Butt > Square > Round)
2. Draw the main path with that cap via `SetLineAttributes()`
3. If `startCap == Round` and the effective cap is not Round, draw a filled circle at the start point
4. If `endCap == Round` and the effective cap is not Round, draw a filled circle at the end point

**Files:**
- `libosmscout-map-skia/src/osmscoutmapskia/MapPainterSkia.cpp` — `DrawPath()` changes

**Acceptance:**
- Path with `startCap=Round, endCap=Butt` renders round start, flat end
- Path with `startCap=Butt, endCap=Round` renders flat start, round end
- Path with `startCap=Round, endCap=Round` renders round both ends (unchanged)
- Path with `startCap=Butt, endCap=Butt` renders flat both ends (unchanged)
- Dashed paths with mixed caps render correctly
- No regression for uniform cap paths

## Task 2: Verify build and run tests [x]

Build the project and run existing Skia map backend tests.

**Commands:**
```bash
cmake -B build -DOSMSCOUT_BUILD_MAP_SKIA=ON
cmake --build build
cd build && ctest -R "MapPainterSkia|SymbolRendererSkia" --output-on-failure
```

**Acceptance:** All existing tests pass. No new warnings.
