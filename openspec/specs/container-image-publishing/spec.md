# container-image-publishing Specification

## Purpose
Makes the regeneration image and the repository web server image obtainable without building this
repository: the registry names that carry them, which revisions publish, which tags a publication
carries, that old builds are pruned automatically, that an image reports the version it was built from,
and how a consumer pulls and runs the published images.

## Requirements

### Requirement: Published images and their names

Both container images, the map repository regeneration image and the repository web server image, SHALL
be published to the project's public container registry on GitHub Packages. The images SHALL be published
under names derived from the repository owner and the repository name, one distinct name per image, and
SHALL be pullable without registry credentials.

#### Scenario: Both images are published under repository-derived names

- **GIVEN** a published source revision
- **WHEN** the images for that revision are published
- **THEN** the regeneration image and the web server image SHALL each be available in the project's
  container registry under a distinct name derived from the repository owner and repository name

#### Scenario: Anonymous pull of a public package

- **GIVEN** an image whose registry package is public
- **WHEN** a consumer pulls a published tag without presenting registry credentials
- **THEN** the pull SHALL succeed

### Requirement: Publication follows the revision and requires verification

Publication SHALL happen only for a revision that passed the image build and the image smoke checks. A
pull request SHALL NOT publish. A commit on the main branch that changes an input of the image SHALL
publish. A commit on the main branch that changes no input of the image SHALL NOT publish, because it
cannot change the image content. A manual run SHALL publish when it is asked to. Nothing else publishes.

#### Scenario: A pull request publishes nothing

- **GIVEN** a pull request that changes a path the image workflow reacts to
- **WHEN** the image workflow runs for that pull request
- **THEN** no tag SHALL be created or moved in the container registry

#### Scenario: A merge on the main branch publishes

- **GIVEN** a merge or direct commit on the main branch that changes an input of the image
- **WHEN** the image workflow runs for it
- **THEN** both images SHALL be published for that revision

#### Scenario: A commit without an image input publishes nothing

- **GIVEN** a commit on the main branch that changes none of the inputs of the image
- **WHEN** the image workflow is considered for it
- **THEN** no image SHALL be built and no tag SHALL be created or moved

#### Scenario: A manual run publishes on request

- **GIVEN** a manual run of the image workflow with publishing requested
- **WHEN** the build and the smoke checks pass
- **THEN** both images SHALL be published for the revision the run built

#### Scenario: A failing verification stops publication

- **GIVEN** a source revision whose image build or whose image smoke check fails
- **WHEN** the image workflow completes for that revision
- **THEN** no registry tag SHALL reference an image built from that revision

#### Scenario: A published image satisfies the smoke checks

- **GIVEN** a published tag
- **WHEN** its image is started with the documented mounts and read-only root filesystem
- **THEN** it SHALL run its process as the non-root user and SHALL complete one pass as the existing
  image smoke checks require
- **AND** when the bundled import tool is asked for its version it SHALL print the library version of
  the source revision the image was built from

### Requirement: The tags a publication carries

Every publication SHALL carry, for each image: the release version the published source declares, when it
declares one; the `latest` tag; and a date and time stamp identifying that build. All tags of one
publication SHALL resolve to the same image content. The stamp tag SHALL be unique to its publication and
SHALL NOT be moved afterwards, while the release version tag and `latest` SHALL follow the newest
publication.

#### Scenario: One publication carries the release version, latest and the build stamp

- **GIVEN** a revision whose source declares the release version X
- **WHEN** that revision publishes
- **THEN** tag X, tag `latest` and the build stamp tag SHALL each resolve to the image content built from
  that revision, for both images

#### Scenario: The stamp does not move

- **GIVEN** two publications
- **WHEN** the build stamp tag of the first is inspected after the second has been published
- **THEN** it SHALL still resolve to the first publication's image content

#### Scenario: The release version tag and latest follow the newest publication

- **GIVEN** two publications of the same release version, the second being newer
- **WHEN** both have been published
- **THEN** the release version tag and `latest` SHALL both resolve to the newer publication's image
  content

#### Scenario: A source without a declared release version publishes without that tag

- **GIVEN** a publication whose source declares no release version
- **WHEN** it publishes
- **THEN** it SHALL carry `latest` and the build stamp only
- **AND** the run SHALL report that the release version tag was omitted

### Requirement: Old builds are pruned

Publications SHALL be pruned automatically: for each image, only a bounded number of the newest builds
SHALL be kept and older ones SHALL be deleted, so that the number of published build stamps does not grow
without bound. The `latest` tag and the release version tag SHALL remain, resolving to the newest
publication after a pruning run.

#### Scenario: Older builds disappear

- **GIVEN** more publications than the configured number to keep
- **WHEN** a publication has been pruned
- **THEN** only the newest kept builds SHALL still be pullable by their build stamp
- **AND** their stamps SHALL be the newest ones published

#### Scenario: The moving tags survive pruning

- **GIVEN** a pruning run
- **WHEN** it has completed
- **THEN** `latest` and the release version tag SHALL still resolve to the newest publication's image
  content

### Requirement: An image reports the version it was built from

An image build definition SHALL NOT contain a hardcoded library version, and an image SHALL report the
library version of the source revision it was built from. A published image SHALL carry metadata naming
its source repository, the source revision and the version it was built from.

#### Scenario: No version literal in the build definition

- **GIVEN** the image build definition of either image
- **WHEN** it is inspected
- **THEN** it SHALL NOT name a specific library version as a literal

#### Scenario: The reported version is the built version

- **GIVEN** a source revision whose library version is Y
- **WHEN** an image is built from that revision and its bundled import tool reports its version
- **THEN** the reported version SHALL be Y

#### Scenario: The image names its provenance

- **GIVEN** a published image
- **WHEN** its image metadata is inspected
- **THEN** it SHALL name the source repository, the source revision and the version it was built from

### Requirement: A consumer runs the published images without building

The documented way to run the two-container orchestration SHALL support using the published images
instead of building them locally. The documentation for the container images SHALL state the published
registry names, the meaning of each published tag, which tag to use to pin a concrete build, that pinned
builds are pruned eventually, and the command that pulls them.

#### Scenario: The orchestration runs published images

- **GIVEN** a checkout that has never been built
- **WHEN** the documented pull path for the published images is followed
- **THEN** both containers SHALL run from images pulled from the registry

#### Scenario: The tag semantics are documented

- **GIVEN** the documentation for the container images
- **WHEN** a consumer reads it
- **THEN** the published registry names, the meaning of the release version tag, of `latest` and of the
  build stamp, and the pull command SHALL be stated
- **AND** it SHALL state how long a build stamp remains pullable

#### Scenario: Building locally remains possible

- **GIVEN** a checkout of the repository
- **WHEN** the orchestration is started without selecting published images
- **THEN** the images SHALL still be buildable from the local sources as before
