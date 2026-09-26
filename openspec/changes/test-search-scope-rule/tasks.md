# Tasks

Spec references: none — this change declares no capability and sets `skip_specs: true` in
`.openspec.yaml` (it adds tests for behavior that already exists, so no spec-level behavior changes).
Each task therefore names the proposal change it serves and how its completion is verified.

## 1. Pin the depth mapping

- [x] 1.1 Add `Tests/src/SearchScopeTest.cpp` with a case over `NormalizeDepthToAdminLevel()` for the
  root, the country, the state, the county, the city and the suburb, plus depth 0
  (change proposal — What Changes; change design — D1; verify: the expectations cover every documented
  scale step and the test binary passes).

- [x] 1.2 Verify the case has teeth: change the mapping in the helper and confirm the test fails, then
  restore and confirm it passes again
  (change design — D1; verify: `depth * 2` instead of `(depth - 1) * 2` makes the test fail).

## 2. Pin the scope expansion boundary

- [x] 2.1 Add a case over `ShouldExpandScope()` for the cap itself, a level finer than the cap, a level
  coarser than the cap, and an unknown parent level against a zero cap
  (change proposal — What Changes; change design — D1; verify: each boundary the header documents has
  an expectation and the test binary passes).

- [x] 2.2 Verify the case has teeth: turn the cap comparison from inclusive into exclusive and confirm
  the test fails, then restore and confirm it passes again
  (change design — D1; verify: `parentLevel > maxLevel` makes the test fail).

## 3. Pin the composed rule and the cap

- [x] 3.1 Add a case that reads the cap against a real chain — the state too coarse to expand into, the
  district and the city fine enough — by feeding the normalized depths into the cap comparison
  (change proposal — What Changes; verify: the three composed expectations hold and the test binary
  passes).

- [x] 3.2 Pin the cap value itself, with a comment naming it a product decision about result volume
  (change design — D3; verify: the assertion states the documented value and its comment explains why a
  failure is a decision and not a bug).

## 4. Register, build and run in both build systems

- [x] 4.1 Register the new target in `Tests/CMakeLists.txt` with the include path of
  `libosmscout-client-java/src`, configure and build with CMake
  (change design — D2; verify: the target builds and `ctest -R SearchScopeTest` passes).

- [x] 4.2 Register the same target in `Tests/meson.build` with the same include path and build with
  Meson
  (change design — D2; verify: the executable is produced and `meson test -C build-meson` runs it).

- [x] 4.3 Run the complete host test suite and verify no test regresses and the suite grows by exactly
  this one test
  (change proposal — What Changes; verify: every configured test passes, one more than before).

## 5. Conventions and documentation

- [x] 5.1 Check the new file against the documents in `guidelines/`, against `.clang-tidy` and against
  `.uncrustify`
  (change proposal — Impact; verify: `clang-tidy` reports no finding for the new file and `uncrustify`
  proposes no change to it).

- [x] 5.2 Record the pre-existing issues this change surfaced in `TODO.md`: the helper's location
  outside the include path of any test, and that nothing checks that both build systems register the
  same tests
  (change proposal — Impact; verify: `TODO.md` has a section for this change with both entries).

- [x] 5.3 Confirm that no document describes the scope rule as untested or documents its test, so
  nothing else needs updating
  (change proposal — Impact; verify: `Documentation/`, `guidelines/` and `AGENTS.md` describe the
  search, not its test coverage).

## Verification Notes

- Host verification: the CMake Release build and the Meson build each produce and run the new target;
  `ctest -j 4` reports one more passing test than before this change and no failure.
- Mutation checks: changing the depth mapping to `depth * 2`, and turning the cap comparison into an
  exclusive one, each make the test fail; both were reverted and the test passes again. Both helpers
  are therefore pinned, not merely called.
- Coverage limitation: the scope decision in the search (`DoSearchLocations`) is not covered — it needs
  the JNI entry point, an open database and an admin-region handle. This change pins the rule that
  decision applies, not the call site. The call site is untested before and after.
- `clang-tidy` reports no finding for the new file, and `.uncrustify` proposes no change to it.
- This change is not an extraction from the `naviveylin-local` branch (PR #1773): it answers what that
  branch's third remaining piece turned out to be worth. That piece — a behaviour-identical reordering
  of the scope branch chain plus two cosmetic comments — was measured against `.clang-tidy` and
  dropped: it adds a `bugprone-branch-clone` finding because the extra branch repeats
  `scope.push_back(nullptr)`, and the reworded comment it carries in `getAdminRegionScopeName()` says
  "the parent" where `ResolveSearchScope()` can climb several levels, so it is less accurate than the
  text already in the tree. After the compound-name matching feature (PR #1849) and the matcher test
  refactor (PR #1850), #1773 therefore has nothing left worth extracting.
