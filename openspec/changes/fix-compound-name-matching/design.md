# Design: fix-compound-name-matching

## Context

Name comparison in the string search is a single case-folded, transliterated substring test
(`StringMatcherTransliterate::Match`) against the stored name. The search builds prefixes and
suffixes of the query's remaining words as patterns (`GenerateSearchPatterns`) and keeps a partial
hit only when the pattern covers the whole remaining word list. A name that joins words with a
separator therefore hides from a query that spells those words apart: the multi-word pattern is not
a substring, and the single-word patterns leave words uncovered.

## Decisions

### D1 — Extend the matcher, not the search visitors

Three options were considered: (a) a word-aware matcher used by the search parameters, (b) word
splitting inside the search visitors, (c) making the free-text index word-keyed.

(b) scatters name comparison across three visitors and duplicates the pattern-side handling; (c)
changes the index format, so every database must be re-imported and redistributed before anything
improves. (a) keeps the rule in one place and is purely additive for existing callers, so the
existing matcher classes and their behavior stay untouched for other clients.

### D2 — Evaluation order and boundary

The matcher keeps the substring test as its first step (fast path, identical result to today) and
only then splits the stored name and the pattern into words — separated by whitespace, comma,
hyphen, en dash, em dash, slash or backslash — and looks for the pattern's words as a consecutive
run of the stored name's words in the same order. A run covering the whole name is a full match, a
run covering part of it a candidate, no run a non-match.

Rationale: the common case stays allocation-free and provably unchanged; the added tolerance is
narrow (no reordering, no gaps, no word-internal splitting); the candidate quality keeps added
entries below exact matches in every consumer's ranking. Periods are deliberately not separators,
so dot-spelled names do not start matching in this change.

### D3 — Every search parameter uses the same rule

The query-string search, the admin-region resolution and the form-based address search are all
driven by the same query words, so all three get the new matcher. Otherwise a hyphen-joined city,
street or postal area would match in one path and not in another — and typed full addresses (which
use the form search) would stay broken.

## Verification

- Unit tests for the matcher: separator variants on either side, transliteration and sharp-s across
  the separator, reordered/interrupted runs, full versus partial coverage, unchanged substring
  cases, and the fast path taken for single-word patterns.
- Search-level tests against the committed test data (`Tests/LocationTest.olt`): a hyphen-joined
  location (already present as `August-Warkner-Platz`) and a hyphen-joined POI, queried as words
  spelled apart, through the string search and the form search.
- Full test suite on the host build (Tests, Import, marisa enabled) green, and no regression in the
  existing search sections whose names are space-separated.

## Risks

| risk | mitigation |
|---|---|
| more candidates per query | added matches are candidates, ranked below full matches; candidate limits are unchanged |
| rule too lenient | consecutive, ordered, whole-word runs only; negative tests pin the boundary |
| cost on the query path | substring fast path first, word path only on a miss, split performed once per candidate comparison and abandoned on the first non-matching word |
| upstream merge conflicts | additive class plus factory, no behavioral edit to existing matchers, single-purpose commit |
