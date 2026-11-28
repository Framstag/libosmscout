# Proposal

## Why

Both jobs of `.github/workflows/build_and test_on_msys.yml` build in the MSYS2 MINGW64 environment,
the `msvcrt` runtime environment that `msys2/setup-msys2@v2` selects by default. MSYS2 marks MINGW64
deprecated, and since 2026-11-18 it also removes leaf packages from that environment's repository:
`MINGW-packages#33705` dropped `glfw`, `MINGW-packages#33714` dropped `skia`. Neither package exists
for MINGW64 any more, and the repository builds the OpenGL and Skia backends with them.

The consequence is that the setup step of both jobs now aborts before anything is configured:

```
[command]C:\Windows\system32\cmd.exe /D /S /C D:\a\_temp\setup-msys2\msys2.cmd -c "'pacman' '--noconfirm' '-S' '--needed' '--overwrite' '*' 'mingw-w64-x86_64-toolchain' ... 'mingw-w64-x86_64-glfw' ... 'mingw-w64-x86_64-skia' ..."
Enter a selection (default=all): error: target not found: mingw-w64-x86_64-glfw
error: target not found: mingw-w64-x86_64-skia
##[error]Error: The process 'C:\Windows\system32\cmd.exe' failed with exit code 1
```

This affected every pull request of 2026-11-21 - run `44886294206` (PR #1934) fails both jobs at
`Setup MSYS/MINGW64`, with every later step of the jobs skipped. The last green MSYS run started
2026-11-19 17:54 UTC (run `44582212555`), before MSYS2's package removals reached the mirrors.

Issue #1927 already scoped the move off MINGW64, and #1936 acknowledged the deprecation with
`suppress-deprecation-warnings` to keep the jobs running. Acknowledging the deprecation no longer
helps: the packages the jobs build with are simply not in that environment any more, so the
acknowledgement has to give way to the migration.

## What Changes

- Both jobs of `.github/workflows/build_and test_on_msys.yml` select the MSYS2 UCRT64 environment
  (`msystem: UCRT64`) - the environment MSYS2 keeps all the packages the jobs install in - instead of
  the deprecated MINGW64 default.
- Every package entry of both install lists carries the `mingw-w64-ucrt-x86_64-` prefix of that
  environment, so the install list resolves again and no entry is left pointing at an environment the
  job does not run in.
- The `suppress-deprecation-warnings: msystem-mingw64` input goes away from both jobs: the
  deprecation is left behind rather than acknowledged.
- The setup steps state the environment and the reason where the environment is selected; the steps
  that read `MINGW_PREFIX`, `MINGW_CHOST` and the host are unchanged, because the environment prefix
  is what they follow.
- The build-status table in `README.md` and the CI/CD notes in `AGENTS.md` name the environment the
  MSYS jobs build in.
- The `ci-build-fixes` requirement that names the libpng package states it as the libpng package of
  the environment a job selects, instead of naming the package of the environment the MSYS jobs leave.

## Capabilities

### New Capabilities

None. The MSYS2 environment a job selects is CI behavior the existing `ci-build-fixes` capability
owns.

### Modified Capabilities

- `ci-build-fixes`: the requirements that cover workflow provisioning gain the MSYS2 environment a
  job builds in: every installed package is available in the environment the job selects, and a
  deprecation is resolved by selecting a different environment rather than by suppressing the
  warning about the deprecated one.

## Impact

- `.github/workflows/build_and test_on_msys.yml` - both jobs select UCRT64, both install lists move to
  the `mingw-w64-ucrt-x86_64-` prefix, `suppress-deprecation-warnings` is removed, and the two setup
  steps are renamed.
- `README.md` - the build-status row for the MSYS workflow names UCRT64.
- `AGENTS.md` (CI/CD section) - the workflow table and a new note record the environment, the reason
  and the package prefix.
- `openspec/specs/ci-build-fixes/spec.md` - the requirements this change adds, and the libpng
  requirement it restates, all via this directory's spec delta.
- No `CMakeLists.txt`, `meson.build`, source or test file is part of the change: the environment
  prefix, not the project, is what moved.