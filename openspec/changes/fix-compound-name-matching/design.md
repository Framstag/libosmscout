# Design: fix-compound-name-matching

## Context

Name comparison in the string search is a single case-folded, transliterated substring test
(`StringMatcherTransliterate::Match`) against the stored name. The search builds prefixes and
suffixes of the query's remaining words as patterns (`GenerateSearchPatterns`) and keeps a partial
hit only when the pattern covers the whole remaining word list. A name that joins words with a
separator therefore hides from a query that spells those words apart: the multi-word pattern is not
a substring, and the single-word patterns leave words uncovered.

The same search also reads a free-text text index, whose lookup normalizes the whole query and
compares it as a byte prefix of the whole stored name. A separator inside the name is preserved by
that normalization (only whitespace-like characters collapse to a space), so a separator-joined name
hides from the index as well and the index contributes nothing for such a query.

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

### D3 — Every name comparison on the search path uses the same rule

The query-string search, the admin-region resolution and the form-based address search are all
driven by the same query words, so all three get the new matcher. Otherwise a hyphen-joined city,
street or postal area would match in one path and not in another — and typed full addresses (which
use the form search) would stay broken.

The fourth comparison on the same path decides whether a hit that came from the free-text text index
is reported as a match or a candidate. It compares the same query against the same kind of name, so
it uses the same matcher and the two paths agree on match quality. Swapping it is safe in both
directions: the word-aware matcher returns the substring result unchanged whenever that result is
not a non-match, so a hit can only gain the quality it deserves and never lose one. Membership of
the result set is unaffected — that site classifies an entry the index has already returned.

The index itself keeps its reach in this change (see D4).

### D4 — Leave the text index format alone

Making the text index find a separator-joined name means keying it by word or by name suffix instead
of by whole name. The index is built by the importer and shipped inside the database, so that change
forces every database to be re-imported and redistributed. It is therefore rejected for this change:
the search paths that compare a query against a name at query time are fixed, the index keys are not.
A query whose word run sits in the middle of a stored name still relies on the query-time paths (and,
for the index, on a region qualifier that narrows the candidate set).

## Verification

- Unit tests for the matcher: separator variants on either side, transliteration and sharp-s across
  the separator, reordered/interrupted runs, full versus partial coverage, unchanged substring
  cases, and the fast path taken for single-word patterns.
- Search-level tests against the committed test data (`Tests/LocationTest.olt`): a hyphen-joined
  location (already present as `August-Warkner-Platz`) and a hyphen-joined POI, queried as words
  spelled apart, through the string search and the form search.
- The four comparison sites in `libosmscout-client-java/src/OSMScoutClient.cpp` are checked by
  inspection: no search parameter and no match-quality classification still builds the previous
  factory (grep for the previous factory type returns no hit). These sites cannot be covered by a
  unit test — they need the JNI bridge, a real database and, for the index, marisa — so the
  capability itself is pinned by the search-level tests above and the wiring by this check.
- Full test suite on the host build (Tests, Import, marisa enabled) green, and no regression in the
  existing search sections whose names are space-separated.

## Risks

| risk | mitigation |
|---|---|
| more candidates per query | added matches are candidates, ranked below full matches; candidate limits are unchanged |
| rule too lenient | consecutive, ordered, whole-word runs only; negative tests pin the boundary |
| cost on the query path | substring fast path first, word path only on a miss, split performed once per candidate comparison and abandoned on the first non-matching word |
| upstream merge conflicts | additive class plus factory, no behavioral edit to existing matchers, single-purpose commit |
| recall via the text index unchanged | documented as out of scope (D4); the query-time paths return such an object as a candidate, which is what the user sees |
| match quality changes for a text-index hit | only upward, only for a query that matches the whole stored name modulo separators; membership is untouched |
