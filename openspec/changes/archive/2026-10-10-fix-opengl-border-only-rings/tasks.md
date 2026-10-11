# Tasks

## 1. The case that reproduces the defect

- [x] 1.1 In `Tests/src/OpenGLAreaVisibilityTest.cpp`, add `MakeBorderOnlyStyleConfig` (an area type drawn by one border style and by no fill style) and the case "A ring drawn only by a border is kept": it loads two areas in view, one of the border-only type and one of a type the style sheet does not mention, prepares the frame through an offscreen painter and asserts `CHECK(painter.GetExaminedRingCount()==1)` and `CHECK(painter.GetKeptRingCount()==1)`. Verify (spec: `map-painter-area-preparation` - A ring a style sheet draws by a border is kept, both scenarios): on the unmodified painter the case fails with `CHECK( painter.GetKeptRingCount()==1 )` and expansion `0 == 1`, run of 2026-10-10 12:00 UTC reporting `test cases: 6 | 5 passed | 1 failed` and `assertions: 116 | 115 passed | 1 failed`; the log is `evidence/red-head-ctest-LastTest.log` and the single-case run `evidence/red-head-case-console.txt`.
- [x] 1.2 Extract the offscreen context of the painter cases into `CreateOffscreenContext()` and use it in both cases, so the file has one spelling of the "create an invisible core-profile context, skip if it fails" step. Verify: the run of 1.1 reports the same tally with the pre-existing painter case still passing (`test cases: 6 | 5 passed | 1 failed`).

## 2. The fix

- [x] 2.1 In `MapPainterOpenGL::ProcessAreas`, replace `if (!fillStyle) continue;` with a guard that makes the fill path conditional on the resolved fill style, keep `keptRingCount` at the point where the step commits to prepare the ring's geometry, and leave the border loop as the single consumer of that geometry for every ring the step kept. Verify (spec: both scenarios of the added requirement): `grep -n "if (!fillStyle)" libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` finds no occurrence; the case of 1.1 passes; both builds compile without a new warning in the touched file.

## 3. The scenarios of the added requirement

- [x] 3.1 Scenario "A ring a style sheet draws by a border only is kept" - case "A ring drawn only by a border is kept", assertion `CHECK(painter.GetKeptRingCount()==1)`. Verify: `ctest -R OpenGLAreaVisibilityTest --output-on-failure` in `build/` passes and `meson test -C build-meson --print-errorlogs "Check OpenGL area visibility"` passes.
- [x] 3.2 Scenario "A ring a style sheet draws by nothing is still not prepared" - the same case, assertion `CHECK(painter.GetExaminedRingCount()==1)`. Verify: the assertion is green, and the probe of 4.2 makes it red for the reason it guards.

## 4. Probes and the gate

- [x] 4.1 Probe (`TEMP-REVERT-PROBE`): reinsert the defect as `if (!fillStyle) { continue; }` in front of the fill guard, rebuild, and require the case to fail with `CHECK( painter.GetKeptRingCount()==1 )` and expansion `0 == 1`; then remove the mutation and require the case green again. Verify: the failing and the restored run are in `evidence/`, `grep -rn TEMP-REVERT-PROBE Tests/ libosmscout-map-opengl/` is empty, and `git diff libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` shows only the fix.
- [x] 4.2 Probe (`TEMP-REVERT-PROBE`): relax the style guard of the ring loop so a ring that resolves neither a fill nor a border style is no longer rejected, rebuild, and require the case to fail with `CHECK( painter.GetExaminedRingCount()==1 )` and expansion `2 == 1`; then restore and require the case green again. Verify: the failing run is in `evidence/probe-2-style-guard-relaxed-cmake.txt`, the tree carries no marker, and the gate run of 4.3 shows the restored tree green. This is the assertion of 3.2, which guards behaviour that already held before this change, so it gets a mutation instead of a reverted fix.
- [x] 4.3 Forced gate for this change shape: `cmake --build build -j "$(nproc)"` followed by the CMake suite with `QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests" ctest -j 2 --output-on-failure`, and `meson compile -C build-meson` followed by `meson test -C build-meson --timeout-multiplier 2 --print-errorlogs`. Verify: both suites report no failure this change introduced, the two logs are copied into `evidence/` before any other run, and the tally and timestamps of both runs are in `verification.md`.

## 5. Conventions

- [x] 5.1 Run `uncrustify -q -c .uncrustify -l CPP -f` on `Tests/src/OpenGLAreaVisibilityTest.cpp` and `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp` and `clang-tidy -p build` on the same files, and record what each reports for the change against the same files at HEAD. Verify: clang-tidy reports the same finding count for the production file as at HEAD (**not retained**, `verification.md` §5 records how the comparison was taken), and the formatter's hunk count does not grow for the production file (47 before and after); the test file's five added hunks are the pointer spacing of the helper and the declaration alignment of the new case, so `scripts/format-check.sh check` cannot be satisfied for either file (TODO §8 class) - the measured hunk counts and that consequence are in `verification.md` §5 instead of a clean verdict.

## Workflow follow-up

- Archive the change after the independent review, and mark `TODO.md` §98 `fixed-by fix-opengl-border-only-rings`. This iteration does not touch `TODO.md` or archive.
- File the two findings this change does not fix as new `TODO.md` entries instead of fixing them here: the border width of the drawing loop derives from the ring's first border style only (so a border-only ring whose first border style carries an offset is drawn with width `0.0`), and a filled ring whose triangulation throws still leaves the ring loop before its border loop.
