# Proposal

## Why

When a library gains a required build dependency, every entry point that builds that library has to
provide it. The basemap work made the import library require a JSON implementation; the propagation
reached the build and test workflows but missed the two release workflows. `Release Latest` then
failed at configure time on every master push from 2026-09-20 (eight consecutive failed runs, last
successful publish 2026-09-20 07:15 UTC), so no "latest" release was published for the next day and
a half. Nothing in the pull-request checks caught it, because the release workflows only run after a
merge.

## What Changes

- Both release workflows provide the JSON implementation the import library requires, so a release
  can configure the project again.
- A run that cannot satisfy a required build dependency stops before any release artifact is
  published, and reports which dependency is missing and where it was looked for.
- The rule becomes explicit rather than incidental: a required build dependency is provided by every
  entry point that builds the affected component, including configurations that deliberately do not
  build it being exempt rather than forgotten.
- The dependency a build entry point provides is recorded where the dependency is declared, so the
  next required dependency has a checklist to follow instead of relying on the author remembering
  every workflow.

## Capabilities

### New Capabilities

None. The affected behavior is dependency provisioning for CI build entry points, which an existing
capability already covers.

### Modified Capabilities

- `ci-build-fixes`: dependency provisioning requirements currently cover the demo build (libpng
  across every demo-building workflow). They extend to the import library's required JSON
  implementation, and to the release entry points that were outside their scope, with the
  configurations that intentionally skip the import library stated as exempt.

## Impact

- `.github/workflows/release.yml`, `.github/workflows/release_latest.yml` — provide the JSON
  implementation in the dependency installation step (fix applied with this change).
- `.github/workflows/build_and test_on_ubuntu_24_04.yml`,
  `.github/workflows/build_and test_on_msys.yml`, `.github/workflows/build_and test_on_osx.yml`,
  `.github/workflows/build_and test_on_vs2025.yml`,
  `.github/workflows/sanitize_on_ubuntu_24_04.yml`, `.github/workflows/sonar.yml` — already provide
  it; unchanged here, but they are the reference set the rule is written against.
- `.github/workflows/build_javascout.yml` — does not build the import library and is exempt.
- `libosmscout-import/meson.build`, `libosmscout-import/CMakeLists.txt` — declare the requirement,
  and are where the list of providing entry points is recorded.
- `subprojects/nlohmann_json.wrap` — the fallback for platforms without a system package.
- `AGENTS.md` (CI/CD section) — states the provisioning rule for a newly required build dependency.
