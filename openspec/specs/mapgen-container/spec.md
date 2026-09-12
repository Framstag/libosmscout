# Mapgen Container Specification

## Purpose

Container image that bundles the regeneration process with a pinned version of the import tool, can mount the database repository to update, and runs one complete pass per invocation under an external scheduler.

## Requirements

### Requirement: Image contents
The container image SHALL contain the regeneration process, the import tool with its runtime dependencies, and the utilities needed to download and verify source data. The image SHALL identify the exact import tool version it bundles.

#### Scenario: Self-contained image
- **WHEN** the image is started with the required mounts
- **THEN** the full regeneration process including import runs inside the container without host-provided tooling

#### Scenario: Tool version identifiable
- **WHEN** an operator inspects the image
- **THEN** the bundled import tool version is determinable and matches the version recorded in generated database metadata

### Requirement: Entry point semantics
The image entry point SHALL execute one complete pass over the imports manifest, honoring the refresh frequency gates, and SHALL exit when the pass is done. The image SHALL NOT run an internal scheduler or long-running daemon.

#### Scenario: One pass per start
- **WHEN** the container is started
- **THEN** it checks due imports, performs applicable downloads and imports, and exits with a status reflecting the outcome

#### Scenario: No scheduler inside
- **WHEN** the container runs
- **THEN** it terminates after the pass and does not schedule further work itself

### Requirement: Volume mounts
The image SHALL mount three distinct areas: a transient work area for downloads and intermediate import data, the database repository to update, and the configuration holding the imports manifest read-only. The region index used for layout is not part of the configuration; the script reads it from the served root of the repository (`public/names.json`).

#### Scenario: Work area isolated
- **WHEN** an import runs in the container
- **THEN** downloads and intermediate data are confined to the transient work area

#### Scenario: Configuration read-only
- **WHEN** the regeneration process reads configuration
- **THEN** the configuration area is not writable by the process

#### Scenario: Repository updated in place
- **WHEN** a database is placed or pruned
- **THEN** this happens in the mounted database repository

### Requirement: Least privilege
The container SHALL run the process as a non-root user and SHALL use a read-only root filesystem, with writability limited to the mounted areas.

#### Scenario: Non-root execution
- **WHEN** the container starts
- **THEN** the process runs without root privileges

#### Scenario: Read-only root filesystem
- **WHEN** the container starts
- **THEN** the image's own filesystem is not writable by the process
