# Tasks

Spec: `specs/ci-build-fixes/spec.md` (delta to the `ci-build-fixes` capability). Design: `design.md`.

## 1. Baseline

- [x] 1.1 Record the outcome of the MSYS runs already queued for the `client-style-load-resilience` commits (started 18:59 UTC) in `design.md` - Open Questions, so the change states whether the drift persisted. Verify: the two run conclusions and the failing test names (if any) are written down.
- [x] 1.2 Confirm the current MSYS failure signature on master (`35638602673`): `MapPainterShieldTest` fails, `PerformanceTest-cairo-{standard,winter-sports,cycle}` fail in the meson job, and each failing test log contains `couldn't load font ... falling back to`. Verify: the quoted lines are reproduced from the run log. (Spec: "MSYS jobs provide the font the font-dependent tests measure against".)

## 2. Font provisioning and verification

- [x] 2.1 Add a font-provisioning step to both jobs in `.github/workflows/build_and test_on_msys.yml`, after the MSYS2 setup and before the build, that makes the font family the tests name resolve from repository content. First attempt: copy the bundled file into the MSYS2 prefix font directory and refresh that directory's cache; the first CI run showed that fontconfig does not index that directory, so task 2.3 replaced the mechanism. Verify: the step appears in both jobs in the workflow diff and the workflow parses as YAML. (Spec: "MSYS jobs provide the font the font-dependent tests measure against", "MSYS font provisioning uses only repository content and the existing package setup".)
- [x] 2.2 Add a verification step to both jobs, before the build, that prints the file the font family resolves to, the number of fonts visible, the resolution of the generic `sans-serif` family and the locale in effect, and that fails when the family does not resolve to the bundled font file. Verify: the step appears in both jobs, and for a deliberately wrong family name the step fails with the resolved file shown - exercised by a temporary local run of the same command sequence, or by reading the first CI run's output. (Spec: "MSYS jobs verify the font and locale environment before running tests".)
- [ ] 2.3 Read the verification output of the first CI run on the branch and decide the mechanism: the run at 2026-09-21 19:34 UTC printed `Liberation Sans resolves to: C:/Windows/fonts/arial.ttf`, so the prefix copy is not indexed; the mechanism was replaced with the `design.md` - D2 fallback, a `conf.d` snippet (`$MINGW_PREFIX/etc/fonts/conf.d/99-libosmscout-fonts.conf`) naming the repository's font directory as a native path, plus `fc-cache -f` on it, and the snippet's XML was checked against a local fontconfig. Remaining: the verification step must pass in both jobs. Verify: the decision and the observed resolved file are recorded in `design.md` - D2 and Open Questions, and the verification step passes in both jobs. (Spec: "Named family resolves to the repository font".)

## 3. Locale

- [x] 3.1 Set `LANG` and `LC_ALL` on the `gcc and cmake` test step to the values the `gcc and meson` test step already uses. Verify: both test steps in the workflow declare the same locale value (`grep` the workflow). (Spec: "MSYS test steps run with an explicit locale".)

## 4. Build and test verification (run after group 6, which the font tests' outcome depends on)

