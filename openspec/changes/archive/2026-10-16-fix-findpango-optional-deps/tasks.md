# Tasks

Parent spec: `ci-build-fixes` — "A dependency the backends feature-gate is not a configure requirement".

This change compiles no product source: the delta is two removed `REQUIRED` keywords in a CMake find
module plus a comment. It adds no invariant beyond the symptom, so it has no probe, and its gate is the
configure matrix in section 3, not the test suite (`gate-class.sh` reports no committed delta and
instructs to classify the WIP by hand; the roster would call a `.cmake` file product code, but no
compiled file is in the delta, so a suite run carries no signal for it - see
`.pi/skills/verify-build-and-tests/SKILL.md` step 0, "the file's own check").

## 1. Red case on the unmodified module

- [x] 1.1 Run `cmake -B /tmp/101-red-freetype -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON` and retain the log. Verify: exit 1 at `2026-10-16T10:50:19Z` with `CMake Error at cmake/FindPango.cmake:112 (FIND_PACKAGE): FIND_PACKAGE for module Freetype called with REQUIRED, but CMAKE_DISABLE_FIND_PACKAGE_Freetype is enabled`; the run does not set `-DOSMSCOUT_BUILD_MAP_OPENGL=OFF`, so the same log goes on to report `-- Found Fontconfig` and then fails a second time at `CMakeLists.txt:341` (`freetype2 is required for OpenGL map drawing backend`), ending `-- Configuring incomplete`; log retained as `evidence/red-configure-disable-freetype.log`.
- [x] 1.2 Run `cmake -B /tmp/101-red-fontconfig -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON` and retain the log, because 1.1 disables only FreeType and its log does not name the Fontconfig call. Verify: exit 1 at `2026-10-16T10:50:34Z` with the same error naming `cmake/FindPango.cmake:116` and `Fontconfig`; log retained as `evidence/red-configure-disable-fontconfig.log`.

## 2. The fix

- [x] 2.1 Drop `REQUIRED` from `FIND_PACKAGE(Freetype ...)` and `FIND_PACKAGE(Fontconfig ...)` in the pangoft2 branch of `cmake/FindPango.cmake`, so a disabled or absent package leaves the `IF(<NAME>_FOUND)` guards to skip the include directories, and add the comment that both are optional and feature-gated (spec: the requirement). Verify: `grep -n 'REQUIRED' cmake/FindPango.cmake` finds no `Freetype`/`Fontconfig` call left, and the file's remaining `FIND_PACKAGE(...REQUIRED)` calls are the pre-existing `GLIB2`/`GObject` ones outside this defect's scope.

## 3. The configure matrix (the gate)

- [x] 3.1 Scenario "A configure with FreeType disabled completes": `cmake -B /tmp/101-a-nofreetype -DCMAKE_BUILD_TYPE=Release -DOSMSCOUT_BUILD_MAP_OPENGL=OFF -DCMAKE_DISABLE_FIND_PACKAGE_Freetype=ON`. Verify: exit 0 at `2026-10-16T10:51:17Z`, and `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FREETYPE` and `OSMSCOUT_MAP_CAIRO_HAVE_LIB_CAIRO_FT` undefined in the configured `MapCairoFeatures.h` (`evidence/green-configure-disable-freetype.log`, `evidence/green-features-disable-freetype.h`). `-DOSMSCOUT_BUILD_MAP_OPENGL=OFF` is required because that backend needs FreeType itself (`CMakeLists.txt:341`); the reason is recorded in `design.md`.
- [x] 3.2 Scenario "A configure with the font configuration disabled completes": `cmake -B /tmp/101-b-nofontconfig -DCMAKE_BUILD_TYPE=Release -DCMAKE_DISABLE_FIND_PACKAGE_Fontconfig=ON`. Verify: exit 0 at `2026-10-16T10:51:28Z`, `OSMSCOUT_MAP_CAIRO_HAVE_LIB_FONTCONFIG` undefined (`evidence/green-configure-disable-fontconfig.log`, `evidence/green-features-disable-fontconfig.h`).
- [x] 3.3 Scenario "A configure that leaves both enabled still finds both": `cmake -B /tmp/101-c-normal -DCMAKE_BUILD_TYPE=Release`. Verify: exit 0 at `2026-10-16T10:51:40Z`, the log reports `Found Freetype: /usr/lib/libfreetype.so (found version "2.14.3")` and `Found Fontconfig: /usr/lib/libfontconfig.so (found version "2.18.3")`, and both gates are defined in the configured header (`evidence/green-configure-default.log`, `evidence/green-features-default.h`).
- [x] 3.4 Scenario "The Cairo backend compiles with either dependency disabled": `cmake --build /tmp/101-a-nofreetype --target OSMScoutMapCairo -j 16` and `cmake --build /tmp/101-b-nofontconfig --target OSMScoutMapCairo -j 16`. Verify: both exit 0 with no `error:`/`warning:` line - `2026-10-16T10:51:50Z` to `10:53:13Z` and `10:53:16Z` to `10:54:43Z` (`evidence/green-build-disable-freetype.log`, `evidence/green-build-disable-fontconfig.log`).

## 4. Structural and cross-build-system checks

- [x] 4.1 Confirm the other build system needs no equivalent change: `libosmscout-map-cairo/meson.build:31` and `:35` add FreeType and fontconfig under `if ftDep.found()` / `if fontconfigDep.found()`, and both are resolved optionally at the root (`meson.build:182` `dependency('freetype2', required: false)`, `:214` `dependency('fontconfig', required : false)`), so Meson already treats them as optional. Verify: the four `grep` lines above; no Meson file is edited by this change.
- [x] 4.2 Confirm no CI workflow relies on the hard requirement: `grep -rn 'CMAKE_DISABLE_FIND_PACKAGE' .github/workflows/` finds nothing, so
  no job configures with a dependency switched off; the jobs that build the Cairo backend install
  `libfreetype6-dev` and the font configuration instead. Verify: the grep is empty; no workflow is edited, and the `vcpkg_minimum.json` profile that ships neither package is recorded in the change's report as a follow-up observation, not as a CI edit.
- [x] 4.3 Formatter/consistency of the touched file: CMake has no uncrustify configuration, so check what applies instead - the file's own indentation (four spaces inside `IF()`, matching the surrounding block), no trailing whitespace in the added lines, and that CMake parses the module - the latter is proven by the four configures of section 3, each of which `include()`s it through `cmake/features.cmake:257`. Verify: `grep -n ' $' cmake/FindPango.cmake` is empty, `git diff` of the file touches two lines plus a comment, and the configures of 3.1-3.3 exit 0.

## 5. Backlog bookkeeping

- [x] 5.1 After archive, delete the §101 entry from `TODO.md` (this run was started with removal allowed) and re-run the structure check: `grep -c '^- \*\*'` equals `grep -c '^\s*\*\*id:\*\*'` and no `§id` collides. Verify: the counts read 90/90 before and 89/89 after; the `**Clusters**` index still carries the deleted entry's line, which `cleanup-todo` regenerates.
