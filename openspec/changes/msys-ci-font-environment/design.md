# Design

## Context

See `proposal.md` - Why for the failure. The design-relevant current state:

**The MSYS workflow.** `.github/workflows/build_and test_on_msys.yml` has two jobs, `gcc and cmake` and `gcc and meson`, both on `runs-on: windows-2025` with `msys2/setup-msys2@v2` (defaults `release: true`, i.e. a fresh install from the latest MSYS2 installer) and `update: true` (full rolling package upgrade). The install lists carry compiler, cmake/meson, cairo, pango, qt5, skia and directx-headers — and no font package. The cmake job runs `ctest -j 4 --output-on-failure --exclude-regex PerformanceTest`; the meson job runs `meson test --timeout-multiplier 2` with `LANG: en_US.utf8` and `LC_ALL=C` on the command line. Only the meson job sets a locale at all.

**How the tests consume fonts.**

```
Tests/src/MapPainterShieldTest.cpp:89      parameter.SetFontName("Liberation Sans")   <- family name
Tests/meson.build:561                      '--font', <repo>/libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf
Tests/CMakeLists.txt (performance tests)   same file path as the --font argument
Tests/src/PerformanceTest.cpp:939          drawParameter.SetFontName(args.font)       <- argument untouched
libosmscout-map-cairo/.../MapPainterCairo.cpp:317
                                           pango_font_description_set_family(font, parameter.GetFontName())
```

So on the Cairo backend both shapes arrive as a font *family*: the shield test asks for a family that no Windows host provides, and the performance tests ask for a family literally spelled like a path. Both render the fallback font. The tests pass as long as that fallback's metrics satisfy their tolerances — `MapPainterShieldTest.cpp:223-224` compares ink centre and background centre with `margin(2.0)` pixels.

