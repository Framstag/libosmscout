# Design

## Context

See `proposal.md` - Why for motivation. Constraints that shape the approach:

- The image build and its smoke checks already exist in `.github/workflows/mapgen_image.yml`: one
  `build` job (`docker build` + non-root check + `--check-config` + a refresh-gated single pass) and
  one `compose` job (local compose build + nginx HTTP checks). Neither has a `permissions:` block,
  a registry login, or a push step.
- Releases are produced by two workflows. `.github/workflows/release.yml` (manual, version input)
  produces a real release; `.github/workflows/release_latest.yml` runs on **every** push to `master`
  and produces a snapshot release. Observed through the GitHub API, the snapshot release carries the
  moving tag `latest` and `prerelease=true`; real releases carry `v<chronver>` and
  `prerelease=false`. `jreleaser.yml` sets `versionPattern: CHRONVER`, `release.github.overwrite: true`.
- The version an image can be matched against is not currently meaningful: `Import/CMakeLists.txt:13`
  defines `OSMSCOUT_IMPORT_VERSION` from `OSMSCOUT_LIBRARY_VERSION`, `Import/src/Import.cpp:466`
  records it in `db.json`, and that value has been `1.1.1` at every release since before
  `v2023.03.30.1` (`CMakeLists.txt:3` is unchanged across those tags). `meson.build:3` carries the
  literal string `'latest'`.
- `scripts/mapgen/Dockerfile` names the library files by a version literal
  (`libosmscout.so.1.1.1`) and rebuilds the `.so` / `.so.1` symlink chain by hand, so the first
  version bump breaks the image build.
- `cmake/ProjectConfig.cmake:81-84` sets `VERSION ${OSMSCOUT_LIBRARY_VERSION}` and
  `SOVERSION ${PROJECT_VERSION_MAJOR}`: a year-leading library version would change the soname from
  `libosmscout.so.1` to `libosmscout.so.2024` and break downstream links every release.
- The build context excludes heavy local artifacts but not `.git` (103 MB locally).

## Goals / Non-Goals

**Goals:**

- Every released source revision has pullable images whose tags state which release and which library
  version they are, without anyone building the repository.
- The registry never receives an image that failed the existing smoke checks, and pull requests and
  snapshot activity never move a tag.
- The library version reported by artifacts identifies the release, and the image build definition is
  free of version literals.

**Non-Goals:**

- Architectures other than `linux/amd64`.
- Publishing images for unreleased commits as a supported, permanently available tag.
- A commit identity inside the reported library version for unreleased builds (recorded in `TODO.md`).
- Changing what the images contain, how they run, or their mounts - `mapgen-container` and
  `web-server` keep their requirements.
- Signing (cosign/notation) beyond the build provenance attestations.

## Decisions

### 1. Publication is triggered by the release event, not by tags or a workflow call

Chosen: `on: release: types: [published]`, with the publish job skipped when
`github.event.release.prerelease` is true, and the checkout pinned to
`ref: ${{ github.event.release.tag_name }}`.

- Alternative - `on: push: tags: ['v*']`: existing tags are inconsistent (`v2024.06.02.1`, `1.1.0`,
  `v1.0.0`) and the snapshot release reuses the moving tag `latest`, so a tag filter cannot reliably
  separate a release from a snapshot, and it would publish from whatever the tag points at even if
  that is not the released revision.
- Alternative - make the image workflow `workflow_call` and invoke it from `release.yml` and
  `release_latest.yml`: explicit and it can carry the library version as an input, but it couples the
  image build to both release workflows, needs two call sites kept in step, and delays publication
  until after the release step in those workflows.
- Rationale: the event fires exactly when a release becomes visible, `prerelease` is set by the
  producer rather than inferred, and neither release workflow needs a call site - `release.yml` only
  gains the library-version step it needs anyway (decision 3).
- Risk handled explicitly: a workflow triggered by a release event checks out the default branch
  unless a ref is given, which would publish the wrong source. The checkout therefore names the
  release tag.

### 2. Verify in one job, publish in a second job that depends on it

```
release published (prerelease=false)
        |
        v
+---------------------------+        +-----------------------------+
| verify                    | needs  | publish                     |
| checkout ref = tag_name   |------->| guard: versions consistent  |
| buildx build --load       |        | buildx build --push         |
|  cache-from/to type=gha   |        |  cache-from/to type=gha     |
| smoke: uid == 1000        |        |  provenance + sbom          |
| smoke: --check-config     |        |  metadata: source, revision,|
| smoke: refresh-gated pass |        |            version, created |
+---------------------------+        +--------------+--------------+
                                                    v
                              ghcr.io/framstag/libosmscout/mapgen:<tag>
                              ghcr.io/framstag/libosmscout/mapserve:<tag>
```

