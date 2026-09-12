## Context

See proposal.md - Why. The VS 2025 CI workflow (`.github/workflows/build_and test_on_vs2025.yml`, `cmake` job) currently caches vcpkg binary archives via `actions/cache` keyed on the manifest hash, with `VCPKG_BINARY_SOURCES` pointed at a local files directory. Empirical evidence from run logs:

- Runner image updates (~weekly, e.g. `20260810.198.2` -> `20260818.207`) change the vcpkg ABI hash even when MSVC/SDK version strings are identical (zlib ABI `a0e4b087...` -> `f16e1392...`).
- `actions/cache` restores the stale archives ("Cache hit", "Cache restored successfully") but vcpkg rejects all of them ("Restored 0 package(s)") and rebuilds all 39 ports (~60 min).
- The post-step never saves on a hit ("Cache hit occurred on the primary key ..., not saving cache"), so the stale cache persists and every subsequent run rebuilds.

## Goals / Non-Goals

**Goals:**
- Warm-cache runs restore vcpkg dependencies without rebuilding (~60 min saved).
- The cache self-heals after toolchain/runner-image changes: one rebuild, then cached again.
- Manifest changes rebuild only affected packages, not the full set.

**Non-Goals:**
- No change to the vcpkg manifest (`vcpkg_medium.json`) or the meson job (uses subprojects, not vcpkg).
- No change to the 10GB repository cache budget or other workflows' caches (flagged as risk only).
- No offline/self-hosted runner support.

## Decisions

### Decision 1: Use a NuGet binary cache hosted on GitHub Packages

vcpkg's `x-gha` binary cache provider was **removed from vcpkg-tool** (PR microsoft/vcpkg-tool#1662, merged 2025-04-29) after GitHub Actions Cache API changes; the vcpkg team recommends NuGet-based binary caching instead. The workflow therefore configures `VCPKG_BINARY_SOURCES=clear;nuget,https://nuget.pkg.github.com/Framstag/index.json,readwrite` and registers the feed with the run's `GITHUB_TOKEN` (`permissions: packages: write`). vcpkg stores each built port as a NuGet package versioned by its ABI hash and restores matching packages before building.

Self-healing flow after a runner image update:

```
+----------------+     +---------------------+     +--------------------------+
| runner image   |     | vcpkg install       |     | GitHub Packages NuGet    |
| update changes | --> | computes new ABI    | --> | feed (per-port packages, |
| toolchain ABI  |     | hashes, no match    |     | version = ABI hash)      |
+----------------+     | -> rebuilds all 39  |     +--------------------------+
                       | -> pushes each as   |              |
                       |    new version      |<-------------+
                       +---------------------+
                              |
                              v
                    next run: all packages
                    restored from feed (fast)
```

**Alternatives considered:**

1. **`x-gha` binary cache provider (original design)**. Rejected: removed from vcpkg-tool 2025-12-16 (the pinned version); the run log shows "The 'x-gha' binary caching backend has been removed".
2. **Files-based archives + `actions/cache` keyed on toolchain fingerprint** (e.g. SHA-256 of `cl.exe`) with `restore-keys` fallback. Keeps the single-entry design, no external feed or tokens. Rejected as primary: the fingerprint is a heuristic — we proved version strings are insufficient (VS 18.8 -> 18.9 image update flipped the ABI while the toolset version string stayed 14.51.36231), and a binary hash may still miss ABI-relevant components. If the fingerprint misses a component, the stale-cache-forever bug returns. The NuGet provider computes the ABI hash itself, so it cannot drift from what vcpkg actually uses.
3. **Files-based archives + `actions/cache` (original workflow design)**. Rejected: `actions/cache` cannot overwrite an existing key, so a cache that is restored but rejected is never updated — permanent rebuild state after every image update.
4. **Cache the installed tree (`build/vcpkg_installed`) instead of archives**. Rejected: vcpkg re-verifies ABI on every install, so the same invalidation applies, and whole-tree granularity means any manifest change invalidates all packages (fails the spec requirement).

### Decision 2: Enable run-vcpkg's tool checkout cache

Set `doNotCache: false` on the `lukka/run-vcpkg` step so the vcpkg tool clone (~18s) and bootstrap (~4s) are cached between runs. Alternative: leave `doNotCache: true` (fresh clone each run) — deterministic but wastes ~25s/run. Chosen: cache it; the tool is pinned to a fixed commit, so caching is safe.

## Risks / Trade-offs

- **Cache entry accumulation** — each image update adds ~250MB of per-package entries; the repo is already near the 10GB limit (msys2 workflow alone holds ~6.3GB across 14 entries). -> Mitigation: GitHub evicts least-recently-used entries first; old ABI generations are exactly what should be evicted. Flag the msys2 cache bloat as a separate cleanup.
- **`GITHUB_TOKEN` push rights on the NuGet feed** — vcpkg docs note that `GITHUB_TOKEN` may not have upload/download rights in some setups. -> Mitigation: first run verifies push; if it fails, switch to a PAT secret (`VCPKG_PAT_TOKEN`, scopes `packages:read`/`packages:write`) — one-time setup by the repo owner.
- **Token stored in runner-local NuGet.config** — `-StorePasswordInClearText` writes the token to `%APPDATA%\NuGet\NuGet.Config` on the ephemeral runner, not the repo checkout. -> Mitigation: explicit `-ConfigFile` to the user-level path; `NuGet.config` added to `.gitignore`; the token is short-lived and already exposed to all steps via the environment.
- **GitHub Packages storage** — packages accumulate per ABI generation (~250MB per image update). The repo is public, so packages are public and storage is free. -> Mitigation: none needed; old versions can be pruned manually if ever required.
- **PR runs miss the master feed state** — fork PRs get a read-only `GITHUB_TOKEN`, so they can restore but not push. -> Mitigation: acceptable; master runs maintain the feed, PRs still restore from it.
- **One full rebuild per image update** — unavoidable and correct: binaries built against an old toolchain must not be reused. -> Mitigation: this is the designed recovery cost; the point is it happens once, not every run.

## Migration Plan

1. Edit `.github/workflows/build_and test_on_vs2025.yml`: add `permissions: packages: write`; remove the `Cache vcpkg binary archives` step and the old `Configure vcpkg to use binary cache` override; add the NuGet source registration step and the NuGet `VCPKG_BINARY_SOURCES` override; add `doNotCache: false` to the run-vcpkg step. Add `NuGet.config` to `.gitignore`.
2. Push to a branch, run the workflow: first run rebuilds all packages and pushes them to the GitHub Packages feed.
3. Re-run: verify "Restored N package(s)" with N > 0 and the configure step drops from ~60 min to ~2 min.
4. Rollback: revert the workflow edit; the old `actions/cache` entry remains until evicted, and feed packages can be deleted from GitHub Packages.

## Open Questions

None — the approach is fully determined by the evidence above.
