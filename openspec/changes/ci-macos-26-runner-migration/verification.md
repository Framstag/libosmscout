# Verification

The macOS jobs can only be verified on a macOS 26 runner, so the verification is this change's own
pull request: [#1925](https://github.com/Framstag/libosmscout/pull/1925), head
`ci/macos-26-runner-migration`, commit `81921e491`, pushed 2026-11-01 20:36 UTC.

## The image the jobs were scheduled on

```
OS X  cmake  Set up job  Included Software: https://github.com/actions/runner-images/blob/macos-26-arm64/20261026.0455/images/macos/macos-26-arm64-Readme.md
                          Image Release: https://github.com/actions/runner-images/releases/tag/macos-26-arm64%2F20261026.0455
OS X  meson  Set up job  Image: macos-26-arm64
```

The labels resolved to the macOS 26 arm64 image (`macos-26-arm64/20261026.0455`, macOS 26.6.2, Xcode
26.6), not to a fallback.

## Runs (tasks 3.1, 3.2, 3.3)

| Workflow | Run | Window | Result |
|----------|-----|--------|--------|
| OS X (`41347710996`) | `meson` | 20:39:10 -> 20:49:00 | success |
| OS X (`41347710996`) | `cmake` | 20:36:20 -> 20:42:16 | success |
| iOS (`41347711040`) | `cmake` | 20:36:20 -> 20:45:34 | success |

```
OS X  cmake  Run tests      100% tests passed out of 152
OS X  meson  Execute tests  Ok:                155
                            Fail:              0
```

The OS X jobs therefore build and test with Xcode 26.6, clang 21 and Homebrew `qt@5` 5.15.19 (the
source build, so the AGL framework Xcode 26 removed from the macOS SDK is not in its link line), on
both build systems.

The iOS job compiles against the iOS 26.5 SDK of that Xcode with deployment target 15:

```
iOS  cmake  Configure build project  -- Looking for lzma_easy_encoder in
                                     /Applications/Xcode_26.6.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS26.5.sdk/usr/lib/liblzma.tbd - found
iOS  cmake  Configure build project  -- Configuring done (33.9s)
```

with the vendor Qt 5.15.2 iOS package (`jurplel/install-qt-action@v4`) and `maxim-lobanov/setup-xcode@v1`
selecting `latest-stable`, that is Xcode 26.6. The deployment target of 13 this change replaces would
have been below what that Xcode accepts.

## The rest of the set (task 3.5)

The same push ran the workflows this change does not touch. When this was recorded:

```
Ubuntu 24.04                       success
Ubuntu 24.04 for Qt on Android     success
JavaScout build                    success
iOS                                success
OS X                               success
MSYS, Visual Studio 2025, Sanitize, Sonar   still running
```

No `CMakeLists.txt`, `meson.build`, source or test file is part of the change, so the macOS jobs are
the only ones whose environment moved. `git diff --stat` against `origin/master` lists the two macOS
workflows, `AGENTS.md`, `TODO.md` and the files of this change.

## Tasks without a test (tasks 3.4, 4.3)

No unit test comes with the change: it modifies workflow configuration and documentation only, and
the diff contains nothing under `Tests/`, `libosmscout*/`, `OSMScout*/` or `Apple/`; the suites the
macOS jobs run are the verification above.

```
$ openspec validate ci-macos-26-runner-migration --strict
Change 'ci-macos-26-runner-migration' is valid
```

The follow-up commit that records this file touches no workflow, so it re-runs the same macOS jobs on
the same workflow content; those runs are not part of the evidence above.
