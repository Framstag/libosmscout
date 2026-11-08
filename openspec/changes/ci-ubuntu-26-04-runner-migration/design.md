# Design

## Context

See `proposal.md` - Why. State the decisions were taken against:

**The jobs and their labels.** 18 `runs-on` entries across ten workflows name `ubuntu-24.04`, and one
job (the Android NDK build) lives in a workflow whose file name still says `22_04`. No job names
`ubuntu-22.04`, so nothing fails today; the migration is ahead of the deprecation, not after it.

| Workflow | Jobs | Notes |
|----------|------|-------|
| `build_and test_on_ubuntu_24_04.yml` | 5 (`clang`/`gcc` × meson/cmake, Qt6 × meson/cmake) | installs Qt5 **and** Qt6, AGG, cairo/pango, Catch2, `xvfb` |
| `sanitize_on_ubuntu_26_04.yml` | 3 (clang ASan+UBSan, clang MSan, gcc ASan+UBSan) | MSan adds `apt.llvm.org` `noble` for `clang-21` |
| `build_on_ubuntu_22_04_qt_android.yml` | 1 | vendor Qt 5.15.2 for Android, NDK 27.2.12479018 |
| `build_javascout.yml` | 2 | JDK 25, Maven, Catch2 |
| `sonar.yml` | 1 | Sonar build wrapper, Catch2, lcov/gcovr |
| `mapgen_image.yml` | 3 | builds `scripts/mapgen/Dockerfile` (`ubuntu:noble` inside) |
| `webpage.yml`, `release.yml`, `release_latest.yml` | 1 each | Hugo/doxygen, meson dist, JReleaser |

**What the image provides** (runner-images `main`, 2026-11-02, image `20261026.180.1`, Ubuntu 26.04.1
LTS, kernel 7.0.0-1017-azure): clang 20/21/22, gcc, cmake, ninja, `catch2` 3.7.1 (transitional) /
`libcatch2-dev` 3.7.1, Qt5 5.15.18 (still in `resolute` despite the planned Qt5-removal transition),
Qt6 6.x. Every package the workflows install was checked to exist in `resolute`: `libagg-dev`,
`libmarisa-dev`, `libtbb-dev`, `libglut-dev`, `libglew-dev`, `libglfw3-dev`, `qtdeclarative5-dev`,
`qtlocation5-dev`, `qtmultimedia5-dev`, `qt6-*`, `libpango1.0-dev`, `libcairo2-dev`,
`nlohmann-json3-dev`, `libprotobuf-dev`, `fonts-liberation`, `xvfb`, `clang-20`, `clang-21`,
`libclang-21-dev`, `libcatch2-dev`.

**The tests have to be run, not skipped.** `CMakeLists.txt` prints "Disabled building of tests, since
Catch2 dependency is not available" and continues when `find_package(Catch2 3)` does not produce
`Catch2::Catch2WithMain`. A green job therefore proves nothing about the test suite unless the Catch2
package resolves — this is the migration's main silent-failure risk.

**Both workflows ignore workflow-only changes.** `build_and test_on_ubuntu_26_04.yml` and
`sanitize_on_ubuntu_26_04.yml` declare `paths-ignore: '.github/workflows/**'` for their pull-request
trigger, so the change has to touch files outside that directory to be exercised — which it does
(`README.md`, `AGENTS.md`, `ci/docker/`, this directory).

## Goals / Non-Goals

**Goals:** every Linux job scheduled on the GA Ubuntu image of the current LTS; the workflows keep
building and *running* the test suite; the names of the files and of the README/AGENTS tables match the
release; the provisioning steps belong to the release they run on.

**Non-Goals:** the container base images (`scripts/mapgen/Dockerfile` on `ubuntu:noble`,
`ci/docker/ubuntu_24.04_*` on `ubuntu:24.04` and `ubuntu:noble`) — a distro bump there changes a fixed
runtime package list and needs an image build and the smoke checks, which #1928 tracks as an
an open item. Also out of scope: moving to the arm64 labels, and the macOS/Windows jobs.

## Decisions

### D1 — Move every Linux job in one change, not one workflow at a time

| Alternative | Assessment |
|---|---|
| **All ten workflows in one change (chosen)** | The label appears in 18 places; a partial move leaves two Ubuntu releases to reason about and the "which release does CI measure" question open. One change, one CI run, one review. |
| One workflow per pull request | Each run costs a full matrix; the intermediate states have the README badge table contradicting the workflows. Rejected. |
| Stay on `ubuntu-24.04` | Defensible (24.04 is supported until 2029), but the `ubuntu-latest` alias moves to 26.04 in November 2026 and every dependency is verified to exist in 26.04 now. Waiting only moves the work. Rejected. |

