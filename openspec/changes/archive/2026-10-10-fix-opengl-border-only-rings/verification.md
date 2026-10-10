# Verification

What was run for `fix-opengl-border-only-rings`, on which tree, and what the result was. Every command ran in
`/home/tim/projects/libosmscout` on the branch `fix-loop-2026-10-10`, at the revision of the iteration
(`1e1ecc00d` plus the two modified files whose `md5sum` table is in section 2).

## Environment

| item | value |
|---|---|
| build | `build/` (CMake, Ninja, `CMAKE_BUILD_TYPE=Release`, `OSMSCOUT_BUILD_MAP_OPENGL=ON`) |
| second build system | `build-meson/` (Meson, `unity=on`) |
| GL | no GPU; `DISPLAY=:0` and `WAYLAND_DISPLAY=wayland-0` present, so the GLFW offscreen window of the painter cases is created and those cases run instead of skipping |
| uncrustify | `Uncrustify-0.83.0_f` (the version `scripts/format-check.sh` requires) |
| clang-tidy | clang-tidy with `-p build` |
| database | none; the case builds its own `TypeConfig`, style sheet and areas |

## 1. The defect and the red case (Phase A)

`MapPainterOpenGL::ProcessAreas` left the ring loop at `if (!fillStyle) { continue; }`
(`libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:313` before the fix), after the style
resolution and the visibility decision, so a ring the loaded style sheet draws by a border and by no fill
contributed nothing. `MapPainterAgg::DrawFill`
(`libosmscout-map-agg/src/osmscoutmapagg/MapPainterAgg.cpp:151-170`) draws the fill only when a fill style
exists and strokes `borderStyle` whenever it is set, so the other backends draw that border.

The case `Tests/src/OpenGLAreaVisibilityTest.cpp` - "A ring drawn only by a border is kept" - loads two areas
in view, one of a type the style sheet draws by one border style and by no fill, and one of a type the style
sheet does not mention, prepares the frame through an offscreen painter and asserts the examined and kept
ring counts of the step.

Red run on the unmodified painter, 2026-10-10 12:00 UTC (ctest log header `Start testing: Oct 10 14:00 CEST`):

```
/home/tim/projects/libosmscout/Tests/src/OpenGLAreaVisibilityTest.cpp:581: FAILED:
  CHECK( painter.GetKeptRingCount()==1 )
with expansion:
  0 == 1
test cases:   6 |   5 passed | 1 failed
assertions: 116 | 115 passed | 1 failed
```

- `evidence/red-head-ctest-LastTest.log` - the ctest log of that run.
- `evidence/red-head-case-console.txt` - the same failure from a single-case run of the binary
  (`./build/Tests/OpenGLAreaVisibilityTest "A ring drawn only by a border is kept"`,
  `test cases: 1 | 0 passed | 1 failed`, `assertions: 15 | 14 passed | 1 failed`).

The passing assertion `CHECK( painter.GetExaminedRingCount()==1 )` in the same run is the control: the
border-only ring reaches the style resolution and the visibility decision, and the unstyled ring does not
reach them.

## 2. The fix

`git diff --numstat` of the change:

```
132  14  Tests/src/OpenGLAreaVisibilityTest.cpp
 45  45  libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp
```

The painter's fill path is now conditional on the resolved fill style and `keptRingCount` counts the rings
the step keeps before that guard, so the ring reaches the border loop the loop already had. No other line of
the step changes; the 45 deleted and 45 added lines of the production file are the guard and the re-indent of
the block it wraps.

The two touched files, with their `md5sum` as measured on this tree:

| file | `md5sum` |
|---|---|
| `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` | `7e7bce57294311e93cd662be2a5570a9` |
| `Tests/src/OpenGLAreaVisibilityTest.cpp` | `b88b768b375e507330da3b4e04d96038` |

Focused runs after the fix:

