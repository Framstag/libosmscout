## Why

The Visual Studio 2025 CI workflow rebuilds all vcpkg dependencies from source on nearly every run (~60 minutes), because the dependency cache is invalidated by runner image updates and never recovers. Every push and pull request pays this cost, delaying feedback on Windows builds.

## What Changes

- vcpkg dependencies in the VS CI workflow SHALL be restored from a cache between builds, so unchanged dependencies are not rebuilt.
- The dependency cache SHALL recover automatically after a toolchain or runner-image change: exactly one rebuild, after which subsequent runs use the cache again.
- A change to the dependency manifest SHALL only cause the affected packages to be rebuilt, not the entire dependency set.
- The current cache mechanism, which restores stale content and never updates after a cache hit, SHALL be replaced.

## Capabilities

### New Capabilities

- `vs-vcpkg-cache`: Caching behavior of vcpkg dependencies in the Visual Studio 2025 CI workflow — restore between builds, self-heal after toolchain changes, and survive manifest changes without full rebuilds.

### Modified Capabilities

<!-- None. No existing spec-level behavior changes. -->

## Impact

- `.github/workflows/build_and test_on_vs2025.yml` — the `cmake` job's vcpkg caching steps are replaced.
- No source code, API, or dependency changes; vcpkg manifest (`vcpkg_medium.json`) unchanged.
- CI runtime: ~60 minutes saved per run when the cache is warm; one full rebuild after each runner image update that changes the toolchain.
- GitHub Actions cache storage: cache entries per dependency generation; total footprint bounded by the repository cache limit.
