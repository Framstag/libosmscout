# Tasks

## 1. Red case and root cause

- [x] 1.1 Re-confirmed:  `TypeSetBytes` (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:527`-`:531`) still
      computes `typeSets.size()*(sizeof(TypeInfoSet)+typeConfig.GetTypeCount()*sizeof(TypeInfoRef))`, and
      `TypeInfoSet::Size()` (`libosmscout/include/osmscout/TypeInfoSet.h`) still returns the number of
      contained types. The set holds `std::vector<uint64_t> words` (one bit per defined type) plus
      `std::vector<TypeInfoRef> contained` (one slot per contained type).
- [x] 1.2 Red case, recorded: `Tests/src/StyleConfigLookupCostTest.cpp` case "Defined but unreferenced types
      add no build work" carries `REQUIRE( smallDiagnostics.typeSetBytes==largeDiagnostics.typeSetBytes )`
      (now `:416`). On HEAD it fails: expansion `3024 (0xbd0) == 14224 (0x3790)`, test cases: 11 | 10 passed |
      1 failed, assertions: 73 | 72 passed | 1 failed, run at 2026-10-16T11:06:43Z. Retained as
      `evidence/red-head-ctest-StyleConfigLookupCost.log`
      (sha256 `164c4ddb7c2223d80d988d780a0826be5320746901a02edafd4a8d4859e4bb17`).
      Parent requirement: style-configuration / "Style-configuration build cost is observable".
- [x] 1.3 The figure is summed over the per-level sets of four families
      (`StyleConfig.cpp:778`, `:882`, `:1003`, `:1052`); no other caller of `TypeSetBytes` exists. No other
      tree location names the figure (`grep -rn 'typeSetBytes\|615 600' Tests/ libosmscout*/src
      libosmscout*/include TODO.md`: the test, `StyleConfig.h`/`.cpp` and the archived changes only).

## 2. The single fix

- [x] 2.1 `libosmscout/include/osmscout/TypeInfoSet.h`: add
      `size_t AllocatedBytes() const`, returning
      `words.capacity()*sizeof(uint64_t)+contained.capacity()*sizeof(TypeInfoRef)`, with the doc comment
      that names the storage it reports. `Size()` cannot serve: it is the contained type count, asserted by
      the model case of the set and used by the lookup positions of the configuration. Verifies spec
      type-set / "The bytes a type set allocates follow the types it contains".
- [x] 2.2 `libosmscout-map/src/osmscoutmap/StyleConfig.cpp`: `TypeSetBytes` sums
      `sizeof(TypeInfoSet)+set.AllocatedBytes()` over the sets and drops the formula that repeats the
      representation; its comment says so. Verifies spec style-configuration / "Style-configuration build
      cost is observable".
- [x] 2.3 `libosmscout-map/include/osmscoutmap/StyleConfig.h`: the comment of `typeSetBytes` names the sets'
      bytes instead of an estimate.

## 3. Cases and probes

- [x] 3.1 `Tests/src/StyleConfigLookupCostTest.cpp` "Defined but unreferenced types add no build work":
      the equality of the two reported figures is asserted and the comment that read the figure as an upper
      bound is replaced. Case: "Defined but unreferenced types add no build work" — scenario "The reported
      type-set bytes follow the retained sets".
- [x] 3.2 `Tests/src/TypeInfoSetStorageTest.cpp`: new case "A set reports the bytes it allocates" asserts
      `set.AllocatedBytes()==bytes` against the bytes the construction allocated (the file's allocation
      counter), guarded by `TYPE_SET_HAVE_ALLOCATION_COUNTER` like its neighbours. Scenario "A set reports
      the bitmap bytes it allocates". The case builds only an empty set of a 1527-type configuration, so
      the counter is compared to the bitmap term alone (`words.capacity()*sizeof(uint64_t)`); the
      contained-type term (`contained.capacity()*sizeof(TypeInfoRef)`) is not covered by a case.
- [x] 3.3 Probe (`TEMP-REVERT-PROBE`): restore the formula of HEAD inside `TypeSetBytes`, run the scaling
      case through the CMake runner, expect exactly the one case to fail on
      `smallDiagnostics.typeSetBytes==largeDiagnostics.typeSetBytes`; archive the log under `evidence/` and
      restore the fix. The case guards an improvement's number, so the probe moves the production constant
      that produced the old number; nothing is claimed about an earlier bug.

## 4. Gate (product code delta, both build systems)

- [x] 4.1 `cmake --build build -j "$(nproc)"` — exit 0, no errors or warnings.
- [x] 4.2 `cd build && QT_QPA_PLATFORM=offscreen TESTS_TOP_DIR="$PWD/../Tests" TESTS_TMP_DIR="$PWD/Tests"
      ctest -j 8 -E 'NumberSetPerformanceTest' --output-on-failure` — the case tally is
      `100% tests passed out of 158`; the performance test is excluded because the change is not about what
      it measures (`NumberSetPerformanceTest`), and the exclusion is named in the evidence copy.
- [x] 4.3 `meson compile -C debug` — exit 0; then `meson test -C debug --timeout-multiplier 2
      --print-errorlogs`, whose summary lines are `Ok:                159` and `Fail:              0  `, i.e. 159
      tests, none failed.
- [x] 4.4 The two figure cases still pass: `ctest -R 'StyleConfigLookupCost|TypeInfoSetStorage'` and the
      meson runner for the same two binaries; the shipped-stylesheet measurement case reports its new
      figure.

## 5. Bookkeeping and evidence

- [x] 5.1 Every cited number carries a log under `evidence/` with command, UTC timestamp, exit code and
      `sha256sum`; `evidence-check.sh` run live before the gate and quoted in the verification note.
- [x] 5.2 Formatter: `uncrustify -q -c .uncrustify -l CPP -f <touched file>` adds no drift: the four
      files that carry the tree's pre-existing alignment drift (81 down to 8 differing lines,
      `evidence/uncrustify.log`) are unchanged by this change, none of their differing lines is one this
      change adds or edits, and `Tests/src/StyleConfigLookupCostTest.cpp` is clean.
- [x] 5.3 `guidelines/FileFormatVersion.md`: the accessor adds no stored or serialized layout and no type
      configuration content, so no version bump. `guidelines/CodeStyles.md`: the accessor carries its doc
      comment.
- [x] 5.4 `TODO.md` §104: the entry is deleted in phase F, on the orchestrator's instruction, because
      this run was started with `--allow-todo-removal yes` and the archive exists; the closure is recorded by
      this archived change instead.
- [x] 5.5 Note, no edit: the archived changes
      `openspec/changes/archive/2026-10-16-fix-typeinfoset-bitmap-storage` (its
      `openspec/changes/archive/2026-10-16-fix-typeinfoset-bitmap-storage/verification.md:177`) and
      `openspec/changes/archive/2026-10-15-improve-styleconfig-lookup-tables` (its
      `openspec/changes/archive/2026-10-15-improve-styleconfig-lookup-tables/design.md:229`) keep the `3024`
      and `615 600` figures as the record of those changes; an archive is history and is not updated. The
      figure this change reports for the shipped configuration is recorded in this change's evidence
      instead.

## 6. Residue

- [x] 6.1 Pending, no host seam: none. The change is host-testable end to end.
