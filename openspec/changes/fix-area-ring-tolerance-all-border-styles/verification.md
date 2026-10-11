# Verification

Evidence for `fix-area-ring-tolerance-all-border-styles`. The library revision for every run in
this note unless stated otherwise is the working tree of this change on `todo-backlog-metadata-pass`.

## Diagnosis (spec: `map-painter-area-culling` - A ring's visibility tolerance covers every border style the ring resolves)

The per-ring visibility decision of area preparation extended a ring by half of the width of
`borderStyles.front()`, and only when that style carried neither an offset nor a display offset; a
front style with an offset left the tolerance at `0.0`. The drawing step below draws every resolved
border style, each one shifted by its own offset:

| expression | before the change | after the change |
|---|---|---|
| per-ring tolerance | `ConvertWidthToPixel(front.width/2)` if the front style has no offsets, else `0.0` | max over the resolved styles of `ConvertWidthToPixel(width/2 + abs(displayOffset)) + abs(offset)/pixelSize` |
| frame-wide bound of the early rejection | `ConvertWidthToPixel(maxAreaBorderWidthMM/2)`, the widest width of the level, offsets ignored | `areaReachPixel` = the same expression over the per-level maxima of the three terms, computed once per frame in `UpdateVisibilityBounds` |
| OpenGL backend | repeated the front-style rule and passed its width in millimetres to `IsAreaRingVisible` | uses the shared tolerance and passes it in pixels |

No shipped stylesheet declares an area border offset (`grep -rn "BORDER(" stylesheets/ | grep -i
offset` is empty) and no rule draws a border without a fill, which is why the defect was latent.

## What the fix does

- `StyleConfig::VisibilityBounds` gains `maxAreaBorderWidth`, `maxAreaBorderDisplayOffset` (mm) and
  `maxAreaBorderOffset` (map units), filled per level from the built `areaBorderStyleSelectors` in
  `PostprocessVisibilityBounds`; the level count now includes the levels only the area border styles
  reach. `GetMaxAreaBorderWidthMM` stays as a wrapper of the width field.
- `osmscout::GetAreaRingTolerancePixel` (`libosmscout-map/include/osmscoutmap/AreaBorderReach.h`,
  `src/osmscoutmap/AreaBorderReach.cpp`, new, registered in both build systems) is the single
  expression above; `MapPainter::PrepareAreaRing` and `MapPainterOpenGL::ProcessAreas` both read it.
  It is a free function because `MapPainterOpenGL` is not a `MapPainter`.
- `MapPainter::UpdateVisibilityBounds` converts the three maxima into
  `DatabaseCacheEntry::areaReachPixel`; `MapPainter::ProcessAreas` uses it for the early rejection and
  the inline `GetMaxAreaBorderWidthMM` conversion is gone.
- The per-ring invariant assertion compares each term of the ring's tolerance with its frame bound in
  the unit the style sheet declares it in (width, display offset, offset), which catches a postprocess
  that stops collecting one of them.
- `osmscoutmapopengl::IsAreaRingVisible` takes the tolerance in pixels; its `minDimensionMM` argument
  and its role as the single decision site are unchanged.

## Falsification (each probe restores the pre-change expression)

| probe | observation |
|---|---|
| the reach collection of `UpdateAreaBorderReach` disabled | `StyleConfigVisibilityBoundsTest.cpp:326 FAILED`, 1 of 5 cases failed |
| the front-style rule restored in `PrepareAreaRing` | `MapPainterAreaVisibilityCullTest.cpp:1059` and `:1103 FAILED`, 2 of 11 cases failed (the two positive cases; the "beyond the reach" and "keeps its tolerance" cases pass on both implementations, as they must) |
| the width-only frame bound restored in `ProcessAreas` | `MapPainterAreaVisibilityCullTest.cpp:1236` and `:1297 FAILED`, 2 of 11 cases failed |
| the millimetre width passed where the pixel tolerance is expected in `MapPainterOpenGL::ProcessAreas` | `OpenGLAreaVisibilityTest.cpp:617 FAILED`, 1 of 7 cases failed (the offset case of the step; the visible-area case of the step passes on both, because a ring inside the view is kept by any tolerance) |
| the front-style rule restored in `PrepareAreaRing`, again for the render case | `MapPainterAreaBorderRenderTest.cpp:260 FAILED`, 1 of 1 case failed |

The line numbers are those of the final test files: the cases of the later probes were appended after
the earlier ones, so no probe's line number moved.

## The rendered comparison (spec scenario: the frame contains the drawn border geometry)

The case renders a synthetic frame with the SVG backend into a string and looks for the border colour
the style sheet declares (`#ff00ff`); the border is drawn at an offset of 2000 map units and the area
is placed outside the view by more than half of its border width can reach, so the border reaches the
view through the offset only.

```bash
cmake --build build --target MapPainterAreaBorderRenderTest
./build/Tests/MapPainterAreaBorderRenderTest
# Randomness seeded to: 2876986218
# ===============================================================================
# All tests passed (14 assertions in 1 test case)
```

