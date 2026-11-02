# Design

## Context

See `proposal.md` - Why for the failure. State the decisions were taken against:

**The jobs and their labels.** `.github/workflows/build_and test_on_osx.yml` has two jobs (`meson`,
`cmake`), `.github/workflows/build_and test_on_ios.yml` has one (`cmake`); all three named
`runs-on: macos-14`, the arm64 image (the x64 macOS 14 image is the separate label `macos-14-large`,
which the repository does not use). Both workflows ignore path changes under `.github/workflows/**`
for their pull-request trigger, so the change is only exercised on a pull request that also touches a
file outside that directory - this change's OpenSpec and documentation files.

**What the available arm64 images provide** (runner-images `main`, 2026-11-01):

| Label | macOS | Xcode |
|-------|-------|-------|
| `macos-14` (deprecated, retired 2026-11-02) | 14 | 15.x |
| `macos-15` | 15 | 16.x |
| `macos-26` (= `macos-latest`) | 26.6.2 | 26.6 (iOS 26.5 SDK, clang 21) |

**What the jobs do with the image.** The OS X jobs install Homebrew `qt@5`, `cairo`, `pango`,
`glfw`, `glew`, `protobuf`, `libxml2` and friends and then configure and build the whole project -
`qt@5` is the 5.15.19 Homebrew builds from source, so the AGL framework that Xcode 26 removed from
the macOS SDK (it breaks the vendor Qt 5.15.2 macOS binaries) is not in their link line. The iOS job
downloads the vendor Qt 5.15.2 iOS package with `jurplel/install-qt-action@v4`, selects the newest
stable Xcode with `maxim-lobanov/setup-xcode@v1`, and configures a device (not simulator) build with
`-DCMAKE_OSX_DEPLOYMENT_TARGET=13`.

```
job scheduled on the named image
    |
    v
macos-26 arm64 runner (Xcode 26.6)  --osx jobs-->  brew install ... -> meson/cmake configure -> build -> tests
                                    --ios job -->  install-qt-action -> setup-xcode -> cmake configure -> build
```

## Goals / Non-Goals

**Goals:**

- The macOS jobs are scheduled again, on an image the runner-image project supports, before the
  macOS 14 label disappears.
- The change states which image each macOS job names and why, so the next retirement is a reviewed
  edit rather than a silent outage.
- The iOS check configures a deployment target the toolchain of its image accepts, and the floor it
  uses is written down where it is set.

**Non-Goals:**

- Moving the jobs off Qt 5. That is the `qt@5` Homebrew deprecation and its own change (recorded in
  `TODO.md`).
- Fixing the `PKG_CONFIG_PATH` both OS X jobs export: it names the Intel Homebrew prefix
  (`/usr/local/opt/...`) on an arm64 image and overrides the arm64 path Homebrew prints
  (`/opt/homebrew/opt/...`), but the jobs configure and pass today because Qt5 and libxml2 are found
  through their CMake configs. Recorded in `TODO.md`.
- Raising the deployment targets of the shipped Apple projects. The check and the projects disagree
  after this change; the projects are what ships and that is a product decision, recorded in
  `TODO.md`.
- `macos-latest`. Rejected below.

## Decisions

### D1: Move every macOS job to `macos-26`

**Chosen:** `macos-26` for all three jobs.

Alternatives:

- **`macos-15`** — still GA and the closest step (macOS 15, Xcode 16.x), so it keeps the Xcode 16
  toolchain and the iOS job's deployment target 13 would keep working. Rejected because the image is
  one GA behind: the macOS 27 image is already in public preview (`xcode-27`), so `macos-15` starts
  its own deprecation as soon as macOS 27 reaches GA, and the migration would be repeated within one
  cycle.
- **`macos-latest`** — resolves to `macos-26` today, so it is behaviorally identical now and moves to
  macOS 27 without a repository change. Rejected: an image move changes the compiler, the SDK and the
  Homebrew package set under the tests, and a green run must not be able to become a red one without a
  commit that says so. The previous state (a pinned label) is the intended one.

Risk of the chosen option: the whole toolchain changes at once (Xcode 15.x/16.x -> 26.6, clang -> 21,
Homebrew `qt@5` 5.15.19 against a newer SDK). Mitigation: this is what the pull request's own CI run
measures, on both macOS workflows, and the earlier Qt 5.15-on-Xcode-26 failures apply to the vendor
macOS binaries, not to the Homebrew build the OS X jobs use.

### D2: The iOS check uses the deployment floor of its toolchain

**Chosen:** `-DCMAKE_OSX_DEPLOYMENT_TARGET=15` for the iOS job.

Alternatives:

- **Keep 13** — the value matches what the Apple projects declare, but Xcode 26 documents its
  supported deployment targets as iOS 15 and later, and the simulator of that toolchain rejects a
  lower value outright. A check that cannot configure is worth less than a check one floor higher.
- **Keep the iOS job on `macos-15`** so 13 keeps working - rejected with D1: it buys one cycle and
  splits the repository's macOS jobs across two images.
- **Raise the value and the Apple projects together** — the honest state, but it changes what the
  shipped iOS application supports and needs its own decision; `TODO.md` records it.

Risk of the chosen option: the iOS check no longer compiles the project at the floor the shipped
projects declare, so a source change that requires iOS 15 APIs would pass CI and break the projects.
That risk exists in the other direction today (nothing verifies the floor either way), and the
`TODO.md` entry is what keeps it visible.

## Risks / Trade-offs

- **Homebrew `qt@5` does not build or link on macOS 26** (the formula is deprecated since
  2026-05-19 and other Qt 5 ports needed patches on macOS 26) → the OS X jobs fail at configure or
  link; the pull request's run answers it, and the fallback is `macos-15` (D1's rejected alternative)
  as a second commit rather than a redesign.
- **The vendor Qt 5.15.2 iOS package does not build against the iOS 26.5 SDK** → the iOS job fails at
  configure or link; the same fallback applies, or the Qt version of the action is raised.
- **`brew install ... pkgconfig ...` fails** because the formula was renamed to `pkgconf` → unchanged
  risk: Homebrew redirects the old name (the run logs `Warning: pkgconf 3.0.6 is already installed and
  up-to-date.`), so this is a name to clean up, not a blocker.
- **The pull-request trigger does not fire** because the workflows ignore `.github/workflows/**` →
  this change touches `openspec/`, `AGENTS.md` and `TODO.md` as well, so the paths filter lets the
  runs through; the verification of the change is those runs.

## Migration Plan

Three label values and one configure argument, no state, no data, no API. Steps: apply the workflow
change, push the branch, read the results of the OS X and iOS jobs of the pull request, and adapt the
image only if a job fails for a reason of the image. Rollback is reverting the commit; the retired
label is not a state anything can return to, so the rollback target for a failing image is `macos-15`.

## Open Questions

- Whether the Homebrew `qt@5` build and both build systems pass on macOS 26 - answered by the pull
  request's own OS X runs (see `verification.md`).
- Whether the vendor Qt 5.15.2 iOS package links against the iOS 26.5 SDK with a deployment target of
  15 - answered by the pull request's iOS run (see `verification.md`).
