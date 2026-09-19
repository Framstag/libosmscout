# Proposal

## Why

The regeneration container and the repository web server container are built and smoke-tested in CI, but
they are only obtainable by building this repository: nothing publishes them, so an operator who wants to
run the map repository pipeline has no artifact to pull. At the same time the version an image can be
matched against is broken: the version the bundled import tool reports has been frozen at `1.1.1` across
releases since before `v2023.03.30.1`, so the field `db.json` records cannot distinguish releases, and the
documented rule "the image tag should equal the libosmscout version so tree drift is visible" does not
hold. Publishing images without first making the reported version meaningful would distribute artifacts
that cannot be told apart.

## What Changes

- The regeneration image and the web server image SHALL be published to the project's public container
  registry as part of ordinary development: a commit on the main branch that changes an input of the image
  publishes, and so does a manual run. Pull requests SHALL NOT publish.
- Every publication SHALL carry the release version the source declares, `latest`, and a date and time
  stamp identifying that build. The stamp SHALL be unique per publication, while the release version tag
  and `latest` move with the newest publication. This is a rolling scheme: a release is a milestone that
  changes the version the following builds are tagged with, not a separate kind of image.
- Published builds SHALL be pruned automatically, keeping only a bounded number of the newest ones, so
  that the number of published stamps does not grow without bound.
- An image SHALL NOT contain a hardcoded library version; the version it reports SHALL be the version of
  the source it was built from.
- Every release SHALL establish a library version, separate from the release's chronological version,
  which the artifacts of that release report - including the version recorded in generated database
  metadata. The release process SHALL refuse to release a revision that does not declare the version it is
  told to release.
- Consumers SHALL be able to obtain and run the published images without building the repository,
  including the two-container orchestration, and the documentation SHALL state the published names, the
  meaning of each tag, how to pin a concrete build, and that pinned builds are pruned eventually.
- Publication SHALL be gated on the existing build and smoke verification, so an unverified image is never
  published.
- No change to rendered output, database format, style sheets, the import pipeline, or the existing public
  C++ API.

## Capabilities

### New Capabilities

- `container-image-publishing`: how the regeneration and web server images are named, which revisions
  publish and which never do, which tags a publication carries and which of them move, the automatic
  pruning of old builds, that an image carries no version literal, and how a consumer obtains the
  published images.
- `release-library-version`: the per-release library version that build artifacts report, its relationship
  to the release's chronological version, and the compatibility rule for its major component.

### Modified Capabilities

<!-- None. `mapgen-container` keeps its requirements (image contents, entry point, mounts, least
     privilege) and `web-server` is untouched: this change adds distribution and version identity, it does
     not change what the images contain or how they run. -->

## Impact

Affected files and modules:

- `.github/workflows/mapgen_image.yml` - publishes on a main-branch commit and on a manual run, derives the
  tags, keeps the build and smoke checks as the gate publication depends on, and prunes old builds.
- `.github/workflows/release.yml` - asserts that the revision it releases declares both the release version
  and the library version, instead of editing the library version in its working tree; it no longer starts
  a publication, because a release is a version milestone and the commit that sets the version publishes
  through the ordinary path.
- `CMakeLists.txt` (`project(libosmscout VERSION ...)` and `set(OSMSCOUT_LIBRARY_VERSION ...)`) and
  `meson.build` (`libraryVersion='...'`, and the `version:` that names the release and the distribution
  archives) - the library version is set per release by a version bump commit instead of being frozen at
  `1.1.1`, and `meson.build`'s `version:` is the release version the images are tagged with.
- `scripts/mapgen/Dockerfile` - the library files are staged without a version literal, the hand-written
  symlink block disappears, OCI metadata is added, and the leftover `ldd ... | wc -l` debug output is
  removed.
- `scripts/mapgen/Dockerfile.serve` - OCI metadata.
- `scripts/mapgen/docker-compose.yml` - the image names become overridable so the orchestration can consume
  published images instead of always building locally.
- `Documentation/MapRepository.md` (section 5) - pull instructions, the tag semantics including which tag
  pins a build and how long a pinned build lasts, the one-time step that makes a package publicly
  pullable, the version bump procedure of a release, and the correction of the current claim that the image
  tag equals the libosmscout version.
- `.dockerignore` - keep the repository history out of the build context.
- `TODO.md` - record the deferred findings.
- `.github/workflows/release_latest.yml` is unchanged: it names the distribution archives of a snapshot
  release, which is independent of the images.

No database format, style sheet, import, build system, or packaging change. The only new external
dependency is the container registry the images are published to.
