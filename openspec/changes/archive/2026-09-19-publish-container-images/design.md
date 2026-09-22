# Design

## Context

See `proposal.md` - Why for motivation. Constraints that shape the approach:

- The image build and its smoke checks already exist in `.github/workflows/mapgen_image.yml`: a `verify`
  job (`docker build` + non-root check + `--check-config` + a refresh-gated single pass) and a `compose`
  job (local compose build + nginx HTTP checks).
- A version the images can be tagged with is declared in two places with different meanings:
  `meson.build`'s project `version:` (today the literal string `'latest'`, set to the release version by
  `release.yml` in its working tree only, and used to name the distribution archives) and the library
  version in `set(OSMSCOUT_LIBRARY_VERSION ...)` / `libraryVersion='...'`, which is what the import tool
  reports and what `db.json` records as `import.version`. `cmake/ProjectConfig.cmake:81-84` derives the
  soname major from `project(libosmscout VERSION ...)`, so a date-shaped library version would change the
  soname from `libosmscout.so.1` to `libosmscout.so.2026` and break downstream links.
- `release.yml` creates the release with `JRELEASER_GITHUB_TOKEN: ${{ secrets.GITHUB_TOKEN }}`, and GitHub
  starts no workflow run for events caused by the default token, so a `release: published` trigger never
  fires for it (observed: the release object exists, the trigger was on master, and
  `gh run list --workflow mapgen_image.yml --event release` was empty).
- Branch protection on `master` requires one approving review with `enforce_admins: false` and no bypass
  list, so a workflow pushing with `GITHUB_TOKEN` cannot write to `master`; a human can.
- `scripts/mapgen/Dockerfile` names the library files by a version literal (`libosmscout.so.1.1.1`) and
  rebuilds the symlink chain by hand, so the first version bump breaks the image build.
- The build context excludes heavy local artifacts but not `.git` (103 MB locally).

## Goals / Non-Goals

**Goals:**

- Every merge or direct commit on the main branch that can change the image content is pullable, without
  anyone building the repository.
- The tags are simple: the version the source declares, `latest`, and a per-build stamp the user can pin;
  old builds are pruned automatically.
- The version a database records identifies the release that produced it.

**Non-Goals:**

- Architectures other than `linux/amd64`.
- Immutable release images, or any behaviour that distinguishes a release image from any other image. A
  release is a version milestone: it changes the version later builds carry.
- A commit identity inside the reported library version (recorded in `TODO.md`).
- Changing what the images contain, how they run, or their mounts - `mapgen-container` and `web-server`
  keep their requirements.
- Signing (cosign/notation) beyond the build provenance attestations.

## Decisions

### 1. Verify in one job, publish in a second job that depends on it

```
push to master (image inputs) | manual run with publishing
        |
        v
+---------------------------+        +-----------------------------+
| verify                    | needs  | publish                     |
| checkout                  |------->| read the release version     |
| read + cross-check the    |        | build the tags, log in       |
|   library version         |        | buildx build --push          |
| buildx build --load       |        | prune old builds             |
| smoke: uid, --tool-version|        +--------------+--------------+
| smoke: --check-config     |                       v
| smoke: refresh-gated pass |     ghcr.io/framstag/libosmscout/mapgen:<tag>
+---------------------------+     ghcr.io/framstag/libosmscout/mapserve:<tag>
```

- Alternative - one job: `buildx build --load`, run the smoke checks, then `docker push` the loaded tags.
  No duplicate compilation, but `--load` and `--push` are mutually exclusive, so attestations cannot be
  produced, and the tags exist before the checks have passed.
- Alternative - push under a temporary tag, verify by pulling it, then promote. One compilation and
  attestations survive, but the registry briefly holds an unverified tag that must be deleted afterwards.
- Rationale: "no unverified image is ever published" is the contract, and the second build is cheap
  because both jobs share `type=gha` cache scope: the compile of the core and import libraries is cached
  and only the runtime stage is re-executed.

### 2. The rolling tag scheme