**Why the drift is invisible today.** `meson test` and `ctest` print a passing test's output only in the log file, so the `couldn't load font "..." falling back to "Sans"` warning that these tests always produce is never shown in a green run. In the red runs it is the only clue, and it names neither the font that was resolved nor the locale that was in effect.

**The signature, quoted from the master push `35638602673` (18:28 UTC).** `gcc and cmake`: `99% tests passed, 1 tests failed out of 81`, `67 - MapPainterShieldTest (Failed)`, with `couldn't load font "Liberation Sans Not-Rotated 37.795px", falling back to "Sans Not-Rotated 37.795px", expect ugly output.` `gcc and meson`: `58/114 ... PerformanceTest-cairo-standard.oss FAIL`, `59/114 ... winter-sports.oss FAIL`, `64/114 ... cycle.oss FAIL` and `108/114 ... Check MapPainterShield compilation FAIL`, each preceded by `couldn't load font "D:/a/libosmscout/libosmscout/Tests/../libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf Not-Rotated 7.669px", falling back to "Sans Not-Rotated 7.669px", expect ugly output.`

**The drift is persistent, not transient.** The two `client-style-load-resilience` MSYS runs queued at 18:59 UTC and finished at 19:12 and 19:14 UTC fail identically to master (`1 of 82` in the cmake job with `68 - MapPainterShieldTest (Failed)`; `4 of 115` in the meson job with the three `PerformanceTest-cairo-*` cases and `Check MapPainterShield compilation`). Those branches do not touch the MSYS environment. A third run of that batch failed earlier, inside `msys2/setup-msys2` itself, with `Unexpected HTTP response: 500` while downloading the MSYS2 installer - an unrelated upstream flake, not a font or locale failure.

**What actually changed between the last green and the first red run** (master push 16:27 green, 18:05 red, no repository change in between): fontconfig 2.18.3-1, freetype 2.14.3-1 and pango 1.58.2-1 identical; cairo 1.18.4-4 → 1.18.6-1 and the mingw-w64 crt/headers `14.0.0.r409` → `14.0.0.r420`. This is not conclusive about the cause, which is exactly why the change adds a verification step rather than assuming one.

**Constraint.** MSYS2 ships no plain Liberation package: the only Liberation artefacts are nerd-patched Liberation *Mono* files under `mingw-w64-nerd-fonts`, a different family name. The repository already ships `libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf`, which every Ubuntu and macOS job obtains by installing `fonts-liberation`.

## Goals / Non-Goals

**Goals:**

- The two MSYS jobs measure font-dependent tests against the font the repository ships, not against a font the runner image happens to provide.
- A broken font or locale environment fails in its own named step, before the build/test, and prints the font the family resolved to and the locale in effect.
- Both MSYS test steps use one explicit locale value instead of relying on the environment.
- All of it stays inside the existing job setup: repository content, no new secret, no new service, seconds of added run time.

**Non-Goals:**

- Changing the Cairo backend so that a font *file* path is loaded as a file. That is the underlying reason a host-font change can move these results, and it changes library behaviour — its own change (recorded in `TODO.md`).
- Pinning MSYS2 package versions or the runner label; the rolling upgrade stays.
- The other red CI signals seen in the same window: the flaky `Check MapDownloadService APIs` abort in the JavaScout `meson + maven` job, and the two 30 s meson test timeouts in the Sonar job.
- Changing `MapPainterShieldTest`'s tolerance or the performance tests' font arguments.

## Decisions

### D1: Provision the repository's own font file

**Chosen:** copy `libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf` into the MSYS2 prefix's font directory before the tests run.

Alternatives:

- **Install an MSYS2 font package** — rejected. There is no plain Liberation package; `mingw-w64-nerd-fonts` installs a large set of nerd-patched fonts whose family names differ from "Liberation Sans", so it would not satisfy the family the tests name.
- **Depend on the runner image's fonts** — rejected; that dependence is the failure mode being fixed.
- **Change the tests to stop needing that family** — out of scope (Non-Goals): it changes test and library behaviour, and the two failing tests are meant to measure a real font.

Risk of the chosen option: it only works if fontconfig scans the directory it is copied into (see D2).

### D2: Put the font where fontconfig looks, and verify rather than assume

**Chosen (as implemented):** register the repository's font directory with the packaged font configuration through a snippet in `${MINGW_PREFIX}/etc/fonts/conf.d/`, naming the directory as a native Windows path (`cygpath -m`), then refresh that directory's font cache. The packaged `fonts.conf` includes `conf.d`, so nothing is copied and no environment variable is needed, and the Windows font directories stay in place.

**First attempt, and why it was replaced:** copy the bundled file into `$MINGW_PREFIX/share/fonts/TTF` and refresh that directory's cache. The first run of this change (MSYS on PR #1829, 19:34 UTC) had the verification step report `Liberation Sans resolves to: C:/Windows/fonts/arial.ttf`: the copy succeeded, but that fontconfig build indexes what its configuration names, so a directory the configuration does not list stays invisible however often it is cached. This is the fallback the alternatives below describe, not a different approach - the requirement, that the family resolves from repository content, is unchanged.

Alternatives:

- **Copy into the prefix and assume it is scanned** — rejected: `MINGW-packages` issue #5762 reports fonts placed under `/mingwXX/share/fonts` not being found by that fontconfig packaging, so this cannot be assumed.
- **Generate a restricted font configuration from the start** — viable and fully deterministic, but it hides the host's fonts from every font-dependent test in the job (`TextMetricsCairoTest` asserts on the family fontconfig resolves), so it is kept as the fallback rather than the first move.
- **Set the font path as an environment variable that the tests read** — requires changing the tests and adds a new configuration surface for one platform; rejected.

Risk of the chosen option: one round trip through CI to learn whether the directory is scanned. Mitigation: the verification step's output names the resolved file, and the fallback needs no spec change — only which mechanism provides the resolution.

### D3: Verify in a step of its own, before the build

**Chosen:** a named verification step per job, placed after the MSYS2 setup and before the build, that fails when the family does not resolve to the repository font and always prints the resolved font file, the number of fonts the environment sees, the resolution of the generic `sans-serif` family, and the locale in effect.

Alternatives:

- **A guard inside the test step** — cheaper, but the failure then reads as a build/test failure, which is the confusion this change removes, and it runs after ~10 minutes of compiling.
- **Rely on the failing test's message** — insufficient: today's message names the requested family but not the font used, and it appears after the build.
- **A separate verification job** — a third runner and a duplicated setup for a two-second check; rejected.

Risk: the step duplicates the font environment check in both jobs (they set up separately). Accepted — they are independent jobs and the check is two seconds.

### D4: Use the meson job's locale in the cmake job too

**Chosen:** set `LANG` and `LC_ALL` to the values the `gcc and meson` job already uses, on both test steps.

Alternatives:

- **Leave the locale unset in the cmake job** — current state; `Locale::ByEnvironmentSafe()` (`libosmscout/include/osmscout/util/Locale.h:124-132`) logs that the environment locale could not be obtained and silently falls back.
- **Set a full UTF-8 locale in both jobs and drop `LC_ALL=C`** — more faithful to a real environment, but it changes a job that is currently green and depends on that locale being present in the image; rejected in favour of consistency with the existing working configuration.

Risk: `en_US.utf8` must exist in the MSYS2 environment for the value to have the intended effect; the verification step prints the locale, and if the value is absent the meson job's long-standing green state shows it is at least not harmful.

### D5: Keep the rolling package upgrade

**Chosen:** leave `update: true` (and `release: true`) as they are.

Alternatives:

- **`update: false` with the installer's frozen package set** — installing the long package list against a stale database is itself a partial upgrade, which MSYS2 explicitly does not support, and it would not have helped if the drift came from the runner image.
- **Pin package versions** — reproducible, but needs the packages hosted, blocks security updates, and is out of proportion for a workflow whose failure mode now fails fast and visibly.

Risk: the package set keeps moving, so this class of drift can recur. Mitigation: the verification step turns a recurrence into a five-second, self-describing failure, and provisioning (D1/D2) removes the font family from the moving part.

## Risks / Trade-offs

- **The family still does not resolve after provisioning** (fontconfig does not index the prefix font directory) → **materialised on the first run** as `C:/Windows/fonts/arial.ttf`; the verification step reported it after two minutes instead of after a build, and the D2 fallback closed it without a spec or task change.
- **Provisioning does not fix the failure** because the real cause is a changed renderer metric rather than font resolution (cairo 1.18.4-4 → 1.18.6-1 is the other candidate) → the verification step separates "environment is broken" from "renderer changed": if the preflight passes and the tests still fail, the cause is in the renderer and the tests' expectations need their own change. The first run of this change answers that question.
- **A later `pacman` invocation in the same job removes the copied file** → the copy happens after the setup step and the verification step immediately precedes the build; `--exclude-regex PerformanceTest` aside, no package operation runs afterwards.
- **A restricted fallback configuration hides host fonts** → include the Windows font directory in it, so that the generic families still resolve; the fallback is only used if the prefix directory turns out not to be scanned.
- **Added run time** → one `fc-cache` scoped to the added directory plus a two-second check, against a job that currently spends ~10 minutes compiling.
- **Duplicated setup in two jobs** → accepted; the two jobs already duplicate the whole MSYS2 setup.

## Migration Plan

Single workflow file, no state, no data, no API. Steps: apply the workflow change, push, and read the verification step's output on both jobs; confirm the font-dependent tests pass. Rollback is reverting the commit; nothing outside `.github/workflows/build_and test_on_msys.yml` changes, so a revert cannot leave a partial state.

## Open Questions

- Whether the prefix font directory is scanned by this fontconfig build - **answered at 2026-09-21 19:34 UTC: it is not.** With the bundled font copied to `$MINGW_PREFIX/share/fonts/TTF` and that directory cached, the verification step reported `Liberation Sans resolves to: C:/Windows/fonts/arial.ttf`; the mechanism was replaced as D2 describes, and the snippet's XML was checked against a local fontconfig before the next run.
- Whether MSYS re-runs of the already-queued commits (started 18:59 UTC) still fail - **answered at 2026-09-21 19:22 UTC: they still fail.** Runs `35641955424` (19:12) and `35641974854` (19:14) of `client-style-load-resilience` reproduce the master failures exactly (see Context), on commits that do not touch the MSYS environment. The drift is therefore persistent, and provisioning plus verification is the response; the change is not contingent on a transient upstream state.