**Risk:** a package that exists but behaves differently (Qt5 5.15.18 vs 5.15.13, Catch2 3.7.1 vs 3.4.0,
gcc 15 vs 13). Mitigation: the pull request runs the whole matrix before anything is merged; the test
steps fail loudly rather than skip, and the "no tests were built" case is checked in the log.

### D2 — Name the image version, not `ubuntu-latest`

| Alternative | Assessment |
|---|---|
| `ubuntu-26.04` (chosen) | Same rule the macOS jobs follow in `ci-macos-26-runner-migration`: an image move is a reviewed change, and a release that changes under the repository cannot break it silently. |
| `ubuntu-latest` | Would have followed 26.04 automatically in November 2026 — the exact silent move the repository just decided against for macOS. Rejected. |

**Risk:** the pinned image has to be bumped by hand after its release reaches end of life. Accepted:
that is the point.

### D3 — Install the Catch2 package under the name 26.04 gives it

| Alternative | Assessment |
|---|---|
| `libcatch2-dev` (chosen) | The release's real package (3.7.1); `catch2` is a transitional package that only pulls it in. Names what is meant, survives the removal of the transitional package. |
| Keep `catch2` | Works today (transitional → `libcatch2-dev`), but depends on a compatibility name whose removal is announced by its existence. Rejected. |
| Build Catch2 from source in every job | Removes the dependency on the distro's version, costs a build in five jobs, and the MSan job already does it for a different reason (instrumentation). Rejected. |

**Risk:** Catch2 3.7.1 rejects test code that 3.4.0 accepts (deprecated matcher/`SECTION` behaviour
changes). Mitigation: the test steps run in CI before the merge; a failure is a source fix, not a
package rollback.

### D4 — Take `clang-21` from the 26.04 archive, drop the `apt.llvm.org` `noble` repository

| Alternative | Assessment |
|---|---|
| Archive `clang-21` (chosen) | `resolute` carries `clang-21` 1:21.1.8-6ubuntu1 and `libclang-21-dev`; the memory-instrumented libc++ is built from LLVM sources with that compiler. No third-party repository, no mixing of releases. |
| `apt.llvm.org/resolute/` | The project's 26.04 repository may or may not be published under that suite name at the time of the change; it is not needed, so the dependency on it is removed. |
| Keep `apt.llvm.org/noble/` on a 26.04 image | `noble` `clang-21` debs against a 26.04 userland: glibc is newer at runtime, so it usually links, but the mix is unverified and unsupported. Rejected. |

**Risk:** the archive's `clang-21` patch level differs from the one the MSan job has been building
libc++ with. Accepted; MSan is built from source with the compiler it is told to use.

### D5 — Rename the workflow files with the release

| Alternative | Assessment |
|---|---|
| Rename (chosen) | The file name is how the Actions UI, the README badges and `AGENTS.md` refer to the job, so a stale name is a wrong statement in three places. |
| Keep the file names | Cheaper diff, but `build_on_ubuntu_22_04_qt_android.yml` already proves how a stale name survives two migrations. Rejected. |

**Risk:** the renamed files break links and badges. Mitigation: every reference in `README.md`,
`AGENTS.md` and `.pi/skills/` is updated in the same change; the workflow name in the Actions UI comes
from the `name:` key, which is updated too.

## The memory sanitizer job after the change

```
job scheduled on ubuntu-26.04
    |
    v
apt-get update
    |                                   (no third-party LLVM repository)
    v
apt-get install clang-21 libclang-21-dev  -- from the release archive
    |
    v
build libc++/libc++abi from llvm-project with -DLLVM_USE_SANITIZER=MemoryWithOrigins
    |
    v
build Catch2 from source with -stdlib=libc++ -fsanitize=memory
    |
    v
cmake configure (-DCMAKE_PREFIX_PATH=/opt/libcxx-msan) -> build -> allocator check -> ctest
```

## Verification strategy

The pull request is the verification: the whole matrix (meson/cmake × gcc/clang × Qt5/Qt6, both
sanitizer jobs, JavaScout, Sonar, the Android NDK build, the container images and the release
workflows that fire on the branch) runs on the new image before the merge. The two checks the CI run
cannot make by itself are read out of the log by hand: the test suite is *built* (no "Disabled building
of tests" line) and the sanitizer binaries are checked for a second allocator, which the workflows
already assert.
