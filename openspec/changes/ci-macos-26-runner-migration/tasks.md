# Tasks

Spec: `specs/ci-build-fixes/spec.md` (delta to the `ci-build-fixes` capability). Design: `design.md`.

## 1. The runner image of the macOS jobs

- [x] 1.1 Record where the repository names `macos-14` and when it was last scheduled green: the `meson` and `cmake` jobs of `.github/workflows/build_and test_on_osx.yml` and the `cmake` job of `.github/workflows/build_and test_on_ios.yml`, last successful runs 2026-11-01 13:37 UTC and 13:46 UTC, every later macOS job `cancelled` with `steps: 0`. Verify: the job names, the timestamps and the cancelled-job signature are written down in `design.md` - Context. (Spec: "A CI job is scheduled on a runner image that is still supported".)
- [x] 1.2 Move both jobs of `.github/workflows/build_and test_on_osx.yml` to `runs-on: macos-26`. Verify: `grep -n "runs-on" .github/workflows/build_and\ test_on_osx.yml` names `macos-26` twice and no other label, and the file parses as YAML. (Spec: "The macOS jobs move with the image they name".)
- [x] 1.3 Move the job of `.github/workflows/build_and test_on_ios.yml` to `runs-on: macos-26`. Verify: `grep -n "runs-on" .github/workflows/build_and\ test_on_ios.yml` names `macos-26`, and the file parses as YAML. (Spec: "The macOS jobs move with the image they name".)
- [x] 1.4 Confirm no workflow of the repository names a deprecated or retired label, and that no macOS job uses `macos-latest`: `grep -rn "runs-on" .github/workflows/`. Verify: no line names `macos-14`, `macos-14-large`, `macos-14-xlarge` or `macos-latest`. (Spec: "No job names a deprecated or retired image label", "A macOS job names an image version, not the latest alias".)

## 2. The deployment target of the iOS check

- [x] 2.1 Raise the iOS job's deployment target to the floor of its coming toolchain and state the reason next to it. Verify: the configure step passes `-DCMAKE_OSX_DEPLOYMENT_TARGET=15` and the step carries a comment naming Xcode 26 and iOS 15. (Spec: "The iOS check configures the floor of its toolchain".)
- [x] 2.2 Record the difference between that value and the deployment target of the shipped Apple projects - iOS 12.0/15.0 in `Apple/OSMScoutOSX/OSMScoutOSX.xcodeproj/project.pbxproj` - as an open item. Verify: `TODO.md` names the workflow, the value 15 and the values the projects declare. (Spec: "A check above the shipped deployment target is recorded".)

## 3. Build and test verification (the pull request's own runs, the only environment with a macOS 26 runner)

- [x] 3.1 Confirm the OS X `meson` job configures, builds and runs its test set on `macos-26`. Verify: the job's conclusion and its `meson test` summary on the pull request.
- [x] 3.2 Confirm the OS X `cmake` job configures, builds and runs its test set on `macos-26`. Verify: the job's conclusion and its `ctest` summary on the pull request.
- [x] 3.3 Confirm the iOS job configures and builds against the iOS 26.5 SDK with deployment target 15. Verify: the job's conclusion and the configure output of the pull request.
- [x] 3.4 Confirm no unit test is required for this change: it modifies workflow configuration only, so no new Catch2 test is added; the existing suites run inside the macOS jobs and are the verification (tasks 3.1, 3.2). Verify: the diff contains no file under `Tests/`, `libosmscout*/`, `OSMScout*/` or `Apple/`.
- [x] 3.5 Confirm no other workflow is affected and both build systems are untouched: the Ubuntu, MSYS, Visual Studio, Android, sanitizer, Sonar and JavaScout jobs name labels this change does not touch, and no `CMakeLists.txt` or `meson.build` changes. Verify: `git diff --stat` against the base revision lists the two macOS workflows, the change's own files and `AGENTS.md`/`TODO.md` only.

## 4. Documentation

- [x] 4.1 Add a note to the CI/CD section of `AGENTS.md` that the macOS jobs name their image version, that `macos-26` is the current GA arm64 image, and why the iOS job's deployment target is 15. Verify: the note exists and names the image and the deployment target.
- [x] 4.2 Record the follow-ups this change does not take in `TODO.md`: the arm64 Homebrew prefix the OS X jobs' `PKG_CONFIG_PATH` contradicts, the deprecated `qt@5` formula both jobs install, and the deployment target difference of task 2.2. Verify: each entry carries an id, a category and a status, and names the file it is about.
- [x] 4.3 Validate the change: `openspec validate ci-macos-26-runner-migration --strict`. Verify: the command reports the change as valid.
