# Tasks

Spec references: none — this change declares no capability and sets `skip_specs: true` in
`.openspec.yaml` (pure test refactor, no spec-level behavior changes). Each task therefore names the
proposal change it serves and how its completion is verified.

## 1. Pin the current coverage

- [x] 1.1 Count the expectations in the four cases at the end of `Tests/src/StringUtilsTest.cpp`
  (sharp s, diacritics, match quality, folding is opt-in) and record the count and the two factories
  they use, so the refactor can be shown to move every expectation and no more
  (change proposal — What Changes; verify: the count of `REQUIRE` lines in those four cases is 20 —
  7 + 4 + 7 + 2 — over `StringMatcherTransliterateFactory` and `StringMatcherCIFactory`).

- [x] 1.2 Record which rows belong to which of the four aspects, so the moved rows keep their
  comments and a reviewer can still separate them
  (change design — D2; verify: each of the 20 expectations has a source comment or a source position
  it can be traced back to).

## 2. Refactor the case

- [x] 2.1 Add the includes the refactor needs to `Tests/src/StringUtilsTest.cpp` — `std::size_t`,
  `std::string` and `std::vector` — in the alphabetical order the style guide asks for, and verify the
  file still compiles as part of the `StringUtilsTest` target
  (change proposal — Impact; change design — D2; verify: the `StringUtilsTest` target builds).

- [x] 2.2 Introduce the `MatchExpectation` row type — constructible from its four values, per D4 —
  and the `RequireMatchQuality()` helper that turns a row into one matcher call and one assertion,
  selecting the transliterating or the case-insensitive factory from the row
  (change design — D1, D3, D4; verify: the helper compiles and the file's other cases are unaffected).

- [x] 2.3 Move all 20 expectations from the four old cases into the table, verbatim, keeping their
  comments, and replace the four cases with the one table-driven `TEST_CASE`
  (change design — D2; verify: the table holds exactly 20 rows and the four old case names no longer
  exist in the file).

- [x] 2.4 Build and run the `StringUtilsTest` target and verify every expectation passes with the
  same verdicts as before
  (change proposal — What Changes; verify: the test passes and no expectation was weakened — flipping
  one row's expected quality makes it fail, then reverted).

- [x] 2.5 Verify no expectation was silently dropped: compare the assertions the four old cases made
  with the rows of the new table one by one
  (change proposal — What Changes; verify: the sets of (pattern, text, expected quality) pairs are
  equal — 20 pairs, identical — and the only added information is the factory flag, which is `false`
  on exactly the two rows of the old case-insensitive case).

## 3. Build and regression

- [x] 3.1 Configure and build with CMake and with Meson and verify both complete without errors or new
  warnings in the touched file
  (change design — Constraints; verify: both builds succeed and neither reports a warning for
  `Tests/src/StringUtilsTest.cpp`).

- [x] 3.2 Run the complete host test suite and verify no test regresses and no new warning appears
  (change proposal — What Changes; verify: 133 of 133 configured tests pass).

## 4. Conventions and documentation

- [x] 4.1 Check every new or modified line of `Tests/src/StringUtilsTest.cpp` against the documents in
  `guidelines/` and verify it satisfies them, in particular the naming, comment and include-order
  rules, and run `.clang-tidy` over the touched file
  (change design — Constraints; verify: no finding is reported on any line the change adds or edits,
  and the file's total finding count does not rise — it falls from 20 to 19).

- [x] 4.2 Check the touched file against `.uncrustify` and verify the change introduces no formatting
  proposal that the committed style does not already contain
  (change design — Constraints; verify: the residual proposal of the formatter is smaller than on the
  unmodified file and covers only the pointer-alignment preference the repository's own declarations
  do not follow).

- [x] 4.3 Confirm that no documentation describes how the matcher expectations are written down, so
  this refactor needs no documentation change
  (change proposal — Impact; verify: the search over `Documentation/`, `guidelines/` and `AGENTS.md`
  describes the matcher's behavior, not the shape of the test that pins it).

## Verification Notes

- Host verification: CMake Release build with the Java client enabled and the Meson build both
  complete without errors, and neither reports a warning for `Tests/src/StringUtilsTest.cpp`. In both,
  the warnings that are reported come from pre-existing code.
- `ctest -j 4` reports 133 of 133 tests passing, `StringUtilsTest` included, and
  `meson test -C build-meson "Check string utils"` passes.
- Set equality against the unmodified file: the four old cases and the 20 table rows yield the same
  20 (pattern, text, expected quality) pairs, compared as sorted sets.
- Mutation check (task 2.4): flipping one row's expected quality from `noMatch` to `match` makes
  `StringUtilsTest` fail, so the table is really asserted and not merely constructed.
- `clang-tidy -p build Tests/src/StringUtilsTest.cpp` reports 19 findings for the modified file
  against 20 for the unmodified one, and none of them sits on an added or edited line. The single
  finding this change would otherwise add — two adjacent `const char*` row-constructor parameters —
  is suppressed with a reason, see design D4. The 20 designated-initializer findings an aggregate row
  type would produce are the reason the row type has a constructor.
- `.uncrustify` on the modified file proposes 214 diff lines against 238 on the unmodified file. The
  remainder is the formatter's pointer-alignment preference, which the repository's own declarations
  do not follow, so the committed style is kept.
- The change is a test-only refactor of one file. It ships as branch `test-stringutils-table-driven`
  off `origin/master`, extracted as the second selective part of the `naviveylin-local` branch
  (PR #1773), which carries it together with the compound-name matching feature and unrelated work.
