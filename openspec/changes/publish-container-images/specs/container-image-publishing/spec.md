# Spec Delta

## Purpose

Makes the regeneration image and the repository web server image obtainable without building this
repository: which registry and names carry them, which tags a release publishes and what each tag
means, which kinds of work never publish, that the image reports the version it was built from, and
how a consumer pulls and runs the published images.

## ADDED Requirements

### Requirement: Published images and their names

A released version of this project SHALL publish both container images, the map repository
regeneration image and the repository web server image, to the project's public container registry
on GitHub Packages. The images SHALL be published under names derived from the repository owner and
the repository name, one distinct name per image, and SHALL be pullable without registry
credentials.

#### Scenario: Both images are published under repository-derived names

- **GIVEN** a released source revision
- **WHEN** the images for that revision are published
- **THEN** the regeneration image and the web server image SHALL each be available in the project's
  container registry under a distinct name derived from the repository owner and repository name

#### Scenario: Anonymous pull of a public package

- **GIVEN** an image whose registry package is public
- **WHEN** a consumer pulls a published tag without presenting registry credentials
- **THEN** the pull SHALL succeed

#### Scenario: The two images stay distinguishable

- **GIVEN** the published registry names of both images
- **WHEN** a consumer pulls by name
- **THEN** the regeneration image SHALL NOT be reachable under the web server image's name and the
  other way round

### Requirement: Tag set published for a release

A release SHALL publish, for each image, an immutable tag identifying that release, a tag equal to
the library version that release reports, and - for the newest release only - the `latest` tag.
Tags that identify a release SHALL NOT be moved by any later publication.

#### Scenario: A release publishes all three tags

- **GIVEN** a release with chronological version X and library version Y
- **WHEN** that release publishes its images
- **THEN** tag X, tag Y and tag `latest` SHALL each resolve to the images built from the released
  source revision

#### Scenario: Tags of one release resolve to the same image content

- **GIVEN** a release with chronological version X and library version Y
- **WHEN** the images published for tags X and Y are compared
- **THEN** both tags SHALL resolve to the same image content for each of the two images

#### Scenario: A release tag is never moved

- **GIVEN** a published release with chronological version X
- **WHEN** a later release with chronological version X2 is published
- **THEN** tag X SHALL still resolve to the image content published for release X

#### Scenario: latest follows the newest release only

- **GIVEN** two releases published in order, the later one being the newest release
- **WHEN** both have been published
- **THEN** tag `latest` SHALL resolve to the image content of the later release

#### Scenario: Unreleased work never becomes latest

- **GIVEN** snapshot or development activity on the main branch after a release
- **WHEN** the image workflow has reacted to that activity
- **THEN** tag `latest` SHALL still resolve to the newest release's image content

### Requirement: Nothing is published for unverified or unreleased work

Publication SHALL NOT happen for a pull request, and SHALL NOT happen for unreleased snapshot
activity. Publication of a source revision SHALL depend on that same revision having passed the
image build and the image smoke checks.

#### Scenario: A pull request publishes nothing

- **GIVEN** a pull request that changes a path the image workflow reacts to
- **WHEN** the image workflow runs for that pull request
- **THEN** no tag SHALL be created or moved in the container registry

#### Scenario: Snapshot activity publishes nothing

- **GIVEN** the continuous snapshot release that is created for a push to the main branch
- **WHEN** the image workflow reacts to that snapshot release
- **THEN** no tag SHALL be created or moved in the container registry

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

### Requirement: An image reports the version it was built from

An image build definition SHALL NOT contain a hardcoded library version, and an image SHALL report
the library version of the source revision it was built from. A published image SHALL carry metadata
naming its source repository, the source revision and the version it was built from.

#### Scenario: No version literal in the build definition

- **GIVEN** the image build definition of either image
- **WHEN** it is inspected
- **THEN** it SHALL NOT name a specific library version as a literal

#### Scenario: The reported version is the built version

- **GIVEN** a source revision whose library version is Y
- **WHEN** an image is built from that revision and its bundled import tool reports its version
- **THEN** the reported version SHALL be Y

#### Scenario: The image can be asked which version it reports

- **GIVEN** a published image of a release whose library version is Y
- **WHEN** the bundled import tool is asked for its version
- **THEN** it SHALL print Y and exit successfully
- **AND** the library version tag published for that release SHALL be Y

#### Scenario: The image names its provenance

- **GIVEN** a published image
- **WHEN** its image metadata is inspected
- **THEN** it SHALL name the source repository, the source revision and the version it was built from

### Requirement: A consumer runs the published images without building

The documented way to run the two-container orchestration SHALL support using the published images
instead of building them locally. The documentation for the container images SHALL state the
published registry names, the meaning of each published tag, and the command that pulls them.

#### Scenario: The orchestration runs published images

- **GIVEN** a checkout that has never been built
- **WHEN** the documented pull path for the published images is followed
- **THEN** both containers SHALL run from images pulled from the registry

#### Scenario: The tag semantics are documented

- **GIVEN** the documentation for the container images
- **WHEN** a consumer reads it
- **THEN** the published registry names, the meaning of the release tag, of the library version tag
  and of `latest`, and the pull command SHALL be stated

#### Scenario: Building locally remains possible

- **GIVEN** a checkout of the repository
- **WHEN** the orchestration is started without selecting published images
- **THEN** the images SHALL still be buildable from the local sources as before
