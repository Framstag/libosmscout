# vs-vcpkg-cache Specification

## Purpose

Defines how vcpkg dependencies are cached in the Visual Studio 2025 CI workflow so that unchanged dependencies are restored between builds, the cache recovers after toolchain changes, and manifest changes do not trigger full rebuilds.

## Requirements

### Requirement: vcpkg dependencies are restored from cache between builds

The Visual Studio 2025 CI workflow SHALL restore previously built vcpkg dependencies from a cache before building, so that dependencies unchanged since the last run are not rebuilt from source.

#### Scenario: Consecutive runs with unchanged manifest and toolchain restore dependencies
- **WHEN** the VS 2025 CI workflow runs a second time with the same dependency manifest and the same toolchain as the previous run
- **THEN** the vcpkg install step SHALL restore the previously built packages from the cache
- **AND** the configure step SHALL complete without rebuilding the dependencies from source

#### Scenario: First run with empty cache builds and saves dependencies
- **WHEN** the VS 2025 CI workflow runs with no matching cache entries
- **THEN** all required vcpkg dependencies SHALL be built from source
- **AND** the built packages SHALL be stored in the cache for subsequent runs

### Requirement: Cache recovers after toolchain changes

When a toolchain or runner-image change invalidates the cached dependencies, the VS 2025 CI workflow SHALL recover after exactly one rebuild: the rebuilt packages SHALL replace the invalidated cache entries so subsequent runs restore them.

#### Scenario: Runner image update invalidates cache, then cache recovers
- **WHEN** the runner image changes the toolchain such that the cached dependency ABI no longer matches
- **THEN** the first run after the change SHALL rebuild the affected dependencies from source
- **AND** the rebuilt packages SHALL be stored in the cache
- **AND** the run after that SHALL restore the dependencies from the cache without rebuilding

### Requirement: Manifest changes do not trigger full rebuilds

A change to the vcpkg dependency manifest SHALL cause only the affected packages to be rebuilt; packages unaffected by the change SHALL be restored from the cache.

#### Scenario: Adding a dependency rebuilds only the new package
- **WHEN** a new dependency is added to the vcpkg manifest
- **THEN** the new dependency SHALL be built from source
- **AND** all previously cached dependencies SHALL be restored from the cache without rebuilding

### Requirement: Cache is updated after a rebuild

The VS 2025 CI workflow SHALL update the cache whenever dependencies are rebuilt, so that a run that rebuilt dependencies leaves a cache that later runs can restore.

#### Scenario: Rebuild after cache invalidation updates the cache
- **WHEN** a run rebuilds dependencies because the cached packages were invalidated
- **THEN** the rebuilt packages SHALL be written to the cache during that same run
- **AND** a subsequent run with the same manifest and toolchain SHALL restore them without rebuilding
