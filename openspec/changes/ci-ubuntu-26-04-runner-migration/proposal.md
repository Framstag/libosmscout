# Proposal

## Why

Every Linux job of the repository is pinned to `ubuntu-24.04`. The runner-image project has been
shipping a general-availability Ubuntu 26.04 image since 2026, and it moves the `ubuntu-latest` alias to
26.04 in November 2026 ([actions/runner-images#14748](https://github.com/actions/runner-images/issues/14748)).
The Ubuntu 22.04 images enter deprecation on 2026-09-17 and are unsupported from 2027-04-17
([actions/runner-images#14254](https://github.com/actions/runner-images/issues/14254)), which is the
next image the repository would otherwise be pushed off — the same situation the macOS jobs were in
when `macos-14` was retired (see `openspec/changes/ci-macos-26-runner-migration`).

Moving now keeps the Linux signal on a release that is supported for the rest of the LTS window and
keeps an image move a reviewed change instead of a forced one. The names are also behind the content:
`build_on_ubuntu_22_04_qt_android.yml` runs on `ubuntu-24.04`, and the `README.md` build-status table
points three badges at workflow files that no longer exist (`build_and test_on_ubuntu_22_04.yml`,
`sanitize_on_ubuntu_22_04.yml`, `build_and test_on_ubuntu_22_04_qt_android.yml`) and one at
`build_and test_on_vs2019.yml`, which is now `build_and test_on_vs2025.yml`. A reader cannot tell from
the name which release a job measures.

Ubuntu 26.04 is not a drop-in for the workflows as they stand. Two provisioning steps are tied to the
release they were written for: the Catch2 package is a transitional name in 26.04 (the library is
`libcatch2-dev`, 3.7.1), and the memory sanitizer job adds the `apt.llvm.org` repository of `noble` to
get `clang-21`, although 26.04 carries `clang-21` in its own archive. Both would either fail or install
a toolchain built against an older release.

## What Changes

- Every Linux job names `ubuntu-26.04`, the current GA Ubuntu image, instead of `ubuntu-24.04`.
- The workflow files that carry the release in their name are renamed with the release they run on:
  `build_and test_on_ubuntu_24_04.yml` → `build_and test_on_ubuntu_26_04.yml`,
  `sanitize_on_ubuntu_24_04.yml` → `sanitize_on_ubuntu_26_04.yml`, and the misnamed
  `build_on_ubuntu_22_04_qt_android.yml` → `build_on_ubuntu_26_04_qt_android.yml`.
- The jobs install the Catch2 development package under the name the release provides it, so the test
  suite is built and run rather than silently skipped.
- The memory sanitizer job takes `clang-21` from the archive of the release the image runs on and no
  longer adds a package repository pinned to an older release.
- The `README.md` build-status table and the workflow table in `AGENTS.md` name the renamed workflows;
  the stale `22.04` rows and the `vs2019` row go.
- The stale `libosmscout/ubuntu_22.04_*` image tags of the 24.04 docker helpers (`ci/docker/`) name the
  directory they live in.

## Capabilities

### New Capabilities

None. Runner image selection and workflow provisioning are CI behavior that the existing
`ci-build-fixes` capability owns.

### Modified Capabilities

- `ci-build-fixes`: the requirements that cover workflow provisioning gain the release a Linux job is
  scheduled on and the requirement that a package a workflow installs is named as the release of that
  image names it, so that a migration cannot trade a failing install for a silently skipped test suite.

## Impact

- `.github/workflows/build_and test_on_ubuntu_26_04.yml` (renamed from `…_ubuntu_24_04.yml`) — five
  jobs move to `ubuntu-26.04`, and each dependency list installs `libcatch2-dev`.
- `.github/workflows/sanitize_on_ubuntu_26_04.yml` (renamed from `…_ubuntu_24_04.yml`) — three jobs
  move to `ubuntu-26.04`; the address/undefined jobs install `libcatch2-dev`; the memory sanitizer job
  drops the `noble` `apt.llvm.org` repository and installs `clang-21` from the release archive.
- `.github/workflows/build_on_ubuntu_26_04_qt_android.yml` (renamed from
  `…_ubuntu_22_04_qt_android.yml`) — the Android NDK job moves to `ubuntu-26.04`.
- `.github/workflows/build_javascout.yml`, `sonar.yml`, `webpage.yml`, `release.yml`,
  `release_latest.yml`, `mapgen_image.yml` — every job moves to `ubuntu-26.04`; the two JavaScout jobs
  and the Sonar job install `libcatch2-dev`. The container base images the `mapgen_image.yml` jobs
  build (`scripts/mapgen/Dockerfile`, `ubuntu:noble`) are unchanged: that is a separate, unverified
  image bump that #1928 tracks.
- `README.md` — the build-status table names the renamed workflows and drops the stale rows.
- `AGENTS.md` (CI/CD section) — the workflow table names the renamed files, and a note records that the
  Linux jobs name their image version, why 26.04, and the two provisioning differences to 24.04.
- `ci/docker/ubuntu_24.04_gcc_cmake/{build,run}.sh`, `ci/docker/ubuntu_24.04_gcc_meson_qt6/{build,run}.sh`
  — the image tags name the directory they belong to instead of `ubuntu_22.04`.
- `openspec/specs/ci-build-fixes/spec.md` — the requirements this change adds (via this directory's
  spec delta).
