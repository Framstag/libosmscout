# Proposal: fix-compound-name-matching

## Why

A free-text search cannot find a named object whose stored name joins words with a separator when
the query spells those words apart. Searching "Hilpert Theater Lünen" yields nothing although the
POI "Heinz-Hilpert-Theater Lünen" is present in the index; the same holds for streets
("August Warkner Platz" vs. "August-Warkner-Platz"), postal areas and administrative regions. The
query's words are only ever compared against the stored name as one contiguous string, so a
separator inside the name — or in the query — hides the object from the search. The free-text text
index of the same search behaves the same way from the other side: it compares the whole normalized
query as a prefix of the whole stored name, so a name that joins its words with a separator is
invisible to a query that spells them apart.

## What Changes

- Location and POI string search matches a query's consecutive words against a consecutive run of
  the stored name's words in the same order, independently of the separator characters used on
  either side (space, hyphen, slash, dash).
- The tolerance is bounded and defined: only consecutive runs in query order match; a query that
  covers the whole stored name is a full match, a query covering only part of it is a candidate.
- Every object kind the search covers (POIs, locations/streets, administrative regions, postal
  areas) follows the same rule, and transliteration/letter-case tolerance keeps applying across the
  separator.
- Behavior is only extended: queries that already matched return exactly the same results.

Out of scope for this change (documented for follow-up work): names whose words are joined without
any separator; reaching a word in the middle of a stored name when the query carries no region
qualifier; and the free-text text index, which still misses a separator-joined name because its
prefix comparison is over whole stored names. Widening the index would key it by word or by name
suffix, which changes the index format and requires every database to be re-imported and
redistributed. This change therefore makes separator-insensitive matching apply to the search paths
that compare a query against a name at query time, including how a hit from the text index is
classified as a match or a candidate.

## Capabilities

**New Capabilities**

- None.

**Modified Capabilities**

- `location-search-api` — the free-text string search contract gains the separator-insensitive
  matching rule, its order/consecutiveness boundary, the match-quality semantics and the
  no-regression guarantee for existing substring and prefix matching.

## Impact

Affected files and modules:

- `libosmscout/include/osmscout/util/StringMatcher.h`, `libosmscout/src/osmscout/util/StringMatcher.cpp`
  — the comparison used for name matching (existing matcher behavior stays unchanged for its
  current callers).
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the search parameters that carry the query
  string, the form-based address search, the admin-region resolution and the classification of a
  text-index hit as a match or a candidate.
- `Tests/LocationTest.olt`, `Tests/src/SearchForLocationByStringTest.cpp`,
  `Tests/src/SearchForLocationByFormTest.cpp`, `Tests/src/StringMatcherTest.cpp` — test data and
  regression coverage, registered in both `Tests/CMakeLists.txt` and `Tests/meson.build`.
- No index or file-format change: matching happens at query time, so existing databases stay usable
  without re-import.
