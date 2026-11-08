# Verification

Filled in from the pull request run (#1926), commit `1083ff14a`.

## Local pre-checks

- `openspec validate ci-ubuntu-26-04-runner-migration --strict` — `Change 'ci-ubuntu-26-04-runner-migration' is valid`.
- Every package the Linux jobs install was checked against the `resolute` (26.04) archive before the
  change was pushed (package pages of `packages.ubuntu.com`, each resolving for the suite):

  ```
  catch2 → transitional, points at libcatch2-dev (3.7.1)
  libcatch2-dev 3.7.1-0.6, clang-21 1:21.1.8-6ubuntu1, libclang-21-dev, libc++-dev,
  libagg-dev, libmarisa-dev, libtbb-dev, libglut-dev, libglew-dev, libglfw3-dev,
  libxml2-dev, nlohmann-json3-dev, libprotobuf-dev, protobuf-compiler, liblzma-dev,
  libpng-dev, libfreetype6-dev, libcairo2-dev, libpangocairo-1.0-0, libpango1.0-dev,
  qtdeclarative5-dev, libqt5svg5-dev, qtlocation5-dev, qtpositioning5-dev,
  qttools5-dev, qttools5-dev-tools, qtmultimedia5-dev, qmake6, libqt6svg6-dev,
  libqt6core5compat6-dev, qt6-declarative-dev, qt6-positioning-dev, qt6-tools-dev,
  qt6-tools-dev-tools, qt6-l10n-tools, qt6-multimedia-dev, libglm-dev, fonts-liberation,
  xvfb — all present.
  ```

## CI

Every job of the matrix concluded `success` on `ubuntu-26.04`; the only skipped job is `Publish images`
of `mapgen_image.yml`, which only runs on a main-branch commit.

| Workflow | Run | Result |
|----------|-----|--------|
| Ubuntu 26.04 (`build_and test_on_ubuntu_26_04.yml`) | 41494971206 | success — `clang`/`gcc` × meson/cmake, Qt6 × meson/cmake |
| Ubuntu 26.04 for Qt on Android | 41494971297 | success |
| Sanitize (`sanitize_on_ubuntu_26_04.yml`) | 41494971308 | success — clang ASan+UBSan, clang MSan, gcc ASan+UBSan |
| JavaScout build | 41494971428 | success |
| Sonar | 41494971212 | success — gate `OK` |
| Mapgen image | 41494971526 | success — image build, smoke, compose smoke; `Publish images` skipped on a pull request |
| OS X | 41494971329 | success |
| iOS | 41494971909 | success (the first attempt was cancelled with `The job was not acquired by Runner of type hosted`, a macOS arm64 capacity problem of the runner service; the rerun passed) |
| Visual Studio 2025 | 41494971237 | success |
| MSYS | 41494971334 | success (after the deprecation acknowledgement of the second commit) |

## Checks the run cannot report by itself

- **The test suite is built, not skipped.** `gcc and cmake` (job 125430575919) configures
  `-- - Catch2::Catch2WithMain        TRUE` and its test step reports `100% tests passed out of 170`.
  The log contains no `Disabled building of tests, since Catch2 dependency is not available`. The
  install step fetches `libcatch2-dev amd64 3.7.1-0.6` from `resolute/universe`.
- **The memory sanitizer toolchain comes from the release archive.** Job 125419016823 installs
  `libclang-21-dev amd64 1:21.1.8-6ubuntu1` from `resolute/universe`, adds no `apt.llvm.org`
  repository, passes the allocator check (`checking build/Tests/PerformanceTest-1.1.1`) and reports
  `100% tests passed out of 129`.
- **The sanitizer jobs keep their allocator assertion.** Both addressed jobs run the "Check that no
  sanitizer binary carries a second allocator" step, which passed.

## Findings that are not part of this change

Filed as issues: #1927 (both MSYS jobs acknowledge the MINGW64 deprecation until they move to UCRT64),
#1928 (the mapgen container base images are still on `ubuntu:noble` and `nginx:1.27-alpine`), #1929 (the
two githubactions lint rules that are ignored for the workflow tree).

- `msys2/setup-msys2@v2` started failing its own step with `MINGW64 is deprecated` on 2026-11-02, which
  turned every pull request of the repository red; the second commit acknowledges the deprecation, and
  #1927 records the move to UCRT64.
- The `githubactions:S8544`/`S7637` findings of SonarCloud report the dependency references of the
  whole workflow set (16 open findings on `master` before this change). A change that touches a
  dependency list or renames a workflow file turns them into new code and failed the quality gate on
  them; the second commit ignores both rules for `.github/workflows/**` with the reason recorded in
  `sonar-project.properties`, as the project already does for the md5 usage of the map scripts, and
  #1929 asks whether they should be pinned instead. The gate is `OK` afterwards.
