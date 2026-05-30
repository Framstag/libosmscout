## Context

Commit `a465f4093` added `if(MINGW)` block in `cmake/ProjectConfig.cmake` `osmscout_test_project` macro. It uses `set_tests_properties(... ENVIRONMENT "PATH=...;$ENV{PATH}")` to set DLL search PATH for MinGW tests.

On MSYS2 (windows-2025, Ninja), `$ENV{PATH}` resolves during CMake configure, not at CTest run time. By test time, the cached PATH contains only MSYS2-style project dirs + no system paths. Windows `CreateProcess` can't resolve MSYS2-style paths (e.g., `/d/a/.../`). All 44 tests hit `STATUS_DLL_NOT_FOUND`.

Only tests with subsequent `set_tests_properties(... ENVIRONMENT ...)` that fully overrides this bad PATH pass (LocationLookupTest, FileFormatVersionTest, HeaderCheckTest).

## Goals / Non-Goals

**Goals:**
- All 55 tests pass on MSYS2/MinGW CI (excluding PerformanceTest)
- No test has `STATUS_DLL_NOT_FOUND` on MinGW

**Non-Goals:**
- Fixing other Windows test failures not related to DLL resolution
- Changing test logic or expected behavior
- Adding static builds or modifying library linkage
- Fixing Meson build (CI runs CMake+Ninja on MinGW)

## Decisions

| Decision | Choice | Rationale |
|----------|--------|-----------|
| Root cause | `if(MINGW)` block's `set_tests_properties(... ENVIRONMENT "...;$ENV{PATH}")` overrides CI shell PATH with a configure-time snapshot. `$ENV{PATH}` resolves during CMake configure, before DLLs exist — contains only MSYS2-style project dirs + no system paths. | Tests with custom `set_tests_properties(... ENVIRONMENT ...)` after macro pass. All others fail with STATUS_DLL_NOT_FOUND. |
| Fix approach | **Remove** `if(MINGW)` ENVIRONMENT block. Let tests inherit CI shell PATH. | CI shell PATH (set via `PATH=$PATH:$PWD/...` in workflow) provides correct MSYS2-translated paths when no ENVIRONMENT override exists. |
| SymbolRendererSVGTest | Add `$PWD/libosmscout-map-svg` to CI workflow PATH | This test links OSMScout::MapSVG. Its DLL dir was not in CI PATH. |

## Alternatives considered

- **Static linking for tests**: Changes build config, may hide real link issues.
- **`$<TARGET_RUNTIME_DLLS:tgt>` generator expression**: Still uses `set_tests_properties(... ENVIRONMENT ...)` which captures `$ENV{PATH}` at configure time.
- **Keep ENVIRONMENT with `file(TO_NATIVE_PATH ...)`**: Fixed path format but `$ENV{PATH}` still captured at wrong time.

## Risks / Trade-offs

- [Risk] Tests rely on CI workflow PATH — if a project DLL dir is omitted, tests fail silently. **Mitigation**: CI failure visible immediately; trivial to add dir.
- [Trade-off] No CMake-level PATH protection for manual `ctest` invocation. **Mitigation**: CI always sets PATH; manual testing can use `env PATH=... ctest`.