- Alternative - one job: `buildx build --load`, run the smoke checks, then `docker push` the loaded
  tags. No duplicate compilation, but `--load` and `--push` are mutually exclusive, so attestations
  (provenance/SBOM) cannot be produced, and the tags exist before the checks have passed.
- Alternative - push under a temporary tag, verify by pulling that tag, then promote with
  `buildx imagetools create`. One compilation and attestations survive, but the registry briefly holds
  an unverified tag that must be deleted afterwards, and promotion adds a failure mode of its own.
- Rationale: the registry contract is "no unverified image is ever published", and the second build is
  cheap because both jobs share `type=gha` cache scope `mapgen`: the compile of core + import library is
  cached, only the runtime stage is re-executed.

### 3. The library version is derived from the released source, with a consistency guard

The verification job reads the library version from the released source, cross-checks its two
declarations against each other and passes the value to the publish job. The
library version lives in two declarations - `set(OSMSCOUT_LIBRARY_VERSION ...)` in `CMakeLists.txt`
(line 5, the value the image build compiles into `OSMSCOUT_IMPORT_VERSION` via
`Import/CMakeLists.txt:13`) and `libraryVersion='...'` in `meson.build` (line 8, used by
`Import/meson.build:6`) - and neither of them is `project(libosmscout VERSION ...)` or `version:`, which
carry the project version that drives the soname major and the release version that names the
distribution archives. The cross-check requires the two declarations to agree, so both build systems of
the released revision report the same version; they agree today (`1.1.1`), so it is green on every path.
It lives in the verification job rather than in the publish job because publication already depends on
verification, so the check runs once, fails the run before any image is built or any tag exists, and the
verified value is a job output the publish job consumes.

- Alternative - pass the library version as a `workflow_dispatch`/`workflow_call` input: explicit, but
  it can disagree with what the image actually compiled in, and the release event cannot carry it.
- Alternative - parse nothing and take the version from the tag name: the release tag is the
  chronological version, which the spec deliberately keeps distinct from the library version.
- Rationale: the tag must equal what the image reports (spec `release-library-version`), and the only
  trustworthy source for "what the image reports" is the source the image is built from. The guard
  turns a silent divergence between the two build systems into a failed publish.
- Residual risk: the guard compares two build files, so a version that is wrong but identical in both
  would still publish. The image itself is therefore interrogated too (decision 8): the verify job asks
  the built image for the version it reports and compares it with the version it parsed from the source.

### 4. Snapshots and non-release builds never publish; dispatch publishes explicitly

The publish job runs for a non-prerelease release and for `workflow_dispatch` with an explicit
`push` input plus tag; nothing else. `master` pushes and the snapshot release they create publish
nothing, so `:latest` always means the newest real release. A dispatch run never sets `latest`; it
publishes only the tags it was given, so the meaning of `latest` cannot be changed by hand.

- Alternative - automatic `:master` and `:sha-<short>` tags on every `master` push whose path filter
  matches (`libosmscout/**` matches nearly every merge): always a fresh image, but permanent mutable
  tags appear in the published package and every merge pays a full image build.
- Alternative - publish snapshots under the snapshot release's version: the snapshot release is
  recreated on every `master` push with `overwrite: true`, so its identity moves; a tag built from it
  would not be reproducible.
- Rationale: the tag set stays small and every tag is either immutable or has a documented meaning.

### 5. The compose orchestration takes the image names from the environment

`scripts/mapgen/docker-compose.yml` keeps `build:` and its `:local` names as defaults, but reads
`MAPGEN_IMAGE` / `MAPSERVE_IMAGE` if set. Consuming published images is `docker compose pull` followed
by `up --no-build` (or plain `up`, which pulls a missing image).

- Alternative - a second, standalone `docker-compose.published.yml`: explicit pinning without
  environment variables, but both service definitions, volume names and ports are duplicated and will
  drift from the base file.
- Alternative - drop `build:` and always pull: simplest consumer story, but it breaks local
  development and the `compose` smoke job in `mapgen_image.yml`, which builds both images.
- Rationale: one file remains the single description of the orchestration, the existing smoke job keeps
  working unchanged, and the published-image path needs no new file.

### 6. Library files are staged in the build stage instead of being named by a version

The build stage copies `libosmscout.so*` and `libosmscout_import.so*` into `/stage/lib` with their
symlink chain intact; the runtime stage does `COPY --from=build /stage/lib/ /usr/local/lib/`. The
hand-written `ln -s` block disappears. `.dockerignore` gains `.git`.

- Alternative - `ARG OSMSCOUT_VERSION` plus `COPY .../libosmscout.so.${OSMSCOUT_VERSION}`: keeps the
  explicit file list, but reintroduces exactly the coupling this change removes, and the argument can
  disagree with what was built.
- Alternative - a glob directly in `COPY --from=build .../libosmscout.so* /usr/local/lib/`: one line,
  but whether symlinks are copied as symlinks or dereferenced is easy to get wrong, and a missing file
  does not fail the build.