- [ ] 4.1 Confirm the workflow is valid and the cmake job still configures, builds and runs its test set: the `gcc and cmake` MSYS job on the branch completes green, or, if it fails, the failure is not in a font- or locale-dependent test. Verify: the run's conclusion and its test summary.
- [ ] 4.2 Confirm the meson job's test set passes: the `gcc and meson` MSYS job on the branch reports `Fail: 0`, including `Check MapPainterShield compilation` and the `PerformanceTest-cairo-*` cases that fail today. Verify: the run's summary of failures.
- [x] 4.3 Confirm the CMake and Meson build systems are unaffected: no source, `CMakeLists.txt` or `meson.build` file is modified, so both build systems compile as before. Verify: `git diff --name-only` lists exactly `.github/workflows/build_and test_on_msys.yml` (plus the change's own artifacts and documentation files).
- [x] 4.4 Confirm no unit test is required for this change: the change contains no new or modified code, only workflow configuration, so no new Catch2 test is added; the existing suites are the verification (tasks 4.1 and 4.2). Verify: the diff contains no file under `Tests/`, `libosmscout*/` or `OSMScout*/`.
- [ ] 4.5 Confirm no other workflow is affected: the Ubuntu, macOS, sanitizer, VS, Android, iOS and JavaScout jobs are unchanged and no longer need the local workaround. Verify: `git diff --stat` against the base revision shows a single workflow file, and one non-MSYS job of the branch is green for completeness (for example `cmake` on Ubuntu).

## 5. Documentation and follow-ups

- [x] 5.1 Record the residual in `TODO.md`: the Cairo backend treats a font file path as a font family name (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp`, `pango_font_description_set_family`), which is why a host font change can move the results of `Tests/src/MapPainterShieldTest.cpp` and `Tests/src/PerformanceTest.cpp`; note that a real fix belongs in its own change. Verify: the entry names both files and the reason.
- [x] 5.2 Add a one-line note to the MSYS row of the CI/CD table in `AGENTS.md` stating that the MSYS jobs provision the repository's Liberation font and verify the font and locale environment before the tests. Verify: the note exists and matches the workflow's step names.
- [x] 5.3 Validate the change: `openspec validate msys-ci-font-environment --strict`. Verify: the command reports the change as valid.

## 6. The font-dependent tests own their font

Implements `specs/font-dependent-test-fonts/spec.md`. Rationale and rejected alternatives: `design.md` - D6.

- [x] 6.1 Make a font family resolvable from a font file available to the tests: reuse the existing reference helper that reads the family out of a font file (`Tests/src/TextMetricsCairoTest.cpp` and `Tests/src/TextMetricsReferenceTest.cpp` use `TextMetricsAll::ReferenceFontFamily`) or add the missing piece of test support. Verify: `MapPainterShieldTest` and `PerformanceTest` both compile against it in the CMake and the Meson build. (Spec: "The font-dependent tests measure the repository font".)
- [x] 6.2 Change `Tests/src/MapPainterShieldTest.cpp` to take the family from the repository font file instead of the literal `Liberation Sans`, register that file with the process font configuration when font configuration is available, and select the font-configuration Pango backend when one is available and the environment has not selected one. Verify: the test passes with a font configuration that lists only the repository font directory, and also with a font configuration that lists no font at all (which passes on Linux today and fails on Windows); and it compiles without font configuration. (Spec: "The font file is resolvable to the measured text stack", "Measured font does not move with the host font set".)
- [x] 6.3 Change the Cairo path of `Tests/src/PerformanceTest.cpp` to resolve the `--font` file to a family name before setting it on the `MapParameter`, keeping the file path for the AGG and OpenGL drivers, and to report an unreadable font file instead of measuring another font. Verify: the Cairo runs print no `couldn't load font ... falling back to` line; the AGG and OpenGL runs still receive the path. (Spec: "A font file argument is resolved to a family name for family-based backends".)
- [x] 6.4 Confirm the two tests no longer depend on a host-installed family: run them in a build with a font configuration that lists only the repository font directory and confirm the same metric assertions pass, and that the shield test's centring assertions hold. Verify: the commands and their results are recorded in this file. (Spec: "Measured font does not move with the host font set".)

### Recorded verification of group 6

Local CMake build (`Release`, `build/`) and local Meson build (`build-meson/`), 2026-09-21:

```
cmake --build build --target MapPainterShieldTest PerformanceTest   -> links, no new warning
meson compile -C build-meson MapPainterShieldTest PerformanceTest   -> links, no new warning

ctest -R MapPainterShieldTest                                        -> 100% passed (default font configuration)
FONTCONFIG_FILE=<config with only the repo font dir> ctest -R MapPainterShieldTest -> 100% passed
FONTCONFIG_FILE=<config naming an empty font dir>    ctest -R MapPainterShieldTest -> 100% passed
ctest -R PerformanceTest-cairo-standard -V                           -> passed, no 'couldn't load font' line
FONTCONFIG_FILE=<config naming an empty font dir> ctest -R PerformanceTest-cairo-standard -> passed
meson test -C build-meson 'Check MapPainterShield compilation' 'Check TextMetricsCairo compilation' 'Check TextMetricsReference compilation' -> Ok: 3, Fail: 0
ctest -R 'MapPainterShieldTest|TextMetricsCairoTest|TextMetricsReferenceTest|TextMetricsSVGTest' -> 100% passed
```

Mechanism of `Tests/src/TestFontSupport.h` proven directly with a probe built against the same Pango/fontconfig in a font configuration that knows no fonts at all:

```
FONTCONFIG_FILE=<empty font dir> probe <repo font> 'Liberation Sans'            -> RESULT font=NULL
FONTCONFIG_FILE=<empty font dir> probe <repo font> 'Liberation Sans' register   -> RESULT resolved=Liberation Sans
```

So the tests measure the repository font even when the host provides no font at all, which is the state the MSYS jobs were in.
