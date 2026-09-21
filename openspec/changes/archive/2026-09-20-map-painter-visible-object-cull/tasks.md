# Tasks: map-painter-visible-object-cull

Parent specs: `specs/map-painter-way-culling/spec.md`,
`specs/map-painter-point-object-culling/spec.md`,
`specs/map-painter-label-culling/spec.md`. Approach: `design.md` (decisions D1 - D7).

Base assumption: current `master` (no branch work merged).

## 1. Per-level visibility bounds (`map-painter-way-culling`, `map-painter-point-object-culling`, `map-painter-label-culling`)

- [x] 1.1 Define the per-level visibility bound record on `StyleConfig` (way line width reach, icon and symbol extent, label extent) with a documented accessor whose conservative direction is stated in the header; verify the build compiles and that the accessor is documented next to the existing per-level queries (D2, D3).
- [x] 1.2 Derive the way line width bound during the existing per-level postprocessing of the way style lookup tables, including the widest width a data-carried width value can contribute; verify with a unit test that the bound at a level equals the widest line style of that level and grows when a stylesheet with wider line styles is loaded (`map-painter-way-culling`, requirement "A way that cannot be visible is rejected before its preparation").
- [x] 1.3 Derive the icon and symbol extent bound per level from the level's icon and symbol styles; verify with a unit test that the bound is the widest icon and symbol of the level and grows with a stylesheet containing larger symbols (`map-painter-point-object-culling`, requirement "A point object that cannot be visible is rejected before its preparation").
- [x] 1.4 Implement the conservative label extent bound from the map parameters and a label's character count and font size, and document its derivation; verify with a unit test that the bound covers the measured rectangle of labels registered in the test views and grows with font size and character count (`map-painter-label-culling`, requirement "A label that cannot intersect the view is neither measured nor stored nor laid out").

## 2. Way rejection (`map-painter-way-culling`)

- [x] 2.1 Add the shared way visibility predicate and apply it in `CalculateWayPaths` before the line style resolution; verify with a new unit test that a loaded way outside the view contributes no prepared way and no transformed coordinates to the frame (requirement 1, scenario "A way outside the view contributes nothing").
- [x] 2.2 Verify the conservativeness of the way predicate with a unit test over two stylesheets whose widest line style differs by a large factor: the reach grows with the stylesheet, is never smaller than the per-line-style tolerance, and a way inside the viewport plus that reach is still prepared (requirement 1, scenario "The rejection is never more aggressive than the per-line-style decision").
- [x] 2.3 Apply the rejection to way shields after the shield style and its label text are resolved, but before the grid positions are built and the label is registered - the extent of a shield label depends on its text, so no sound bound precedes the style lookup; verify with a unit test that a shield-styled way outside the view adds no labels, and that the shield labels of a visible way keep their text, positions, priorities and order (requirement 3).
- [x] 2.4 Verify that way preparation work and allocation follow the prepared ways: unit test comparing the prepared ways, coordinates and allocation count of a view with and without a large number of further loaded ways outside the view (requirement 2).
- [x] 2.5 Verify the regression property of the capability: prepared ways, their transformed coordinates and their draw order are identical for a view with equal-comparing ways (requirement 4, scenario "Prepared way entries are unchanged"), using the existing frame buffer and route tests as additional guards.

## 3. Point-object rejection (`map-painter-point-object-culling`)

- [x] 3.1 Add the point object decision to the preparation: reject an object of a type that resolves no label style at the level of the frame before any of its styles is resolved (only its icon and symbol can reach the view, and the stylesheet bounds their extent), and reject an object whose label elements cannot reach the view before those elements are registered (the extent of a label is set by its text); verify with unit tests that a far object registers no label elements, that the label set is identical to the view without it, and that an object of a type without a label style adds no heap allocation (requirement 1).
- [x] 3.2 Verify that a point object inside the view is prepared as before (same elements, text, positions, priorities, order) and that an object at the viewport edge whose label or icon rectangle reaches into the view keeps its elements (requirement 1, scenarios 3 and 4).
- [x] 3.3 Verify that point object preparation work and allocation follow the objects that can be visible: unit test comparing the registered elements of a view with and without a large number of further loaded point objects outside the view, plus a unit test asserting that objects of a type without a label style add no heap allocation at all (requirement 2).
- [x] 3.4 Verify the regression property: the registered element set of a view is identical element by element with and without the change (requirement 3, scenario "The registered element set is unchanged").

## 4. Label rejection (`map-painter-label-culling`)

