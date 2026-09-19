# Tasks

## 1. Earlier work kept by this change

- [x] 1.1 Add `--tool-version` to the import tool, so a published image can be asked which version it reports (spec `container-image-publishing`: an image reports the version it was built from). Verify: `./build/Import/Import --tool-version` prints the library version and exits 0, and `--help` lists the option. A Catch2 unit test is not feasible for a target-private compile definition behind a command-line option; the contract is asserted against the built image in task 2.2
- [x] 1.2 Remove the version literal from the image: stage the libraries with their symlink chain in the build stage, drop the hand-written symlink block and the leftover debug output, add OCI metadata to both Dockerfiles, and keep the repository history out of the build context (spec `container-image-publishing`: no version literal in the build definition). Verify: both images build, `ldd` reports no missing library, the symlink chain is intact inside the image, the Dockerfile contains no version, all four OCI labels are present, and the build context drops from 107.97 MB to 58.17 MB
- [x] 1.3 Let the orchestration take the image names from the environment (spec `container-image-publishing`: the orchestration runs published images, building locally remains possible). Verify: the default names still build and run locally, and overridden names are used by `docker compose up --no-build`
- [x] 1.4 Verify the C++ change against the build and the test suite (spec `container-image-publishing`). Verify: `cmake --build build --target Import` compiles without new warnings and `ctest -j 2` reports 120 of 120 tests passing

## 2. The publication workflow

- [x] 2.1 Make the workflow publish on a commit to the main branch that changes an image input, and on a manual run that asks for it, while a pull request never publishes (spec `container-image-publishing`: publication follows the revision and requires verification). Verify: the job conditions and the path filter inspected against the spec scenarios, and a pull-request run on the runner published nothing
- [x] 2.2 Derive the tags from the published source: the release version the source declares, `latest`, and a date and time stamp unique to the build; keep the build and the smoke checks as the gate, and report when the source declares no release version (spec `container-image-publishing`: the tags a publication carries). Verify: the tag step extracted from the workflow and run for a source with a declared version, for a source without one, and for a manual run, always emitting the stamp and never a duplicate tag
- [ ] 2.3 Add the pruning job that keeps only the newest builds of each image and deletes older package versions (spec `container-image-publishing`: old builds are pruned). Verify: after more publications than the retention count, the oldest stamp tags are gone, the newest kept ones are pullable, and `latest` and the release version tag still resolve to the newest publication
- [ ] 2.4 Verify the first publication on the runner: the next commit to the main branch that touches an image input publishes both images, the run summary lists the tags, and the packages appear (private until made public) (spec `container-image-publishing`: a merge on the main branch publishes)

## 3. Version handling at release time

- [x] 3.1 Turn the release workflow's version editing into an assertion: fail when the revision does not declare the release version and the library version the run is told to release, and stop rewriting the library version in the working tree (spec `release-library-version`: the released revision declares the released version; a revision that declares another version is not released). Verify: the step extracted from the workflow and run against a scratch copy passes when the revision declares both versions and fails, naming both, when either differs
- [x] 3.2 Document the version bump procedure: the single `chore: release <version>, library <Y>` pull request, that merging it publishes the new version tag, and what the release run asserts (spec `release-library-version`: every release establishes its own library version). Verify: section 5 of `Documentation/MapRepository.md` names the pull request, what it sets, and the order of the bump, the merge and the release run
- [ ] 3.3 Verify a release end to end on the next release: the version bump pull request is merged, its merge publishes the new version tag, the release run asserts both versions, and a database generated with the released image records the released library version (spec `release-library-version`: release artifacts report the release's library version)

## 4. Documentation and findings

- [x] 4.1 Document the published names, the tag semantics including which tag pins a build and how long a pinned build lasts, and the one-time step that makes a package publicly pullable (spec `container-image-publishing`: the tag semantics are documented). Verify: section 5 of `Documentation/MapRepository.md` states each of those, and every command in it runs as written
- [x] 4.2 Record the deferred findings in `TODO.md`: the reported library version carries no commit identity, only `linux/amd64` is published, the build context no longer contains `.git`, the retention count bounds how long a pinned build stays pullable, and the version bump is a human step (spec `container-image-publishing`, spec `release-library-version`). Verify: each entry names what it concerns, why it exists and what would close it
- [x] 4.3 Update the CI/CD table in `AGENTS.md` for the image workflow (spec `container-image-publishing`: published images and their names). Verify: the row names the workflow and what it publishes

## 5. Verification

- [x] 5.1 Validate the change artifacts (both specs). Verify: `openspec validate --change publish-container-images --strict` passes
- [ ] 5.2 Pull a published image anonymously once the packages are public, and run it: the documented pull path works, the image completes a pass, and `--tool-version` prints the library version the run summary reported (spec `container-image-publishing`: anonymous pull, the orchestration runs published images)
- [ ] 5.3 Observe a pruning run on the runner after enough publications, and confirm a build stamp that fell out of the retention window is no longer pullable while `latest` and the release version tag are (spec `container-image-publishing`: old builds are pruned)

## Verification status

Merged earlier as `publish-container-images` (PR #1804, merge `fe874d226`): the Import version query, the
image build definition without a version literal, the OCI metadata, the `.dockerignore` entry, the compose
image names, the first documentation pass, the `AGENTS.md` row and the `TODO.md` findings. Runner evidence
from that pull request: every image check passed (the library version cross-check reported `1.1.1`, the
image reported `1.1.1`, the non-root check, the config check, the refresh-gated single pass and the compose
smoke job), no registry login ran and nothing was published. The 120 of 120 test suite and the clean
Import build were verified locally before that merge.

Two designs were implemented and then withdrawn in this branch, both recorded in the design's decisions:
a `release: published` trigger with a dispatch from `release.yml` (it exists to publish a tag set owned by
a release, which the rolling model has no place for) and a development library version with its own tag
(the stamp tag serves the same purpose - naming a concrete build - without a second moving version).
