# Proposal

## Why

The regeneration container and the repository web server container are built and smoke-tested in CI,
but they are only obtainable by building this repository locally: nothing publishes them, so an
operator who wants to run the map repository pipeline has no artifact to pull and no release-stable
identity to pin. At the same time the identity an image can be matched against is broken: the version
the bundled import tool reports has been frozen at `1.1.1` across releases since before
`v2023.03.30.1`, so the field `db.json` records cannot distinguish releases, and the documented rule
"the image tag should equal the libosmscout version so tree drift is visible" does not hold. Publishing
images without first making the reported version meaningful would distribute artifacts that cannot be
told apart.

## What Changes

- The regeneration image and the web server image SHALL be published to the project's public container
  registry as part of a release, under names derived from this repository.
- A published release image SHALL carry an immutable tag identifying the release, an additional tag
  matching the library version that release reports, and the newest release SHALL also be reachable
  through a `latest` tag.
- Pull requests and unreleased snapshot activity SHALL NOT publish anything.
- A published image SHALL NOT contain a hardcoded library version literal; the version an image
  reports SHALL be the version it was built from, and the bundled import tool SHALL be able to report
  that version on request.
- Every release SHALL establish a library version, distinct from the release's chronological version,
  which the build artifacts of that release report; the library version's major component SHALL only
  change for a deliberate compatibility break.
- Consumers SHALL be able to obtain and run the published images without building the repository,
  including the two-container orchestration, and the documentation SHALL state the published names,
  the tag semantics, and how the images are pulled.
- Publishing SHALL be gated on the existing build and smoke verification, so an unverified image is
  never published.
- No change to rendered output, database format, style sheets, the import pipeline, or the existing
  public C++ API.

## Capabilities

### New Capabilities

- `container-image-publishing`: how the regeneration and web server images are named, which tags a
  release publishes, what is never published, that an image carries no version literal, and how a
  consumer obtains the published images.
- `release-library-version`: the per-release library version that build artifacts report, its
  relationship to the release's chronological version, and the compatibility rule for its major
  component.

### Modified Capabilities

<!-- None. `mapgen-container` keeps its requirements (image contents, entry point, mounts, least
     privilege) and `web-server` is untouched: this change adds distribution and version identity,
     it does not change what the images contain or how they run. -->

## Impact

Affected files and modules:

- `.github/workflows/mapgen_image.yml` - gains the publish step, its registry credentials and
  permissions, the release trigger, and the derivation of the release tags; the existing build and
  smoke checks stay and become the gate that publication depends on.
- `.github/workflows/release.yml` - gains the release-time input for the library version and sets it
  in the source the release is built from, so the released artifacts report it.
- `CMakeLists.txt` (`project(libosmscout VERSION ...)` and `set(OSMSCOUT_LIBRARY_VERSION ...)`) and
  `meson.build` (`libraryVersion='...'`, beside the `version:` that names the distribution archives)
  - the library version is set per release instead of being frozen at `1.1.1`.
- `scripts/mapgen/Dockerfile` - the library files are staged without a version literal, the
  hand-written symlink block disappears, OCI metadata is added, and the leftover `ldd ... | wc -l`
  debug output is removed.
- `scripts/mapgen/Dockerfile.serve` - OCI metadata.
- `scripts/mapgen/docker-compose.yml` - the image names become overridable so the orchestration can
  consume published images instead of always building locally.
- `Documentation/MapRepository.md` (section 5, "The container image") - pull instructions, the tag
  semantics table, the one-time step that makes a package publicly pullable, and the correction of the
  current claim that the image tag equals the libosmscout version.
- `.dockerignore` - keep the repository history out of the build context.
- `TODO.md` - record the deferred findings: unreleased builds report a library version with no commit
  identity, and only one architecture is published.
- `Import/src/Import.cpp` - the import tool gains a query that prints the library version it was
  built from, so a published image can be asked which version it reports.
- `Tests` - a unit test for the new version query. The workflow and registry behavior itself is
  verified by running the workflow and inspecting the published tag list (see design), not by a unit
  test.

No database format, style sheet, import, build system, or packaging change. The only new external
dependency is the container registry the images are published to.
