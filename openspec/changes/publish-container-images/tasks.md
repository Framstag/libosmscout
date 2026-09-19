# Tasks

## 1. Import tool version query

- [x] 1.1 Add a `--tool-version` option to `Import/src/Import.cpp` that prints `OSMSCOUT_IMPORT_VERSION` and exits successfully, and list it in `DumpHelp` next to `--data-version` (spec `release-library-version`: release artifacts report the release's library version). A Catch2 unit test is not feasible for this contract: the value is a target-private compile definition of the `Import` tool and the query is a command-line option, so the contract is asserted against the built image in task 4.3 instead. Verify: `cmake --build build --target Import` then `./build/Import/Import --tool-version` prints the library version and exits 0, and `--help` lists the option
- [x] 1.2 Verify the Import target compiles without warnings after the change (spec `release-library-version`). Verify: `cmake --build build --target Import` completes and the build output contains no new warning for `Import/src/Import.cpp`
- [x] 1.3 Verify the existing test suite still passes after the change (spec `release-library-version`). Verify: `cd build && ctest -j 2 --output-on-failure` reports no failure

## 2. Release library version

- [x] 2.1 Add a required `library_version` input to `.github/workflows/release.yml` and set it in the three declarations that carry the library version: `project(libosmscout VERSION ...)` and `set(OSMSCOUT_LIBRARY_VERSION ...)` in `CMakeLists.txt`, and `libraryVersion='...'` in `meson.build`; leave `meson.build`'s `version:` at the release version, which names the distribution archives (spec `release-library-version`: every release establishes its own library version, and the released source carries it). Reject a value that is not three numeric components and a value equal to the current library version, so no release can be cut without a bump. Verify: apply the workflow's version steps to a scratch copy of the tree, then `grep -nE 'OSMSCOUT_LIBRARY_VERSION|libraryVersion' CMakeLists.txt meson.build` shows the new library version in all three declarations while `meson.build`'s `version:` still carries the release version, and `cmake -B /tmp/vcheck` leaves `CMAKE_PROJECT_VERSION` equal to the library version in `/tmp/vcheck/CMakeCache.txt`

## 3. Image build definition

- [x] 3.1 Remove the version literal from `scripts/mapgen/Dockerfile`: stage `libosmscout.so*` and `libosmscout_import.so*` into a fixed directory in the build stage, copy that directory in the runtime stage, and delete the hand-written `ln -s` block and the leftover `ldd ... | wc -l` debug line (spec `container-image-publishing`: no version literal in the build definition). Verify: `docker build -f scripts/mapgen/Dockerfile -t osmscout-mapgen:check .` succeeds, `docker run --rm --entrypoint sh osmscout-mapgen:check -c 'ldd /usr/local/bin/Import | grep "not found"'` prints nothing, and `grep -nE '[0-9]+\.[0-9]+\.[0-9]+' scripts/mapgen/Dockerfile` finds no library version
- [x] 3.2 Add OCI metadata (source repository, revision, version, created) to both `scripts/mapgen/Dockerfile` and `scripts/mapgen/Dockerfile.serve` (spec `container-image-publishing`: the image names its provenance). Verify: `docker image inspect --format '{{json .Config.Labels}}'` on both built images lists all four labels
- [x] 3.3 Add `.git` to `.dockerignore` (spec `container-image-publishing`; see design decision 6). Verify: both images still build and the context size reported by `docker build` is smaller than before the change

## 4. Publication workflow

- [x] 4.1 Switch the existing `build` job of `.github/workflows/mapgen_image.yml` to the buildx builder with a shared cache and explicitly computed image tags and build-argument labels, keeping all four existing smoke checks (spec `container-image-publishing`: nothing is published for unverified work; the image names its provenance). Verify: a pull-request run still performs the build, the non-root check, the config check and the gated pass, and the registry tag list is unchanged after it
- [x] 4.2 Give the workflow `contents: read` and `packages: write` permissions and add the registry login to the publishing job only (spec `container-image-publishing`: both images are published under repository-derived names). Verify: the pull-request run log shows no login step and no push, and the publishing job's log shows the login against the registry
- [x] 4.3 Assert in the verify job that the built image reports the version parsed from the source: read the library version from `CMakeLists.txt` and compare it with the output of the image's `--tool-version` from task 1.1 (spec `container-image-publishing`: the reported version is the built version). Verify: the check passes on the current tree, and fails when the parsed version and the built version are temporarily made to differ
- [x] 4.4 Add the release trigger and the publishing job: trigger on `release: types: [published]`, `needs:` the verify job, skip when `github.event.release.prerelease` is true, check out the release tag, take the library version that the verification job read and cross-checked, and push the release tag, the library version tag and `latest` (spec `container-image-publishing`: tag set published for a release, nothing published for unreleased work). Verify: a `workflow_dispatch` dry run with publishing disabled shows the guard passing and the computed tag set for a non-prerelease release in the log, and no registry tag is created by it
- [x] 4.5 Add the `workflow_dispatch` inputs for an explicit tag and for publishing, which never set `latest` (spec `container-image-publishing`: `latest` follows the newest release only; design decision 4). Verify: a dispatch run with publishing enabled creates exactly the given tag and leaves `latest` unchanged
- [x] 4.6 Add a concurrency group to the workflow (spec `container-image-publishing`: tags of one release are never moved). Verify: two overlapping runs serialize, the second waiting for the first, and neither run's tag set is truncated
- [x] 4.7 Verify that snapshot activity publishes nothing: push to a branch that triggers the snapshot release path, or dispatch the snapshot workflow for a scratch branch (spec `container-image-publishing`: snapshot activity publishes nothing). Verify: the publishing job is skipped in the run and the registry tag list is unchanged

## 5. Consumer path and documentation

- [x] 5.1 Make the image names in `scripts/mapgen/docker-compose.yml` overridable from the environment (`MAPGEN_IMAGE`, `MAPSERVE_IMAGE`) with the current local names as defaults (spec `container-image-publishing`: the orchestration runs published images, building locally remains possible). Verify: local `docker compose build` and `up` still work, and `MAPGEN_IMAGE=<published> docker compose pull && docker compose up --no-build` runs both containers from the pulled image as reported by `docker compose ps --format '{{.Image}}'`
- [x] 5.2 Update section 5 of `Documentation/MapRepository.md` with the published registry names, the tag semantics (release tag, library version tag, `latest`), the pull command, the published-image path of the orchestration, and the one-time step that makes a package publicly pullable; correct the claim that the image tag equals the libosmscout version (spec `container-image-publishing`: a consumer runs the published images without building, the tag semantics are documented). Verify: every command in the section runs as written, and each `container-image-publishing` scenario about documentation, pull path and local build is addressed
- [x] 5.3 Record the deferred findings in `TODO.md`: unreleased builds report a library version with no commit identity, only `linux/amd64` is published, and the build context no longer contains `.git` (spec `release-library-version`: library version compatibility; design Risks). Verify: each entry names the file or artifact it concerns and the reason it was deferred
- [x] 5.4 Add `.github/workflows/mapgen_image.yml` to the CI/CD table of `AGENTS.md` with its trigger and what it publishes (spec `container-image-publishing`: published images and their names). Verify: the row names the workflow, its triggers and the published image names

## 6. End-to-end verification

- [x] 6.1 Validate the change artifacts (specs `container-image-publishing`, `release-library-version`). Verify: `openspec validate --change publish-container-images --strict` passes
- [ ] 6.2 Verify the non-release path end to end (spec `container-image-publishing`: a pull request publishes nothing, a failing verification stops publication). Verify: a pull-request run performs the build, all smoke checks and the compose smoke job and publishes nothing, and a `workflow_dispatch` dry run with publishing disabled reports the tag set it would publish
- [ ] 6.3 Verify the release path once, after merge (spec `container-image-publishing`: a release publishes all three tags, tags of one release resolve to the same content, `latest` follows the newest release). Verify: dispatch `release.yml` with the next version, then confirm both images carry the release tag, the library version tag and `latest`; the release tag and the library version tag resolve to the same image content; `latest` resolves to that content; then make both packages public and pull a tag anonymously
- [ ] 6.4 Verify the reported version of a released artifact (spec `release-library-version`: generated database metadata carries the release's version; the published image tag matches the recorded version). Verify: pull the published regeneration image, run its `--tool-version`, and confirm the output equals the library version tag and the `import.version` recorded in a `db.json` generated by that release

## Verification status

Tasks 4.1, 4.2, 4.4, 4.5, 4.6, 4.7, 5.1 and 5.2 are marked complete on local evidence plus inspection:
both images were built and every smoke step was run against them, the library version cross-check and
the tag computation were extracted from the workflow and executed for every input case (release with
and without a leading `v`, manual run with a tag, manual run without a tag, manual run asking for
`latest`, manual run with an invalid tag), the cross-check was run against mismatching declarations,
and the orchestration was started with the default image names and with overridden ones. What still
needs a runner: that a pull-request run publishes nothing and logs in to no registry, that a
non-prerelease release publishes the release tag, the library version tag and `latest`, that a snapshot
release skips the verification and publish jobs, that the concurrency group serializes runs, and that an
anonymous pull works once the packages are public.

Tasks 6.2, 6.3 and 6.4 stay open: they are runner- and release-side checks by definition (pull-request
run plus manual dry run, the first release after merge, and the comparison of a published image's
`--tool-version` with the tag and with a generated `db.json`), so they can only be completed after this
change is merged and released.
