# Verification

The MSYS jobs can only be verified on a Windows runner with MSYS2, so the verification is this
change's own pull request: [#1938](https://github.com/Framstag/libosmscout/pull/1938), head
`ci/msys-ucrt64-migration`. Two runs of the `MSYS` workflow cover it: `45049676443` on commit
`1268b9e77` (the workflow and documentation change) and `45052006614` on commit `7f3c8edec` (the
spec delta and this file), both pushed 2026-11-22.

## The image the jobs were scheduled on

```
gcc and meson  Set up job  Image: windows-2025-vs2026
                           Image Release: https://github.com/actions/runner-images/releases/tag/win25-vs2026%2F20261116.315
```

Both jobs of both runs resolve the `windows-2025` label to that image, the same one the failing run
`44886294206` was scheduled on, so the difference between those runs and the failing one is the
environment the setup step selects and nothing else.

## The environment the setup step selects (tasks 1.3, 1.5, 3.1)

The setup step concludes `success` in both jobs. `gcc and cmake` reports 18 entries, all with the
`mingw-w64-ucrt-x86_64-` prefix:

```
  msystem: UCRT64
  install: mingw-w64-ucrt-x86_64-toolchain mingw-w64-ucrt-x86_64-libtool mingw-w64-ucrt-x86_64-cmake
           mingw-w64-ucrt-x86_64-ninja jq mingw-w64-ucrt-x86_64-catch mingw-w64-ucrt-x86_64-protobuf
           mingw-w64-ucrt-x86_64-libxml2 mingw-w64-ucrt-x86_64-nlohmann-json mingw-w64-ucrt-x86_64-cairo
           mingw-w64-ucrt-x86_64-pango mingw-w64-ucrt-x86_64-qt5 mingw-w64-ucrt-x86_64-glew
           mingw-w64-ucrt-x86_64-glfw mingw-w64-ucrt-x86_64-glm mingw-w64-ucrt-x86_64-libpng
           mingw-w64-ucrt-x86_64-skia mingw-w64-ucrt-x86_64-directx-headers
```

`gcc and meson` carries the same set with `mingw-w64-ucrt-x86_64-meson` in place of `cmake`. `pacman`
resolves the whole list ("Packages (131)" including the dependencies of the group packages) and
installs the two packages whose removal from MINGW64 caused the failure:

```
 mingw-w64-ucrt-x86_64-glfw-3.5.1-1-any downloading...
 installing mingw-w64-ucrt-x86_64-glfw...
 mingw-w64-ucrt-x86_64-skia-143.r77119.da51f0d6-5
```

Toolchain from that transaction: `mingw-w64-ucrt-x86_64-gcc-16.2.0-7`,
`mingw-w64-ucrt-x86_64-binutils-2.47-5`, `mingw-w64-ucrt-x86_64-qt5-base-5.15.19+kde+r96-2`,
`mingw-w64-ucrt-x86_64-pango-1.58.2-1`, `mingw-w64-ucrt-x86_64-libpng-1.6.59-2`,
`mingw-w64-ucrt-x86_64-directx-headers-1~1.619.5-1`. No `error: target not found` occurs in either
job.

## The steps that read the installation prefix (task 1.6)

They are unchanged by the diff and resolve `/ucrt64` without naming it, because they read
`MINGW_PREFIX`/`MINGW_CHOST`:

```
confDir="${MINGW_PREFIX}/etc/fonts/conf.d"                      (font step, both jobs)
DXSDK_DIR=${MINGW_PREFIX}/${MINGW_CHOST} cmake -B build ...       (cmake configure)
PATH=$PATH:$PWD/libosmscout:...:${MINGW_PREFIX}/bin                (test step, both jobs)
```

Each step concludes `success`, prints the environment it runs in (`MSYSTEM: UCRT64`), and the tools
the configure step starts come from that prefix:

```
Program D:/a/_temp/msys64/ucrt64/bin/rcc found: YES 5.15.19
Program D:/a/_temp/msys64/ucrt64/bin/qmlcachegen found: YES 5.15.19
```

## Font and locale (task 3.2)

Both verification steps (`Provide the font the font-dependent tests measure against`,
`Verify the font and locale environment`) conclude `success`, and the font resolves to the repository
file in both jobs rather than to a font that happens to be on the runner:

```
Liberation Sans resolves to: D:/a/libosmscout/libosmscout/libosmscout-map-opengl/data/fonts/LiberationSans-Regular.ttf
```