- The area within the offset reach: the colour is present in the SVG document.
- The area beyond the reach (`tolerancePx + 10`): the colour is absent, so the case is a statement
  about the tolerance and not about a pipeline that draws everything it loaded.
- Falsified: with the front-style rule the border never reaches the frame and the case fails
  (`MapPainterAreaBorderRenderTest.cpp:260`), because the front style carries the offset and the old
  tolerance was `0.0`.

The OpenGL counterpart is the step-level case `The area step keeps a ring whose border reaches the
view through its offset` in `Tests/src/OpenGLAreaVisibilityTest.cpp`, which needs an offscreen GLFW
context and runs in this environment (no skip, 133 assertions in 7 cases for the file). A rendered
OpenGL/Cairo comparison of the same view is not performed: the OpenGL driver cannot render the area
style sheets of the Dortmund database while TODO §97 is open, and a synthetic frame would compare no
more than the prepared ring count that case already pins.

## Build and test sweeps

```bash
cmake --build build -j 8                       # 632 targets, 9m24s, no error
cd build && xvfb-run ctest -j 2 --output-on-failure
# 100% tests passed out of 145, Total Test time (real) = 103.62 sec

meson compile -C build-meson                  # no error, no new warning
xvfb-run meson test --timeout-multiplier 2 -C build-meson
# Ok: 145, Fail: 0, 1m41s
```

