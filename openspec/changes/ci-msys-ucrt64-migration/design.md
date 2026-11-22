# Design

## Context

See `proposal.md` - Why for the failure. The state the decisions were taken against:

**The jobs.** `.github/workflows/build_and test_on_msys.yml` has two jobs, `gcc and cmake` and
`gcc and meson`, both `runs-on: windows-2025` with `defaults.run.shell: msys2 {0}`. Each job's setup
step is `msys2/setup-msys2@v2` with `update: true` (a full rolling upgrade) and an `install` list;
neither job passed `msystem`, so the action's default `MINGW64` applied, and both lists spelled their
packages `mingw-w64-x86_64-*`.

**What the two environments provide** (`packages.msys2.org`, 2026-11-22; a package's `repos` field is
the set of environments its build is published in):

| Package | MINGW64 | UCRT64 | CLANG64 |
|---------|---------|--------|---------|
| `gcc` / `toolchain` | yes | yes | (clang instead) |
| `libtool`, `cmake`, `meson`, `ninja` | yes | yes | yes |
| `catch`, `protobuf`, `libxml2`, `nlohmann-json` | yes | yes | yes |
| `cairo`, `pango`, `qt5` (5.15.19) | yes | yes | yes |
| `glew`, `glm`, `libpng`, `directx-headers` | yes | yes | yes |
| `glfw` (3.5.1) | **no** since 2026-11-18 | yes | yes |
| `skia` (143.r77119) | **no** since 2026-11-18 | yes | yes |
| `extra-cmake-modules` | no | yes | yes |

**Why MINGW64 is being emptied.** Both removals are the same MSYS2 series: `MINGW-packages#33705`
("mingw64: drop some leaf packages", merged 2026-11-18 17:05 UTC) and `#33714` (same title, merged
2026-11-18 20:50 UTC) reduce `mingw_arch` of a long list of packages to
`('ucrt64' 'clang64' 'clangarm64')` — one line per `PKGBUILD`:

```
-mingw_arch=('mingw64' 'ucrt64' 'clang64' 'clangarm64')
+mingw_arch=('ucrt64' 'clang64' 'clangarm64')
```

The packages MSYS2 keeps dropping are the ones no `mingw64` maintainer builds for, so the environment
loses coverage package by package; the `extra-cmake-modules` loss the repository already handled in
2026-11-19 (`52b87506b`, dropping the entry from the cmake job) is the same series one package
earlier. `msys2/setup-msys2@v2` reports the environment itself as deprecated and fails its step unless
the caller acknowledges it.

**Where the environment enters the jobs.** Apart from the install prefix, the jobs read the
environment through variables the msys2 shell exports, not through names:

- `MINGW_PREFIX` (`/mingw64` -> `/ucrt64`): the font-configuration directory
  `${MINGW_PREFIX}/etc/fonts/conf.d`, the CMake job's `DXSDK_DIR` (`${MINGW_PREFIX}/${MINGW_CHOST}`,
  read by `cmake/FindDirect2D.cmake` as `$ENV{DXSDK_DIR}/Include`), and the test `PATH`
  (`${MINGW_PREFIX}/bin`, where the Qt, cairo, pango, protobuf and skia DLLs live);
- `MINGW_CHOST` (`x86_64-w64-mingw32`, identical in both environments): the DirectX header root the
  `directx-headers` package installs into, because that `PKGBUILD` places its headers under
  `${MINGW_PREFIX}/${MINGW_CHOST}` in every environment it builds for.

```
msys2/setup-msys2@v2  --msystem: UCRT64-->  /ucrt64 prefix
     |
     +--> pacman -S mingw-w64-ucrt-x86_64-*   (glfw, skia, qt5, cairo, pango, ... resolve)
     |
     +--> MINGW_PREFIX=/ucrt64, MINGW_CHOST=x86_64-w64-mingw32
              |
              +--> font conf.d snippet, DXSDK_DIR, test PATH
```

## Goals / Non-Goals

**Goals:**

- Both MSYS jobs install, configure, build and test again, on the environment MSYS2 keeps the package
  set the repository builds with in.
- The environment is a stated property of the jobs, so the next package MSYS2 drops from an
  environment is caught by the same statement instead of by a red run.
- The deprecation is left behind instead of suppressed.

**Non-Goals:**

- Moving the jobs to Qt 6. Both environments carry Qt 5.15.19, so the Qt version is orthogonal to the
  environment and stays where it is.
- Moving to CLANG64. It carries every package as well, but it changes the compiler from gcc to clang
  for jobs whose names say `gcc`, and the project's clang coverage is the Ubuntu clang build
  (`.github/workflows/build_and test_on_ubuntu_26_04.yml`, `sanitize_on_ubuntu_26_04.yml`).
- The MSVC job (`.github/workflows/build_and test_on_vs2025.yml`) and the vcpkg-based Windows
  dependencies: untouched by the MSYS2 environment.
- Re-adding `extra-cmake-modules`: nothing in the repository uses ECM, and the meson job never
  installed it; the comment above the install list stays for that reason.

## Decisions

### D1: Both jobs select UCRT64

**Chosen:** `msystem: UCRT64` in both jobs, with install lists moved to the `mingw-w64-ucrt-x86_64-`
prefix.

Alternatives:

