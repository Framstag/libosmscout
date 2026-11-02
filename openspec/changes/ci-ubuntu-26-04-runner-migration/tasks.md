# Tasks

## 1. Confirm the image and what it provides

- [x] 1.1 Confirm that `ubuntu-26.04` is a GA label of the runner-image project and that the repository
  names no deprecated label. Verify: the `Available Images` table of `actions/runner-images` lists
  `ubuntu-26.04`, and no `runs-on` value under `.github/workflows/` names `ubuntu-22.04` (spec: *A Linux
  CI job is scheduled on the Ubuntu release its workflow names*).
- [x] 1.2 Confirm that every package the Linux jobs install exists in the release of that image.
  Verify: each package page resolves for the release suite, and the three that changed name or
  provenance are noted (`catch2` transitional / `libcatch2-dev`, `clang-21` in the archive,
  Qt5 still shipped) (spec: *A dependency a workflow installs is named as the release of its image
  names it*).

## 2. Move the jobs

- [x] 2.1 Set `runs-on: ubuntu-26.04` for every Linux job of `build_javascout.yml`, `sonar.yml`,
  `webpage.yml`, `release.yml`, `release_latest.yml` and `mapgen_image.yml`. Verify: no `runs-on` value
  of a Linux job names `ubuntu-24.04` (spec: *The Linux jobs name the current LTS image*).
- [x] 2.2 Rename `build_and test_on_ubuntu_24_04.yml` to `build_and test_on_ubuntu_26_04.yml`, its
  `name:` key to `Ubuntu 26.04`, and its five jobs to `ubuntu-26.04`. Verify: the file name, the
  `name:` key and every `runs-on` value name 26.04 (spec: *A workflow file name states the release it
  runs on*).
- [x] 2.3 Rename `sanitize_on_ubuntu_24_04.yml` to `sanitize_on_ubuntu_26_04.yml` and its three jobs to
  `ubuntu-26.04`. Verify: the file name and every `runs-on` value name 26.04.
- [x] 2.4 Rename `build_on_ubuntu_22_04_qt_android.yml` to `build_on_ubuntu_26_04_qt_android.yml`, its
  `name:` key and its job to 26.04. Verify: the misnamed file is gone and no workflow file name
  disagrees with the label it runs on (spec: *A workflow file name states the release it runs on*).

## 3. Make the provisioning steps belong to the release

- [x] 3.1 Install `libcatch2-dev` in the dependency lists of the two Catch2-installing workflows
  (`build_and test_on_ubuntu_26_04.yml` × 5, `sanitize_on_ubuntu_26_04.yml` × 2, `sonar.yml`,
  `build_javascout.yml` × 2). Verify: no list of a Linux job names the transitional `catch2` package
  (spec: *The test framework is installed under its current package name*).
- [x] 3.2 Drop the `apt.llvm.org` `noble` repository from the memory sanitizer job and install
  `clang-21` from the archive of the image's release. Verify: the job adds no third-party package
  repository and still installs `clang-21` and `libclang-21-dev` (spec: *The compiler comes from the
  release of the image*).

## 4. Documentation and helpers

- [x] 4.1 Update the build-status table of `README.md`: the renamed workflow files, the stale `22.04`
  rows and the `vs2019` row. Verify: every workflow file the table links exists (spec: *Documentation
  names the workflows that exist*).
- [x] 4.2 Update the workflow table of the CI/CD section of `AGENTS.md` and add the note that the Linux
  jobs name their image version, why 26.04, and the two provisioning differences to 24.04. Verify: the
  table names the renamed files and the note names the image (spec: *Documentation names the workflows
  that exist*).
- [x] 4.3 Point the `.pi/skills/verify-cairo-no-pango/SKILL.md` references at the renamed workflows and
  the `libosmscout/ubuntu_22.04_*` image tags of `ci/docker/ubuntu_24.04_*` at the directory they live
  in. Verify: neither file names a workflow file or an image tag that does not exist (spec:
  *Documentation names the workflows that exist*).
- [x] 4.4 Validate the change: `openspec validate ci-ubuntu-26-04-runner-migration --strict`. Verify:
  the command reports the change as valid.

## 5. Verify on the image

- [x] 5.1 Open the pull request and let the whole matrix run on `ubuntu-26.04`. Verify: every Linux
  workflow that triggers on a pull request concludes successfully.
- [x] 5.2 Read the configure log of the gcc/cmake job and confirm that the test suite was built. Verify:
  the log does not contain `Disabled building of tests, since Catch2 dependency is not available`, and
  `ctest` reports the test count (spec: *The test framework is installed under its current package
  name*).
- [x] 5.3 Read the memory sanitizer log and confirm that the toolchain came from the archive. Verify:
  `clang-21` is installed, the allocator check passes, and no `apt.llvm.org` step remains (spec: *The
  compiler comes from the release of the image*).
- [x] 5.4 Record the run identifiers and the conclusions in `verification.md`. Verify: each task of
  section 5 has its run id and result.
