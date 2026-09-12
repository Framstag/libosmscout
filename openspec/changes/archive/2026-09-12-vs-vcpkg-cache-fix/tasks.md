## 1. Workflow changes (spec: vs-vcpkg-cache)

- [x] 1.1 Remove the `Cache vcpkg binary archives` step (`actions/cache`) from the `cmake` job in `.github/workflows/build_and test_on_vs2025.yml` and verify the step is no longer present in the file
- [x] 1.2 Remove the `Configure vcpkg to use binary cache` step (the `VCPKG_BINARY_SOURCES=clear;files,...` override) and verify the step is no longer present in the file
- [x] 1.3 Add `doNotCache: false` to the `lukka/run-vcpkg` step inputs and verify the input is present in the file
- [x] 1.4 Verify the workflow YAML parses without errors (e.g. `actionlint` or a YAML parser) and the `cmake` job step order is: checkout, copy vcpkg.json, msbuild, MSVC env, run-vcpkg, configure, build, collect, upload
- [x] 1.5 Add `permissions: packages: write` (with `contents: read` and `actions: write`), the `Configure NuGet source for vcpkg binary cache` step (feed registration with `GITHUB_TOKEN`, explicit `-ConfigFile` to the user-level NuGet.config), and the `Configure vcpkg binary cache` step (`VCPKG_BINARY_SOURCES=clear;nuget,https://nuget.pkg.github.com/Framstag/index.json,readwrite` + `VCPKG_NUGET_REPOSITORY`) and verify all are present in the file
- [x] 1.6 Add `NuGet.config`/`nuget.config` to `.gitignore` and verify the entries are present

## 2. CI verification (spec: vs-vcpkg-cache)

- [x] 2.1 Push the workflow change to a branch and run the VS 2025 workflow; verify the first run builds all vcpkg dependencies and pushes them to the GitHub Packages feed (vcpkg install log shows packages built and pushed, no NuGet push errors)
- [x] 2.2 Re-run the workflow without changes; verify the vcpkg install log shows "Restored N package(s)" with N > 0 and the configure step completes in minutes, not ~60 minutes
- [x] 2.3 Verify the `meson` job in the same workflow still passes unchanged (it does not use vcpkg)
- [x] 2.4 Verify the workflow's build and test steps still pass on the cached run (build compiles without errors, existing tests pass)

## 3. Documentation

- [x] 3.1 Update AGENTS.md CI/CD workflow table or notes if the workflow's caching behavior description changes, and verify the change is documented
