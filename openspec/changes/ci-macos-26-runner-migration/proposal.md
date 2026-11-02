# Proposal

## Why

GitHub retires the macOS 14 runner image on 2026-11-02 ([actions/runner-images#13518](https://github.com/actions/runner-images/issues/13518)).
The deprecation started 2026-07-06, and scheduled brownouts fail jobs that name the label before the
retirement. Three jobs of this repository name `macos-14`: the `meson` and `cmake` jobs of
`.github/workflows/build_and test_on_osx.yml` and the `cmake` job of
`.github/workflows/build_and test_on_ios.yml`.

The macOS CI signal of this repository is already lost. The last successful OS X and iOS runs started
2026-11-01 13:37 UTC and 13:46 UTC. Every run after 2026-11-01 14:00 UTC - inside the brownout window
of that date - concludes `cancelled`, and its macOS jobs never start a step:

```
$ gh run view 41345662834 --json jobs --jq '.jobs[] | {name, conclusion, startedAt, steps: (.steps|length)}'
{"name":"meson","conclusion":"cancelled","startedAt":null,"steps":0}
{"name":"cmake","conclusion":"cancelled","startedAt":null,"steps":0}
```

Every macOS job of the repository fails this way from 2026-11-02 on, when the label is gone
entirely.

## What Changes

- The two OS X jobs and the iOS job name `macos-26`, the current GA macOS arm64 image, instead of the
  retired `macos-14`.
- The iOS job configures the deployment floor of the toolchain that image provides: it passes
  `-DCMAKE_OSX_DEPLOYMENT_TARGET=15` (Xcode 26 accepts iOS 15 and later) instead of `13`, and states
  the reason where the value sits.
- The rule behind the change becomes explicit: a CI job names a runner image the runner-image project
  still supports, and a macOS job names the image version rather than a moving `-latest` alias, so an
  image move is a reviewed change instead of a silent one.

## Capabilities

### New Capabilities

None. Runner image selection is CI behavior that the existing `ci-build-fixes` capability owns.

### Modified Capabilities

- `ci-build-fixes`: the requirements that cover workflow configuration (dependency provisioning,
  font environment) gain the runner image a job is scheduled on: a job names a supported image, the
  macOS jobs name the current GA arm64 image, and the deployment target of an Apple platform build
  check follows the floor of the Xcode the image provides.

## Impact

- `.github/workflows/build_and test_on_osx.yml` — both jobs move to `macos-26` (fix applied with this
  change).
- `.github/workflows/build_and test_on_ios.yml` — the job moves to `macos-26` and its deployment
  target rises to 15 (fix applied with this change).
- `.github/workflows/build_and test_on_msys.yml`, `build_and test_on_ubuntu_24_04.yml`,
  `build_and test_on_vs2025.yml`, `build_on_ubuntu_22_04_qt_android.yml`,
  `sanitize_on_ubuntu_24_04.yml`, `sonar.yml`, `build_javascout.yml` — name non-macOS or already
  supported labels; unchanged here, inspected as the rest of the set the rule holds for.
- `AGENTS.md` (CI/CD section) — records the macOS runner policy and why the iOS job's deployment
  target is 15 while the shipped Apple projects declare less.
- `TODO.md` — the follow-ups this migration does not take: the arm64 Homebrew prefix the OS X jobs'
  `PKG_CONFIG_PATH` contradicts, the deprecated `qt@5` formula both OS X jobs install, and the
  difference between the deployment target the iOS check now uses and the one the Apple projects
  declare.