- [x] 4.1 Add the early label decision to the shared label layouter registration path (both regular label registrations and the contour label registration) in front of the measurement call, using the anchor position, the extent bounds and the frame's layout viewport; verify with a unit test that a label outside the view is not measured, not stored and not laid out, observed through a counting test painter (requirement 1, scenario "A label far outside the view is not measured").
- [x] 4.2 Verify the conservativeness of the label decision: a label whose anchor lies outside the viewport but whose rectangle intersects it is measured, stored and drawn exactly as before, and a label outside the viewport but inside the extent bound is kept (requirement 1, scenario "A label whose rectangle reaches into the view is kept").
- [x] 4.3 Verify that the rule holds for every label source with unit tests covering a node label, an area label, a way shield label and a contour label of an off-view way (requirement 1, scenario "The same rule holds for every label source"); the node, area and shield labels are covered by the element and element list tests of the shared label stage, the contour label of an off-view way by the way rejection test of group 2.
- [x] 4.4 Add the debug assertion of D7 on the built label elements: the rectangle of every element has to stay inside the anchor plus the reach of the label; verified in an assertions-enabled build that the assertion fires with a deliberately reduced bound and does not fire for the full test suite (design D7).
- [x] 4.5 Verify that the label stage budget follows the labels that can appear: unit test comparing the label set, placement and measurement count of a view with and without a large number of further loaded off-view labels, with the measurement count equal to the labels whose reach touches the layout viewport (requirement 2).
- [x] 4.6 Verify the regression property of the capability: label set, placement, order and measured dimensions of a view are identical with and without the change (requirement 3, scenarios "The label set and placement are unchanged" and "The text measurement results are unchanged").

## 5. Measurement before and after

- [x] 5.1 Record the baseline of the fixed view (Dortmund, zoom 15, single tile, the view of the previous changes) and of the zoom band 14 - 17 for the way preparation, shield, point object and label steps, using the two-library `LD_LIBRARY_PATH` A/B harness, with allocation counts as the reproducible anchor; record the numbers in `verification.md`.
- [x] 5.2 Measure the same steps with the change applied and record the achieved shares and the absence of a regression in the steps that the change does not touch (design: risks, "the conservative bound is too coarse"); `#4` ProcessAreas is unchanged in time and allocations (161 per run in both variants).

## 6. Build, tests, linters and documentation

- [x] 6.1 Verify the CMake build compiles without errors and without new warnings in the changed files (`cmake --build build`).
- [x] 6.2 Verify the Meson build compiles without new warnings in the changed files and build the new test targets (`meson compile -C build-meson`).
- [x] 6.3 Register the new test files in `Tests/CMakeLists.txt` and `Tests/meson.build` and verify both build systems build and run them.
- [x] 6.4 Verify the existing test suite passes: `ctest -j 2 --output-on-failure` in `build/` (121 of 121) and `meson test -C build-meson` (Ok 74, Fail 0), including `MapPainterShieldTest`, `MapPainterShieldQtTest`, `MapPainterRouteTest`, `MapPainterAreaPreparationTest`, `MapPainterFrameBuffersTest` and `PerformanceTest`.
- [x] 6.5 Verify the formatting and static analysis of the changed files with the project `.uncrustify` configuration and `.clang-tidy` (no added drift in the new files and no added line flagged in the library files; no new clang-tidy category).
- [x] 6.6 Verify the rendered output is unchanged: render the same view with and without the change using a stylesheet that draws icons, symbols, shields and labels, and compare the images byte for byte; the comparison found two defects of the first implementation (design D9), which are fixed, and the final render is byte-identical (0 differing pixels).
- [x] 6.7 Record the findings of this work in `TODO.md`: the label layouter viewport that belongs to the previous frame in the early steps, the uninstantiable single element `RegisterLabel` overload, the per-object label element store of the point label path, and the missing line style observation hook. The loader side over-fetch that the analysis found is recorded there as well (a zoom 14 tile loads 1621 areas for a view whose zoom 15 tile, four times smaller, loads 17629; the database load step costs 91.9 ms against a 15.2 ms painter-side frame at zoom 15).
- [x] 6.8 Verify the documentation of the change: the new `StyleConfig` accessor and the new label helper functions are documented in their headers; `AGENTS.md` does not need a change (no structural, build or roadmap change), `TODO.md` is updated.
- [x] 6.9 Write the verification record in `verification.md`: every scenario of the three specs traced to the test or measurement that covers it, the sensitivity of the new tests (checked against deliberately broken variants), and the results of tasks 6.1 - 6.8.