| command | result | when |
|---|---|---|
| `ctest -R OpenGLAreaVisibilityTest --output-on-failure` in `build/` | Passed, 1/1 - the same case's later run on the restored tree is `evidence/probe-1-restored-cmake.txt`; this row's own runner log is **not retained** | 2026-10-10 12:02:06 UTC |
| `meson test -C build-meson --print-errorlogs "Check OpenGL area visibility"` | `Ok: 1 Fail: 0` - **not retained** (the focused run left no artifact; `evidence/gate-meson-testlog-excerpt.txt` carries the gate run of the same case, not this one) | 2026-10-10 12:02:44 UTC |

## 3. Probes (`TEMP-REVERT-PROBE`)

Both probes were run in the CMake build with the focused case; each mutation was removed before the next
step, and `grep -rn TEMP-REVERT-PROBE libosmscout-map-opengl/ Tests/src/` on the final tree finds nothing.

### 3.1 The defect restored (the invariant the fix adds)

Mutation: `if (!fillStyle) { continue; }` reinserted in front of the fill guard.
Failure, 2026-10-10 12:02:57 UTC: `CHECK( painter.GetKeptRingCount()==1 )` with expansion `0 == 1`,
`test cases: 6 | 5 passed | 1 failed`, `assertions: 116 | 115 passed | 1 failed` -
`evidence/probe-1-defect-restored-cmake.txt` and `evidence/probe-1-defect-restored-ctest-LastTest.log`.
Mutation removed, 2026-10-10 12:03:06 UTC: the case passes again -
`evidence/probe-1-restored-cmake.txt`.

### 3.2 The style guard relaxed (the assertion that guards behaviour older than the fix)

Mutation: the guard `if (!fillStyle && borderStyles.empty()) { continue; }` of the ring loop removed, so the
unstyled ring reaches the visibility decision as well.
Failure, 2026-10-10 12:03:17 UTC: `CHECK( painter.GetExaminedRingCount()==1 )` with expansion `2 == 1` and
`CHECK( painter.GetKeptRingCount()==1 )` with expansion `2 == 1`,
`test cases: 6 | 5 passed | 1 failed`, `assertions: 116 | 114 passed | 2 failed` -
`evidence/probe-2-style-guard-relaxed-cmake.txt`. Mutation removed; the case passes in the gate of
section 4 (`All tests passed (116 assertions in 6 test cases)` in `evidence/gate-meson-testlog-excerpt.txt`),
which is the green run after this probe.

## 4. Gate (Phase D) - both build systems, one run each

| step | command | result | when |
|---|---|---|---|
| CMake build | `cmake --build build -j "$(nproc)"` | exit 0, 255/255 steps | 12:04:42 - 12:07 UTC |
| CMake suite | `QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR=…/Tests TESTS_TMP_DIR=…/build/Tests ctest -j 2 --output-on-failure` | **149/149 passed**, 44.45 s | `Start testing: Oct 10 14:07 CEST`, `End testing: Oct 10 14:08 CEST` |
| Meson build | `meson compile -C build-meson` | exit 0, 393/393 targets | 12:08:36 - 12:15 UTC |
| Meson suite | `meson test -C build-meson --timeout-multiplier 2 --print-errorlogs` | **`Ok: 149 Fail: 0`** | log header `2026-10-10T14:15:26`, last test 12:16:21 UTC |

The two suite logs are `evidence/gate-cmake-ctest.txt` (19 KB) and `evidence/gate-meson-test.txt` (14 KB);
the runner logs they come from are kept as excerpts (`evidence/gate-cmake-ctest-LastTest-excerpt.txt`,
`evidence/gate-meson-testlog-excerpt.txt`), because a full runner log is overwritten by the next run of either
runner - **not retained in full**, reproducible with the commands above. The Meson excerpt
carries the case's own tally of the gate run, `All tests passed (116 assertions in 6 test cases)`, so the new
case ran in the gate and not only in the focused run.

Warnings: the gate's CMake build printed `45` warnings - `evidence/gate-cmake-build-excerpt.txt` carries the
bare count, not the warning text, and no artifact carries the set of the tree before the change, so the "same
set as before" reading is **not retained**. The one diagnostic the change's log was read as carrying in a
touched file is the pre-existing `MapPainterOpenGL.cpp:454` `-Wunused-but-set-variable` for `lineOffset`, which
the backlog records (the bullet at `TODO.md:364`); no new warning beyond that reading is claimed.

