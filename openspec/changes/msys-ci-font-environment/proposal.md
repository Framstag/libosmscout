## Why

Between 2026-09-21 16:27 and 18:05 UTC every branch's Windows/MSYS run turned red without a repository change: the font-dependent tests (`MapPainterShieldTest`, the three `PerformanceTest-cairo-*` cases) now render with a different fallback font than the one the tests measure against, so their metric assertions fail. The MSYS jobs install no font at all, so those tests measure against whatever font the rolling MSYS2 package set and the runner image happen to provide that day — while the repository already ships the font the tests refer to. Every earlier and later MSYS run of the same commits (and of PR #1827, which touches only the JNI POI result and two Java test files) failed identically, and the failure message does not name the font that was actually used.

Making the font available in the environment is necessary but not sufficient: on Windows the pango/cairo stack resolves font families through its Win32 font collection by default and ignores fontconfig entirely, so a job that registers a font with fontconfig can still see the tests fall back (MSYS2 tracks this as issue #4293). The tests therefore also have to stop depending on which font family the host provides: one names a family no Windows host has, and the other hands a font *file path* to an interface that takes a family name.

## What Changes

- Both MSYS jobs SHALL make the font family that the font-dependent tests name resolvable from the font file the repository already ships, so those tests no longer measure against a font the runner environment happens to provide.
- Both MSYS jobs SHALL verify, before running the tests, that the required font family resolves and that a locale is available, and SHALL fail that verification step with the resolved font and locale information when they do not.
- The `gcc and cmake` test step SHALL run with the same explicit locale environment as the `gcc and meson` test step, instead of inheriting whatever locale the MSYS2 environment provides.
- The font-dependent tests SHALL take the font they measure from the font file the repository ships, instead of naming a host font family, and SHALL make that file resolvable to the text stack they measure through; today they name a family that no Windows host provides and pass a font *file path* to an interface that expects a family name.
- The two MSYS font-dependent tests SHALL no longer depend on the host's font set: `MapPainterShieldTest` and the Cairo runs of `PerformanceTest` SHALL measure the repository font on every platform that can resolve it.
- The provisioning and verification SHALL be part of the existing setup, adding no new external service or secret.

## Capabilities

### New Capabilities

- `font-dependent-test-fonts`: how the font-dependent tests choose and resolve the font they measure, so their results do not move with the fonts a CI runner happens to provide.

### Modified Capabilities

- `ci-build-fixes`: adds requirements that the MSYS jobs provide the font the font-dependent tests measure against, verify the font and locale environment before running tests, and run the tests with an explicit locale.

## Impact

- `.github/workflows/build_and test_on_msys.yml` — both jobs (`gcc and cmake`, `gcc and meson`) gain a font- and locale-verification step and font provisioning; the `gcc and cmake` test step gains the locale environment.
- `Tests/src/MapPainterShieldTest.cpp` — takes its font family from the repository font file instead of the literal `Liberation Sans`, and makes that file resolvable to the text stack it measures through.
- `Tests/src/PerformanceTest.cpp` — the Cairo driver receives a font family resolved from the `--font` file instead of the file path; the AGG and OpenGL drivers keep the path (they load the file directly).
- Test support for resolving a family name from a font file, shared with the existing `TextMetricsAll` reference (`Tests/src/TextMetricsCairoTest.cpp`, `Tests/src/TextMetricsReferenceTest.cpp`).
- `libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf` — used as the provisioned font; not modified.
- No public API, database format, style sheet or dependency change.
- Not in this change, and recorded for a follow-up: the Cairo backend treats a font *file* path as a font *family* name (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp`), so an application that hands it a path silently paints a fallback font. This change stops the tests from relying on that, but leaves the library behaviour alone; fixing it changes rendering behaviour and belongs in its own change.