## Runs and tests (tasks 3.3, 3.4)

Run `45049676443` (commit `1268b9e77`):

| Job | Window (UTC) | Duration | Result |
|-----|--------------|----------|--------|
| `gcc and cmake` | 17:28:59 -> 17:44:40 | 15m41s | success |
| `gcc and meson` | 17:28:59 -> 17:47:56 | 18m57s | success |

Run `45052006614` (commit `7f3c8edec`, its documentation follow-up; the workflow files are the ones
of the commit above):

| Job | Window (UTC) | Duration | Result |
|-----|--------------|----------|--------|
| `gcc and cmake` | 17:54:02 -> 18:06:38 | 12m36s | success |
| `gcc and meson` | 17:54:03 -> 18:11:33 | 17m30s | success |

Every step of every job concludes `success`, including `Configure build project`, `Build project` and
`Run tests`. The test summaries:

```
45049676443  gcc and cmake  Run tests  100% tests passed out of 120
             gcc and meson  Run tests  Ok:                154
                                       Fail:              0

45052006614  gcc and cmake  Run tests  100% tests passed out of 120
             gcc and meson  Run tests  Ok:                156
                                       Fail:              0
```

The meson job runs the `PerformanceTest-cairo-*` cases (task 3.4), for example
`73/154 libosmscout:PerformanceTest-cairo-winter-sports.oss OK 2.21s`.

## What UCRT64 configures differently from MINGW64

The last green MINGW64 run `44582212555` (2026-11-19 17:54 UTC) is the baseline. Comparing the meson
feature block of both runs leaves two differences:

| Feature | MINGW64 | UCRT64, run `45049676443` | UCRT64, run `45052006614` |
|---------|---------|------|------|
| `libosmscout-client-qt` | false | true | true |
| `libosmscout-map-skia` | true | false | true |

`libosmscout-client-qt` compiles in the UCRT64 meson jobs, which it did not before — the Qt5 package
there brings the modules that dependency asks for. That is coverage gained, not lost (the meson test
count rises from 155 to 156).

`libosmscout-map-skia` was switched off in the first run by the runtime check of `meson.build`, which
exists because some Skia packages are built with CPU instructions that not every runner has:

```
Run-time dependency skia found: YES 143.r77119.da51f0d6
Checking if "Skia runtime check" with dependency skia runs: NO (3221225501)
meson.build:278: WARNING: Skia runtime check failed - disabling Skia backend
libosmscout-map-skia       : false
```

`3221225501` is `0xC000001D`, an illegal-instruction exception. The same check passes in the second
run with the same workflow files and the same package (`Checking if "Skia runtime check" with
dependency skia runs: YES`), and the `gcc and cmake` job of the first run passes the identical check:

```
-- Performing Test SKIA_RUNTIME_COMPATIBLE - Success
--  - Skia map drawing backend:     ON
```

That job builds and links `libosmscout_map_skia.dll` and passes `MapPainterSkiaTest`,
`SymbolRendererSkiaTest` and `SVGIconSkiaTest`. So the Skia backend stays covered by the matrix, and
whether the guard trips is a property of the runner a job lands on (the two jobs of a run are
different runners), not of this change: it degrades one test instead of failing the job, as it is
meant to.

DirectX, OpenGL, Qt, SVG, GDI and cairo are configured in both jobs as before (`d2d1.h` is found,
`libosmscout-map-directx : true`).

## The rest of the set (tasks 3.5, 3.6, 3.7)

The diff contains no file under `Tests/`, `libosmscout*/`, `OSMScout*/`, `Apple/`, `Android/` and no
`CMakeLists.txt` or `meson.build`, so no test is required for this change; the existing suites run
inside the two MSYS jobs above. No other job selects a MSYS2 environment.

The push that carries the workflow change (commit `1268b9e77`) ran the workflows this change does not
touch, all green:

| Workflow | Run | Result |
|----------|-----|--------|
| Ubuntu 26.04 | `45049676444` | success |
| Ubuntu 26.04 for Qt on Android | `45049676451` | success |
| OS X | `45049676478` | success |
| Visual Studio 2025 | `45049676460` | success |
| iOS | `45049676458` | success |
| Sanitize | `45049676420` | success |
| JavaScout build | `45049676423` | success |
| Sonar | `45049676415` | success |

`openspec validate ci-msys-ucrt64-migration --strict` reports the change as valid (task 2.3).