- Rationale: `cp -a` into `/stage/lib` fails loudly when a library is missing, preserves the
  `.so -> .so.1 -> .so.1.1.1` chain that `ldconfig` and the `ldd` check depend on, and leaves the
  version in exactly one place: the build system.

### 7. Build and push with buildx and a shared cache

Both jobs use `docker/build-push-action` with `load: true` in the verify job and `push: true` in the
publish job, `cache-from`/`cache-to: type=gha,scope=<image>`, and `provenance: true` with `sbom: true`
on the push. The tag list is computed by a shell step instead of by `docker/metadata-action`, and the
OCI labels come from the Dockerfile's own `ARG`/`LABEL` declarations instead of being injected as
buildx labels.

- Alternative - `docker/metadata-action` for tags and labels: conventional, and it lower cases the
owner name automatically, which a container registry requires and GitHub expressions cannot express.
Rejected because this workflow can only be executed on a runner, so a shell step that can be extracted
and run locally against every input case is worth more here than a third-party action whose label
handling overlaps with the Dockerfile's own labels; the lower casing is one `tr` call, and the tag rules
(three tags for a release, one for a manual run, `latest` never moved by hand) have to be expressed
anyway.
- Alternative - keep `docker build` in the verify job and add `docker push` afterwards: no cache
  sharing between the verify and publish jobs (the compile would run twice at full cost), and
  attestations are impossible with the docker image store.
- Rationale: the compile dominates the build time, and cache sharing is what makes decision 2
  affordable.

### 8. The bundled import tool can be asked which version it reports

`Import/src/Import.cpp` gains an option that prints `OSMSCOUT_IMPORT_VERSION` and exits, listed in the
`--help` output next to the existing `--data-version`. The verify job uses it to assert that the built
image reports the same version the publish job parsed from the source, and a unit test in `Tests/`
covers the option.

- Alternative - rely on the source alone (decision 3's guard only): no code change, but a tag can then
  disagree with the image it was published for, and nothing an operator pulls can be interrogated.
- Alternative - read the version out of a `db.json` produced by a real import in CI: end to end, but it
  requires source data and a full import per build, which the current smoke job deliberately avoids by
  pre-seeding the refresh gate.
- Rationale: the version a released artifact reports is part of the contract (spec
  `container-image-publishing`, `release-library-version`), and a query is the cheapest way to make that
  contract checkable against the artifact rather than against the source of the artifact.

## Risks / Trade-offs

- First publication requires a one-time visibility change; until it is done the package is private and
  anonymous pulls fail with 401 → documented next to the pull instructions, and the publish job prints
  the package URL. The workflow itself is identical for a public and a private package.
- The snapshot release fires on every `master` push, so the release-event workflow is triggered often →
  the publish job is skipped by its `prerelease` condition and costs a skipped job only.
- A release tag that is later moved or force-updated would leave published tags pointing at different
  content than the tag does → releases are append-only by policy; nothing in this change makes it worse,
  and the publish job's provenance attestation records the revision each tag was built from.
- Two builds per release: if the cache is evicted, the publish job pays a full compile → cache scope is
  pinned per image, and the verify job already warmed it moments earlier.
- Attestation manifests turn the pushed artifact into an image index, which some older clients handle
  imperfectly → provenance and SBOM are each a single flag; if they cause trouble, drop `sbom` first.
- The consistency guard fails a release whose two build files disagree → that is the intent; the
  `release.yml` task sets both, so the guard is green from the first release after this change.
- The library version tag moves whenever a release bumps the library version, so pinning `:<library
  version>` is not reproducible across releases → `:<chronological version>` is the reproducible tag,
  and the documentation states which tag is which.
- Removing `.git` from the build context breaks any future build step that reads git history (nothing
  does today) → noted in `TODO.md`-adjacent documentation of the image build.
- Only one architecture is published: an `arm64` consumer cannot pull → recorded in `TODO.md`.

## Migration Plan

1. Merge the `release.yml` library-version step together with the workflow changes, so the guard has
   consistent sources from the first release.
2. Validate the whole path without creating a permanent tag: `workflow_dispatch` with `push` disabled
   (build, smoke checks, tag computation and the guard all run; nothing is published).
3. At the next release, the images appear under the chronological version, the library version and
   `latest`. Then flip the two packages to public once and verify an anonymous `docker pull`.
4. Rollback: the registry is additive - reverting the workflow file stops publication and leaves
   existing tags intact. Tags can be deleted manually in the package settings; no consumer of the
   repository is affected by a revert, because nothing in the pipeline reads the registry.

## Open Questions

None. Both questions raised during design are resolved: the import tool gains a version query
(decision 8), and a `workflow_dispatch` run publishes only the tags it is given and never sets `latest`
(decision 4).
