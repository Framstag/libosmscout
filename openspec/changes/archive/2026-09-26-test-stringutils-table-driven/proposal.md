# Proposal

## Why

The transliteration, diacritic, match-quality and case-insensitivity expectations of the string
matcher are pinned in four separate test cases in `Tests/src/StringUtilsTest.cpp`, and each of them
repeats the same matcher-and-assert boilerplate. The repetition is what the project's duplicate-code
metric counts as new duplicated code, so adding an expectation to any of the four aspects costs a
copied block rather than a row.

## What Changes

- The expectations of the four cases (sharp s, diacritics, match quality, folding is opt-in) are
  held in one table and asserted in one loop, so each expectation is a row and the boilerplate
  exists once.
- Every expectation that exists today keeps existing, with the same pattern, the same text and the
  same expected match quality. No expectation is dropped, added, weakened or reordered in meaning.
- The test reports the same verdict for the same input: the observable behavior under test does not
  change, and neither does any test binary, test name registration or build description.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

None. This is a test-only refactor: no requirement, contract or observable behavior changes, so the
change carries no spec delta and its `.openspec.yaml` sets `skip_specs: true`. Inventing a
capability for it would describe the test file rather than the product.

## Impact

- `Tests/src/StringUtilsTest.cpp` — the only file that changes. Four `TEST_CASE`s are replaced by one
  `TEST_CASE` over a `MatchExpectation` table plus one helper that asserts the table.
- No production code, no public API, no build description (`Tests/CMakeLists.txt`, `Tests/meson.build`),
  no test data and no build-system behavior is touched.
- The capability the expectations cover (`StringMatcher`, used by the location search) is unchanged;
  this change only changes where its expectations are written down.
