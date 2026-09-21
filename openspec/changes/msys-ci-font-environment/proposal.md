## Why

Between 2026-09-21 16:27 and 18:05 UTC every branch's Windows/MSYS run turned red without a repository change: the font-dependent tests (`MapPainterShieldTest`, the three `PerformanceTest-cairo-*` cases) now render with a different fallback font than the one the tests measure against, so their metric assertions fail. The MSYS jobs install no font at all, so those tests measure against whatever font the rolling MSYS2 package set and the runner image happen to provide that day — while the repository already ships the font the tests refer to. Every earlier and later MSYS run of the same commits (and of PR #1827, which touches only the JNI POI result and two Java test files) failed identically, and the failure message does not name the font that was actually used.

## What Changes

- Both MSYS jobs SHALL make the font family that the font-dependent tests name resolvable from the font file the repository already ships, so those tests no longer measure against a font the runner environment happens to provide.
- Both MSYS jobs SHALL verify, before running the tests, that the required font family resolves and that a locale is available, and SHALL fail that verification step with the resolved font and locale information when they do not.
- The `gcc and cmake` test step SHALL run with the same explicit locale environment as the `gcc and meson` test step, instead of inheriting whatever locale the MSYS2 environment provides.
- The provisioning and verification SHALL be part of the existing setup, adding no new external service or secret.

## Capabilities

### New Capabilities

<!-- None. The requirements land in the existing CI-hardening capability. -->

### Modified Capabilities

- `ci-build-fixes`: adds requirements that the MSYS jobs provide the font the font-dependent tests measure against, verify the font and locale environment before running tests, and run the tests with an explicit locale.

## Impact

- `.github/workflows/build_and test_on_msys.yml` — both jobs (`gcc and cmake`, `gcc and meson`) gain a font- and locale-verification step and font provisioning; the `gcc and cmake` test step gains the locale environment.
- `libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf` — used as the provisioned font; not modified.
- `Tests/src/MapPainterShieldTest.cpp`, `Tests/src/PerformanceTest.cpp` — not modified; they must pass on MSYS again after the environment is deterministic.
- No source code, public API, database format, style sheet or dependency change.
- Not in this change, and recorded for a follow-up: the Cairo backend treats a font *file* path as a font *family* name (`libosmscout-map-cairo/src/osmscoutmapcairo/MapPainterCairo.cpp`), so a test that passes a font path silently measures a fallback font. That is the underlying reason a host-font change can move these test results at all; fixing it changes library behaviour and belongs in its own change.