## 5. Formatter and static analysis (task 5.1)

**clang-tidy, production file.** `clang-tidy -p build
libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` reported **1204 findings on the changed file
and 1204 on the same file at HEAD**, with the same class distribution - **both counts are not retained** (no
clang-tidy output was kept as an artifact), so the "adds none" reading rests on that unretained run. The
comparison was taken by formatting the file at HEAD in place, running the tool, and restoring the change; the
file's MD5 on this tree is `7e7bce57294311e93cd662be2a5570a9` (section 2).

**clang-tidy, test file.** The same command reported 790 findings for the changed file against 788 at HEAD -
**both counts are not retained**, no clang-tidy output was kept. That run attributed the two added ones to
classes the file already carries: one more `25 is a magic number` for `borderFilter.SetMaxLevel(25)` (the
identical call stands at `Tests/src/OpenGLAreaVisibilityTest.cpp:217`) and one more C-style cast for
`(int)viewportSize` (the neighbouring painter case casts the same way, `:426`). It reported one
`readability-function-cognitive-complexity` finding at HEAD that the run on the changed file lacked; the class
is present in both runs and no added line is attributable, so it is recorded as a run difference rather than
a fix.

**uncrustify.** `evidence/uncrustify-diff-hunks.txt` carries the hunk counts of the diff
`scripts/format-check.sh diff` prints (`uncrustify -c .uncrustify -l CPP -f <file> | diff -u <file> -`; the
verdict `scripts/format-check.sh check` computes, `uncrustify --check`, prints no hunks at all):

| file | hunks at HEAD | hunks with the change |
|---|---|---|
| `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` | 47 | 47 |
| `Tests/src/OpenGLAreaVisibilityTest.cpp` | 12 | 17 |

The production file keeps its HEAD hunk count. The test file gains three hunks (one for the pointer spacing of
the new `GLFWwindow *CreateOffscreenContext()`, two for the declaration alignment of the new case's body);
the other two new positions are the HEAD hunks that the insertion moved apart. **Neither file can be made
clean under this configuration** (47 and 17 hunks, above). The demands are pointer-star spacing
(`sp_before_ptr_star = remove`, e.g. `GLFWwindow*CreateOffscreenContext()`) and padding of consecutive
declarations. This is the `TODO.md` §8 class, measured for these two files; the added code follows the
file-local style of its neighbours.

## 6. Findings this change does not fix (reported, not fixed)

1. The border loop below the decision draws every resolved border style with **one** width, the width of the
   ring's first border style, and derives `0.0` when that style carries an offset or a display offset
   (`MapPainterOpenGL.cpp:263-273`, `:379-424`). A border-only ring of such a style sheet is therefore drawn
   with a width of `0.0` - the same "first border style" shape `TODO.md` §99 records for the visibility
   tolerance, in the drawing loop instead of the decision. Not fixed here: §99 owns the tolerance and this
   change must not widen.
2. A filled ring whose triangulation throws still leaves the ring loop at the `catch` before the border
   loop, so that ring loses its border as well (`MapPainterOpenGL.cpp:347-350`). Pre-existing; the border of
   such a ring is a separate contract from the fix's.

## 7. Reviewer pointers

- The kept counter is the case's observable: `keptRingCount` is documented as "kept and whose geometry it
  went on to prepare", and the drawing half rests on the guard putting the border loop back in reach of
  every kept ring - the hunk to check is `MapPainterOpenGL.cpp:313-317` and the border loop that follows it.
- `evidence/red-head-*` is the only artifact of the pre-fix state; `evidence/probe-*` is the mutation
  evidence; `evidence/gate-*` is the gate; `evidence/uncrustify-diff-hunks.txt` is the formatter measurement
  of section 5.
- The case skips - it never fails - when no offscreen GL context can be created, as the neighbouring painter
  case does.