- **Keep MINGW64 and drop `glfw`/`skia` from the lists, as the `extra-cmake-modules` drop did.** The
  smallest edit and it turns the jobs green today. Rejected: it removes the OpenGL and Skia backends
  from the only Windows/gcc validation the repository has - `PerformanceTest` and
  `OpenGLAreaVisibilityTest` (Sources) register only when `TARGET glfw` exists, and
  `libosmscout-map-skia` is skipped - so the CI signal would silently cover less. It also leaves the
  jobs on a deprecated environment that loses packages package by package, which is a repeated red
  run instead of one migration. That MSYS2 removed an *unused* package is a reason to drop
  `extra-cmake-modules`; it is not a reason to drop used ones.
- **CLANG64.** Also carries every package. Rejected: it changes the compiler of jobs named `gcc`, so
  the repository would lose its MinGW/gcc validation and gain a clang/Windows combination it does not
  currently state; the compiler is not the problem.
- **Move to a container or `msvcrt` packages from the Arch/`clang64` repository.** Rejected: mixing
  runtimes inside one environment is not supported by MSYS2 and would break the DLL search the test
  step relies on.

Risk of the chosen option: the whole MSVCRT -> UCRT runtime changes at once (UCRT replaces the
`msvcrt.dll` C runtime for every library the build links), so a test that depends on C-runtime
behavior - `printf` variants, locale, file encoding, the `tchar`/`_s` functions - could behave
differently, and the font-dependent tests run through a different cairo/pango build. Mitigation: the
pull request's own MSYS run measures both jobs on the full test suites (`ctest -j 4`, `meson test`),
and the two font/locale verification steps fail loudly instead of letting the tests measure a
different font.

### D2: The setup steps name the environment; nothing else names it

**Chosen:** add `msystem: UCRT64`, rename the two setup steps to `Setup MSYS/UCRT64`, and leave the
`MINGW_PREFIX`/`MINGW_CHOST` steps and `DXSDK_DIR` alone, with a comment above the cmake job's setup
step stating why the environment is UCRT64 and that the prefix is what follows from it.

Alternatives:

- **Pass the prefix explicitly everywhere** (`mingw-w64-ucrt-x86_64-*` plus hardcoded `/ucrt64`). It
  makes the workflow greppable for an environment name, but it duplicates what the msys2 shell
  already exports and drifts the moment the environment moves again - the same duplication the
  `MINGW_PREFIX` use avoided while the jobs ran in MINGW64.
- **Use `${{ env.MINGW_PACKAGE_PREFIX }}` in the install list.** Rejected: the environment variable
  is only exported inside the msys2 shell the action runs later, so it is not available while the
  step's inputs are rendered; a literal list is what the action needs.

Risk of the chosen option: a reader who wants to know the environment has to read two `msystem` lines
rather than the whole file - acceptable, because the package prefix and `MSYS/UCRT64` step name say
it as well.

### D3: The deprecation acknowledgement goes away

**Chosen:** remove `suppress-deprecation-warnings: msystem-mingw64` from both jobs.

Alternatives:

- **Keep it.** With `msystem: UCRT64` the input names an environment the job no longer selects, so it
  would acknowledge nothing; leaving it would be a line a reader has to check against the `msystem`
  value.

## Risks / Trade-offs

- **A package the jobs install has no UCRT64 build** -> the setup step fails with
  `error: target not found`, which is the same message that started this change; the list was checked
  against `packages.msys2.org` for every entry (Context table), and the pull request's run confirms
  the resolution.
- **UCRT64's `qt5`/`cairo`/`pango` builds behave differently under the UCRT runtime** (font
  resolution, locale initialization) -> the font and locale verification steps fail before the tests
  run, and the cairo/skia/OpenGL test cases are the evidence that they pass; a failure there is a
  follow-up of the same change, not a redesign.
- **`DXSDK_DIR` stops resolving the DirectX headers** because the `directx-headers` package installs
  under a different relative path in UCRT64 -> the cmake job fails at the DirectX backend; the
  package's `PKGBUILD` installs relative to `${MINGW_PREFIX}/${MINGW_CHOST}` in every environment, and
  the run's configure output says whether `d2d1.h`/`d3d11.h` were found.
- **The `gh`-less reader cannot follow the package table** -> the two `MINGW-packages` pull-request
  numbers in the proposal name the removals; `packages.msys2.org` shows the current `repos` of any
  package.

## Migration Plan

Two `msystem` values, two package prefixes, one removed input, one README row, one `AGENTS.md` note -
no state, no data, no API, no build-system change. Steps: apply the workflow edit, push the branch,
read the two MSYS jobs of the pull request, and adapt only if a job fails for a reason of the
environment. Rollback is reverting the commit; the rollback target is not MINGW64, which no longer
carries the packages, but MINGW64 with `glfw` and `skia` removed from the lists - the state that
keeps the jobs green at the cost of the OpenGL and Skia coverage (D1's rejected alternative).

## Open Questions

- Whether the full test suites pass under the UCRT runtime on both build systems - answered by the
  pull request's own MSYS runs (see `verification.md`).
- Whether the `qt5` packages of UCRT64 serve `uic`/`moc`/`rcc` as the MINGW64 ones did - answered by
  the configure output of both jobs.