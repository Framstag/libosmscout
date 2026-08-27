## Context

See proposal.md — Why for motivation. Current state:

- `PerformanceTestBackendOGL` (Tests/src/PerformanceTest.cpp) passes the compiled-in `SHADER_INSTALL_DIR` to `MapPainterOpenGL`; shaders only exist after `cmake --install`, so dev builds fail with `Shader file ... doesn't exists` → painter init fails → test exits 1.
- Meson builds PerformanceTest (Tests/meson.build, inside `buildDemos` gate) but never registers it via `test()`; its `SHADER_INSTALL_DIR` is the relative path `"shaders"`, which does not resolve from the meson test working directory.
- The demos (`Demos/src/DrawMapOpenGL.cpp`, `OSMScoutOpenGL`) already solve the same problem with a `--shaders` CLI override; PerformanceTest lacks it. The font is already passed explicitly via `--font` pointing into the source tree.
- CMake registers `PerformanceTest-<driver>-<stylesheet>` tests (Tests/CMakeLists.txt) for drivers `noop none cairo Qt agg opengl gdi`, gated per-driver on backend availability (`TARGET glfw` for OpenGL).

## Goals / Non-Goals

**Goals:**
- PerformanceTest OpenGL driver resolves shaders from an explicit directory, defaulting to the compiled-in install dir.
- CMake `PerformanceTest-opengl-*` tests pass without a prior install step.
- Meson registers PerformanceTest tests (including the OpenGL driver) with the same runtime resources as CMake.
- Backend init failures keep producing clear, resource-identifying errors and non-zero exit.

**Non-Goals:**
- Making OpenGL rendering work in environments with no GL context at all (headless without X/EGL) — those environments get a clear failure, not a working renderer.
- Changing `MapPainterOpenGL` shader loading itself.
- Changing CI workflow exclusions (e.g. macOS `PerformanceTest-opengl` exclusion) — out of scope unless tests prove they now pass there.

## Decisions

### D1: Add `--shaders` CLI option to PerformanceTest, defaulting to `SHADER_INSTALL_DIR`

The OpenGL driver reads the shader directory from a new `--shaders` argument; when absent, it uses the compiled-in `SHADER_INSTALL_DIR` (unchanged behavior for installed usage). Both build systems pass the source-tree shader dir (`libosmscout-map-opengl/data/shaders`) in their test registrations, mirroring how `--font` and `--icons` already point into the source tree.

- **Alternative A (chosen)**: CLI override. Consistent with `DrawMapOpenGL`/`OSMScoutOpenGL` (`--shaders`), zero build-system copy steps, works identically for CMake and meson, keeps installed behavior intact.
- **Alternative B**: Build systems copy shaders into the build dir and point `SHADER_INSTALL_DIR` there. Requires new copy steps in both CMake and meson, diverges from the demo pattern, and meson would need a `custom_target`/`configure_file` per shader.
- **Alternative C**: Compile a source-relative fallback path into PerformanceTest. Bakes a source-tree path into the binary, breaks when the source tree moves, and cannot be overridden by users.

Risk assessment: A is lowest risk — additive CLI arg, no behavior change when the flag is absent, reuses a proven pattern. B risks build-system drift between CMake/meson. C risks stale paths and is least flexible.

### D2: Register meson tests mirroring the CMake driver × stylesheet matrix

In Tests/meson.build, add `test()` entries for PerformanceTest for each driver × stylesheet combination, gated per-driver on backend availability. OpenGL driver registered only when `buildMapOpenGL` and `glfwDep.found()`. All runtime args (database, stylesheet, icons, font, shaders, bbox) use `meson.current_source_dir()`-based absolute paths, since meson tests run with the build dir as working directory.

- **Alternative A (chosen)**: Explicit `test()` calls in Tests/meson.build, same matrix as CMake. Direct, readable, per-driver gating matches CMake's `TARGET glfw` check.
- **Alternative B**: A wrapper script invoked by meson that dispatches drivers. Extra indirection, harder to debug, no benefit.
- **Alternative C**: Keep meson unregistered (status quo). Rejected — user requirement is meson activation.

Risk assessment: A is low risk — meson `test()` is standard; the main risk is arg drift between the two build systems, mitigated by keeping the driver/stylesheets lists and arg construction structurally identical to CMake's.

### D3: Keep the existing `buildDemos` gate for meson PerformanceTest

Meson builds PerformanceTest only when `buildDemos` is enabled; test registration lives in the same block so tests exist exactly where the binary exists.

- **Alternative A (chosen)**: Keep the gate, register tests inside it. Minimal change, no footprint impact.
- **Alternative B**: Build PerformanceTest unconditionally in meson. Larger change, changes install footprint, not required for the fix.

Risk assessment: A is low risk; a meson build with `buildDemos=false` simply has no PerformanceTest tests, same as today's CMake behavior when `OSMSCOUT_BUILD_MAP` is off.

## Risks / Trade-offs

- [Shader dir arg drift between CMake and meson registrations] → Keep both registrations structurally identical; both reference the same `libosmscout-map-opengl/data/shaders` source dir.
- [Headless environments still fail OpenGL tests (no GL context)] → Spec requires a clear, resource-identifying error and non-zero exit; CI already runs these under `xvfb-run`. Making rendering work headless is a non-goal.
- [macOS CI excludes `PerformanceTest-opengl`] → Exclusion stays until tests are proven to pass there; not part of this change.
- [Meson `glfwDep` optional (`required: false`)] → OpenGL driver tests simply not registered when GLFW is absent; spec covers this.
- [Windows GLFW via vcpkg] → CMake `TARGET glfw` gate already handles availability; no new dependency introduced.

## Migration Plan

No data migration. Deployment is a build-config + CLI change: rebuild and re-run `ctest` / `meson test`. Rollback: revert the CLI option and the two build-file registrations; installed-usage behavior (default `SHADER_INSTALL_DIR`) is unchanged either way.

## Open Questions

- Should the macOS CI exclusion for `PerformanceTest-opengl` be lifted once these tests pass there? Deferrable — CI policy, not spec behavior; revisit after the fix lands.