Every publication carries, for each image:

| tag | meaning | moves? |
|-----|---------|--------|
| `<release version>` | the version the published source declares (`meson.build`'s project version), e.g. `2026.01.15.1` | yes, with every publication of that version - and the next milestone changes the value |
| `latest` | the newest publication of any kind | yes |
| `<UTC build stamp>` | e.g. `20260919T143512Z`, one per publication | no |

The stamp is what makes a concrete build pullable: it is unique, it never moves, and it is the tag to pin
when a deployment has to be reproducible. Old builds are pruned (decision 3), so a stamp remains pullable
for a bounded number of publications - which the documentation states.

- Alternative - an immutable tag per release (`:<release version>` never moved by later builds): that is
  the classic model, but it needs a release event that is known not to fire for this repository
  (Context), or a dispatch from `release.yml`, and it makes every user wait for a release to get a current
  image.
- Alternative - a `sha-<short>` tag instead of a timestamp: equally immutable and cheaper to relate to a
  commit, but less readable for a human who wants to know how old a build is, and the source revision is
  in the image metadata and the provenance attestation anyway.
- Alternative - also publish the library version as a tag: then a `db.json` in hand could find an image by
  the version it records, but the tag would move with every milestone rather than with every build, so it
  identifies a development period rather than a build; the stamp does the pinning job better.
- Rationale: two moving tags for "what is current" and one immutable tag per build for "what I tested" is
  the smallest set that serves both needs.

### 3. Old builds are pruned by a job that runs after publication

After a publication, a cleanup job deletes the oldest package versions of each image, keeping the newest
ones up to a configured count (initial value: 20 per image, which is a few weeks of merges). The official
`actions/delete-package-versions` action does this with `packages: write`, which the publishing job already
needs, so no additional credential or script is involved. Deleting an old package version removes its
stamp tag; `latest` and the release version tag always point at the newest publication, which is never a
deletion candidate.

Alternative considered: a scheduled cleanup workflow - rejected, because pruning right after publishing
keeps the package bounded at all times without a second schedule to maintain.

### 4. What publishes: a main-branch commit and a manual run

The publish job runs for a `push` to the main branch whose path filter matched, and for `workflow_dispatch`
with a `push` input. A pull request never publishes. There is no release-specific publication: the commit
that sets a new release version is an ordinary main-branch commit, and it publishes like any other, so a
release needs no dispatch, no release-event trigger, and no `actions: write`.

- Alternative (implemented first, then withdrawn as unnecessary) - a `release: published` trigger plus a
  dispatch from `release.yml`: it exists to publish a tag set owned by a release, which this model does not
  have.
- Alternative - publish on tags, or only on releases: covered by the previous alternative.
- Rationale: one path, one tag rule, no event semantics to depend on. The path filter stays, because a
  commit that changes no image input cannot change the image content.

### 5. Version handling: the release version is declared, the library version travels in a version bump

The image tag comes from `meson.build`'s project `version:` (parsed by the workflow, with a build stamp as
the only tag when the source declares no parseable version, which is the case while it still says
`latest`). The library version stays a separate number for exactly the reason it was introduced: `db.json`
records it, so two releases must not report the same value, and the soname major must not follow a date.

Because branch protection prevents a workflow from pushing to the main branch with `GITHUB_TOKEN`, the
version changes are made by a human, in a pull request, and `release.yml` asserts them instead of applying
them:

```
1. PR   chore: release <version>, library <Y>     sets meson.build's version: and both library
   |                                              version declarations; merging it publishes
   v                                              :<version>, :latest and a build stamp
2. release.yml  <version> <Y>                     asserts the revision declares both, builds the
                                                  archives, creates the release and its tag
```

`release.yml` keeps its `version` and `library_version` inputs but no longer rewrites the library version
in its working tree, so a revision that does not declare the version it is told to release fails the run
instead of producing archives that disagree with the tag they came from.

