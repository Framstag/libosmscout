# Proposal

## Why

A POI search result identifies a place by a display label only. For many places the more useful
identity is who runs them or which chain they belong to — a bank branch, a fuel station, an ATM —
and that information is already part of the map data. Callers cannot reach it, so an application
that wants to show an operator or a brand next to a result has to guess it from the label, if it
manages at all. This is being fixed now because the client work is being split into small
single-goal changes, and this is the smallest self-contained one.

## What Changes

- A POI search result gains two optional descriptive attributes: the operator and the brand of the
  found object.
- Both attributes are empty when the found object carries no such information. An empty attribute is
  normal and raises no error; it SHALL NOT be treated as a value.
- Retrieving the attributes does not change which objects a search finds, how many it returns, or
  the order it returns them in.
- The existing label behaviour is unchanged: when a found object has no name, the label may still be
  derived from its operator. A result may therefore show the same text as its label and as its
  operator, and that is expected.
- The two attributes are documented on the client result type for callers.

## Capabilities

### New Capabilities

None. The two attributes are part of the existing POI search result contract, which an existing
capability already owns.

### Modified Capabilities

- `poi-search-api`: the result data class gains the operator and brand attributes, including the
  rule that they are empty when the found object carries no such information, and that the presence
  or absence of a value does not affect which results are returned or in what order.

## Impact

- `libosmscout-client-java/src/OSMScoutClient.cpp` — POI result building and result conversion.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/PoiEntry.java` — the result type
  gains the two attributes.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/PoiEntryTest.java` — model-level
  assertions for the new attributes.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/OSMScoutClientPoiSearchTest.java` — an
  end-to-end assertion that a search result carries an operator when the searched object has one;
  guarded by the test's existing database-directory assumption.
- `openspec/specs/poi-search-api/spec.md` — updated through this change's spec delta.

Unchanged by this change:

- The POI category mapping, the search radius and limit behaviour, and result ordering.
- The JavaScout result list, which continues to show label, type and distance only.
- The native POI service, the import pipeline, and the type configuration.
