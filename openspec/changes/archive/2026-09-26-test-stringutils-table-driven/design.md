# Design

## Context

See `proposal.md` — Why.

Current state of `Tests/src/StringUtilsTest.cpp`: four `TEST_CASE`s at the end of the file each build
a factory, wrap it in a local `quality(pattern, text)` lambda and then assert one `REQUIRE` per
expectation. The four cases are `Transliterated matcher folds the sharp s` (7 expectations),
`Transliterated matcher folds diacritics` (4), `Transliterated matcher keeps match quality` (7) and
`Case-insensitive matcher does not fold` (2) — 20 expectations over two factories
(`StringMatcherTransliterateFactory`, `StringMatcherCIFactory`) and one result type
(`osmscout::StringMatcher::Result`).

Constraints that shape the approach: the file is part of the `StringUtilsTest` target registered in
both `Tests/CMakeLists.txt` and `Tests/meson.build`, and its test name is referenced by neither, so
the target must keep building and passing without a build-description change. The repository style
guide and `.uncrustify` apply to the changed lines.

## Goals / Non-Goals

**Goals:**

- One place for the boilerplate, one row per expectation, same verdicts.
- Keep the two-factory, three-result-type surface visible at the point of assertion, so a reader can
  see which factory and which quality a row asks for without reading a helper.
- Leave the file's other cases, its includes' order and its target untouched.

**Non-Goals:**

- Moving matcher expectations into the dedicated matcher test binary (`Tests/src/StringMatcherTest.cpp`,
  added by the companion change `fix-compound-name-matching`) — that file belongs to another change.
- Changing the matcher, the search code or any production behavior.
- Adding new expectations. The boundary cases of the word-aware matcher live in the matcher test; this
  change only re-expresses what already exists.

There is no non-trivial control flow in this change, so the design carries no sequence diagram.

## Decisions

### D1 — One table in the existing file, not a helper-only extraction

Alternatives: (a) one table-driven case in `StringUtilsTest.cpp`, (b) keep the four cases and extract
the shared `quality` lambda into a file-local helper, (c) move the expectations into the matcher test
binary added by the companion change.

Chosen: (a). (b) removes the lambda duplication but leaves one `REQUIRE` line per expectation, which
is the bulk of the repeated code the metric sees, so the metric keeps firing on the next added
expectation. (c) would make this pull request depend on the matcher pull request and would put
pre-existing transliteration coverage into a file whose subject is the word-match rule; a reviewer of
either change would have to read the other.

### D2 — Plain table plus a loop, not a Catch2 generator or a macro

Alternatives: (a) a `MatchExpectation` array and one loop, (b) a Catch2 data generator
(`GENERATE(...)`) so every row is reported as its own run, (c) a macro that expands one expectation
into the four lines it replaces.

Chosen: (a). (b) gives per-row reporting but changes the failure output and pulls in a generator
header, and the file currently uses no generators; its value here is small because the expectations
are exact-quality comparisons that fail immediately and point at the row's line. (c) hides the
factory and the result type behind a macro, which is worse to read than the table and is not used in
the test suite. The table is a `std::vector<MatchExpectation>` and the helper takes it by const
reference, so the row count is not written down twice, no C-style array is declared and no iterator
or index arithmetic is needed in the loop.

### D3 — Table row carries the factory choice, not the factory object

The `transliterate` flag selects between the two factories inside the helper. Alternative: store a
`StringMatcherFactory*` in the row. Rejected because it would put two factory objects in the table's
scope and make each row longer for no gain; a plain flag states the intent ("this row exercises
folding opt-in") directly and keeps the two factory objects constructed once for all rows.

### D4 — The row type has a constructor, and the table initializes by braces

Alternatives: (a) an aggregate row type with brace-initialized rows, (b) a row type with a
constructor, (c) an aggregate row type with `NOLINT` on each row.

Chosen: (b). The project's `.clang-tidy` enables `modernize-*` broadly while the code is C++20, so an
aggregate row type makes `modernize-use-designated-initializers` report one finding per row — 20 new
findings for a test-only refactor. A user-declared constructor makes the type non-aggregate and the
check no longer applies, at the cost of four lines. (c) would put a suppression comment on every row,
which is more noise than the constructor. The constructor takes two adjacent `const char*`
parameters, which `bugprone-easily-swappable-parameters` reports once; that single finding is
suppressed with a `NOLINTNEXTLINE` and a reason, the pattern `libosmscout/src/osmscout/io/Crc32.cpp`
already uses for the same check.

## Risks / Trade-offs

- [A row silently changed while moving it] → Every expectation is moved verbatim with its source
  comment; the task list pins the row count at 20 and the suite is run before and after.
- [Loss of per-aspect test naming in the test report] → The comments on the rows keep the four aspects
  separable, and the single case name states all four; a failing row points at its exact line.
- [Table type limits a future expectation to `const char*` and one result type] → Both are what the
  matcher's API takes; if a future expectation needs more, it is a new row field and a constructor
  parameter, not a redesign.
- [The constructor exists for the linter, not for the test] → It is four lines and carries one
  suppression; the alternative was 20 findings, and the reason is written next to it (D4).
- [The include block is reordered while adding `std::vector`] → The guideline asks for standard
  headers in alphabetical order and the block is touched anyway; the reorder covers only those five
  lines.
- [The refactor is not required by any product change] → It is scoped to one file and verified by the
  existing suite; if a reviewer rejects it, nothing else in the branch depends on it.