Both sweeps were repeated after the branch was rebased onto master, which meanwhile gained
`harden-polygon-triangulation` (PR #1874) and one further test: `cmake --build build` clean and
`xvfb-run ctest -j 2 --output-on-failure` **146 of 146 tests passed** (71.6 s), `meson test` **Ok: 146,
Fail: 0**. The counts below name the run of this change's own tree, the rebased run is the one the
pull request carries.

- The only warning in a touched file is the pre-existing `MapPainterOpenGL.cpp:458`
  `-Wunused-but-set-variable` for `lineOffset` (TODO §93); the line number moved from `:454` because
  this change added two lines above it, and the function is not touched by this change.
- The meson build needed the pango stack in the dependency list of the new test, because a meson
  `link_with` does not propagate the include directories of `MapPainterSVG.h` the way the CMake target
  does; the test now lists `pangoft2Dep`, `ftDep` and `fontconfigDep` when they are found.
- Four tests are registered in both build systems for this change: `StyleConfigVisibilityBoundsTest`,
  `MapPainterAreaVisibilityCullTest`, `OpenGLAreaVisibilityTest` (extended) and
  `MapPainterAreaBorderRenderTest` (new).

Sanitizer configuration, whose asserts are active:

```bash
cmake -B build-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=address -fsanitize=undefined" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=address -fsanitize=undefined" \
  -DOSMSCOUT_BUILD_TOOL_OSMSCOUT2=OFF -DOSMSCOUT_BUILD_TOOL_OSMSCOUTOPENGL=OFF \
  -DOSMSCOUT_BUILD_DEMOS=OFF -DOSMSCOUT_BUILD_TOOL_STYLEEDITOR=OFF \
  -DOSMSCOUT_BUILD_BINDING_JAVA=OFF -DCMAKE_UNITY_BUILD=ON -G Ninja
cmake --build build-asan --target MapPainterAreaVisibilityCullTest StyleConfigVisibilityBoundsTest -j 6   # 1m06s
ldd build-asan/Tests/MapPainterAreaVisibilityCullTest-1.1.1 | grep tcmalloc   # no output
ASAN_OPTIONS=detect_leaks=0 ./build-asan/Tests/MapPainterAreaVisibilityCullTest      # 732 assertions, 11 cases
ASAN_OPTIONS=detect_leaks=0 ./build-asan/Tests/StyleConfigVisibilityBoundsTest        # 37 assertions, 5 cases
```

The `build-asan` directory had to be re-configured: it was generated by a CMake that a toolchain
update removed (`.../cmake-4.4.3-linux-x86_64/bin/cmake: Datei oder Verzeichnis nicht gefunden`), so
`cmake --build build-asan` failed in `Re-running CMake` before any compilation. The command above
reproduces the documented configuration; it is recorded in `TODO.md` as an environment issue.

## Conventions (task 5.5)

- `guidelines/CodeStyles.md`: the added code follows the include order, the naming, the brace style and
  the comment conventions of the files it joins; the two new files carry the library's licence header
  and a header guard named after the file.
- Uncrustify 0.83.0 (`scripts/format-check.sh check`, `uncrustify -c .uncrustify -l CPP --check`):
  - The three files this change adds - `libosmscout-map/include/osmscoutmap/AreaBorderReach.h`,
    `libosmscout-map/src/osmscoutmap/AreaBorderReach.cpp` and
    `Tests/src/MapPainterAreaBorderRenderTest.cpp` - pass the check.
  - The modified files drift file-wide (4 to 94 hunks each: `MapPainter.cpp` 94,
    `MapPainterOpenGL.cpp` 47, `StyleConfig.h` 16), exactly the state TODO §80 records (800 of the 906
    tracked files differ; the same check reports a change for files this change never touched).
    Attributing the formatter's changes by line - the added ranges of `git diff -U0` against the
    removed lines of the formatter's diff - leaves **0** changes on a line this change added. The two
    findings the added lines had produced themselves were fixed: a `0.5` magic number (now
    `borderReachFactor`) and a missing parenthesis in the multiplication of the frame-wide area reach.
    Reformatting the rest would be the file-wide pass that TODO §80 leaves to its own decision, so it
    is not part of this change.
  - Two pre-existing classes do appear on lines that neighbour the additions, and the neighbouring
    pre-existing code carries them identically: `misc-include-cleaner` (`no header providing
    osmscout::BorderStyle is directly included` in the new postprocess helper, as in its sibling
    helpers) and `cppcoreguidelines-pro-bounds-avoid-unchecked-container-access`
    (`selectorsForType[level]`, guarded by a size test, as in the sibling helpers).
- clang-tidy (LLVM 23.1.1 with the repository `.clang-tidy`): the new `AreaBorderReach.cpp` reports no
  finding; the added lines of the modified files report none of their own beyond the two classes
  above, which the same files carry throughout.
- `AGENTS.md`: not updated. The change adds one header/source pair to `libosmscout-map` and one test
  target, and neither the repository map nor the build sections enumerate individual files or tests;
  no build option, platform note, workflow or CI job changes. The backlog update belongs to `TODO.md`
  and was made there.

## Scenario traceability

| Scenario (`specs/map-painter-area-culling/spec.md`) | Case or observation |
|---|---|
| A border drawn at an offset keeps its ring | `MapPainterAreaVisibilityCullTest` "A border drawn at an offset keeps its ring"; `MapPainterAreaBorderRenderTest` (the rendered frame) |
| The widest drawn border decides the tolerance | `MapPainterAreaVisibilityCullTest` "The widest drawn border decides the tolerance" |
| A ring whose drawn borders do not reach the view contributes nothing | `MapPainterAreaVisibilityCullTest` "A ring beyond the reach of its drawn borders contributes nothing"; the far half of `MapPainterAreaBorderRenderTest` |
| A ring without an offset border keeps its tolerance | `MapPainterAreaVisibilityCullTest` "A ring without an offset border keeps its tolerance" |
| No area that a per-ring decision would keep is rejected early | `MapPainterAreaVisibilityCullTest` "An area within the border tolerance is not rejected" (existing), the new early cases, and the term-wise assert in a Debug/sanitizer build |
| The tolerance grows with the stylesheet | `StyleConfigVisibilityBoundsTest` "The border width bound is the widest border style of a level" (existing) and "The area border reach bound is the widest border and the largest offsets of a level" (new) |
| The early tolerance follows the DPI of the frame | `MapPainterAreaVisibilityCullTest` "The border tolerance of a stylesheet width follows the DPI of the projection" (existing) and "The area reach of the frame follows the DPI and the offsets of the stylesheet" (new) |
| An area whose only reachable border is offset is not rejected early | `MapPainterAreaVisibilityCullTest` "An area whose only reachable border is offset is not rejected early" |
| Every painter applies a stylesheet-derived tolerance in the frame's pixels (`fix-opengl-area-visibility-cull`, unarchived) | `OpenGLAreaVisibilityTest` "The tolerance of the decision follows the DPI of the frame", now through the shared helper |

## Limitations

- The prepared-entry count of the Dortmund view is not re-measured, for the core painter as well as
  for the OpenGL step. The tolerance can only grow (a maximum over the resolved styles covers the
  front style's half width, and the front style contributed nothing at all when it carried an offset),
  so no ring the previous decision kept can be dropped; what a re-measurement would show beyond that
  is the intentional widening at the viewport edge for a ring whose front style is narrower than
  another slot of the same ring, which the tolerance requirement exempts from the "unchanged"
  comparison.
- The OpenGL step's decision is now wider than its own drawing: the step draws every resolved border
  style with the front style's width and applies no border offset at all, so for a stylesheet that
  declares an offset the step keeps rings it draws degenerately. The decision stays conservative for
  that backend. The divergence is recorded in `TODO.md` as a defect of its own change and is out of
  scope here.
- The exact drawn pixels of a view are compared only by the SVG case of this change, which pins the
  presence of the border marker rather than a pixel-exact image.

## Residue recorded in `TODO.md`

- The OpenGL area step draws every resolved border style with the width of the front style and applies
  no border offset (`libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:377-410`).
- `build-asan` broke on a toolchain update of CMake and had to be re-configured.
- The entry this change closes (TODO §99) is removed, and the `file:line` references of the entries
  that this change moved are corrected.