- Alternative - let `release.yml` commit the bump (the earlier design): fewer human steps, but it needs a
  bypass entry for the Actions app or a PAT secret, and it makes the release workflow a writer of the main
  branch.
- Alternative - pass the released library version to the image build as a build argument: the image would
  report a version its own source does not declare, so "building the released source reproduces the
  reported version" would stop being true.
- Residual: the version bump is a human step, so a revision can be released that declares an older
  version; the assertion turns that into a failed run rather than a wrong tag, and until a milestone sets a
  version the images carry `latest` and the build stamp only, which the run reports.

### 6. The image build definition names no version, and the orchestration takes image names from the
environment

The build stage copies `libosmscout.so*` and `libosmscout_import.so*` into `/stage/lib` with their symlink
chain intact and the runtime stage copies that directory, so the version lives only in the build system;
`.dockerignore` keeps `.git` out of the context. Both Dockerfiles declare OCI metadata (`ARG`/`LABEL`),
which the publishing job fills from the revision it publishes. `docker-compose.yml` reads `MAPGEN_IMAGE`
and `MAPSERVE_IMAGE`, defaulting to the local build, so the published images can be run with `pull` and
`up --no-build`.

- Alternative - `ARG OSMSCOUT_VERSION` plus `COPY .../libosmscout.so.${OSMSCOUT_VERSION}`: keeps the
  explicit file list but reintroduces the coupling this change removes, and the argument can disagree with
  what was built.
- Alternative - a second, standalone published compose file: explicit pinning without environment
  variables, but it duplicates every service definition and will drift from the base file.
- Rationale: one place knows the version (the build system), one file describes the orchestration.

## Risks / Trade-offs

- A version tag does not identify released source: later builds of the same version overwrite it. Pinning a
  concrete build means using the stamp tag, which is what the documentation recommends → stated in
  `Documentation/MapRepository.md` and in the spec's documentation scenario.
- A pinned build is pruned eventually → the retention count is documented as the pullable lifetime of a
  stamp, and keeping the newest 20 publications is generous for a deployment that picks a build and pins
  it.
- Every main-branch commit that touches an image input now pays for a publication in addition to the
  build, and the registry also receives a new package version per publication → the `type=gha` cache makes
  the second build mostly cache hits, and the pruning job bounds what accumulates.
- The first publication needs a one-time visibility change (a new package is private) → documented next to
  the pull instructions, and the publish job prints the package settings links.
- Pruning deletes package versions with `packages: write`; if the token turns out to lack that ability for
  deletions, the job reports the failure instead of publishing a broken state, and the fallback is a
  dedicated cleanup workflow.
- The version bump is a human step → the release assertion fails loudly, and a missing bump only means the
  images carry `latest` and the stamp, reported in the run summary.
- Attestation manifests turn the pushed artifact into an image index, which some older clients handle
  imperfectly → provenance and SBOM are each a single flag; if they cause trouble, drop `sbom` first.
- `Import/src/Import.cpp` has 25 pre-existing uncrustify deviations and hundreds of clang-tidy findings;
  the added option follows the file-local style and adds no new kind of finding.
- Removing `.git` from the build context breaks any future build step that reads git history (nothing does
  today) → recorded in `TODO.md`.
- Only one architecture is published: an `arm64` consumer cannot pull → recorded in `TODO.md`.

## Migration Plan

1. Merge this change: the next main-branch commit that touches an image input publishes for the first time
   and creates both packages (private).
2. Flip both packages to public once, then verify an anonymous `docker pull` of `:latest` and of the newest
   stamp.
3. At the next release: merge the `chore: release <version>, library <Y>` pull request (which publishes the
   new version tag), then run `release.yml` with the same two values.
4. Rollback: the registry is additive - reverting the workflow stops publication and leaves existing tags
   intact. The pruning job only ever deletes old builds, never `latest` or a version tag, and the workflow
   can be disabled in the repository settings if it misbehaves.

## Open Questions

None that change the specs, the approach or the task list. The retention count is a value in the workflow
and in the documentation, easy to change on its own.
