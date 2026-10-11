# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/search_scope.h` holds the pure scope-expansion rule (how a hierarchy depth
  maps onto the OSM admin_level scale, and from which parent level a search may widen its scope). It is
  deliberately dependency-free so it can be host-tested; `Tests/src/SearchScopeTest.cpp` covers it.
- `libosmscout-client-java/src/OSMScoutClient.cpp`: a resolved default admin region is applied as a
  `LocationStringSearchParameter` region and as a region object offset. Both are meaningful only inside the
  database that owns the region handle; every other loaded database is searched unconstrained, and the
  free-text index is scope-blind in every database.
- The region is a name plus an object reference; it carries no coordinates of its own, so a geographic
  extent has to be derived from the object that represents it.
- A region's object can be an area, a way or a node, and it may not be loadable at the time the search runs.

## Goals / Non-Goals

**Goals:**

- Narrow a scoped search to the resolved region's geography in every loaded database, including free-text
  hits.
- Never let an unknown extent empty a search: the filter degrades to the unscoped behaviour.
- Give a consumer the verdict instead of forcing it to re-derive a region's area.
- Keep the decisions in a dependency-free, host-tested helper, next to the existing expansion rule.

**Non-Goals:**

- Changing how the default admin region is resolved or which region is chosen.
- Replacing the region-based scoping inside the owning database; the extent is an additional, comparable
  filter.
- Filtering structured hits that the owning database already scopes (they stay as they are; the extent is
  what makes a hit comparable across databases).
- Adding a geographic index or changing the location service.
- Antimeridian-crossing extents (no installed data needs one).

## Decisions

**D1 — The scope is narrowed by a geographic extent, not by the region identity.**
The extent is derived from the region's object and applied to hits of every database.
Alternatives:
- *Apply the region parameter to every database*: region offsets are not comparable across databases, so a
  handle from one database scopes nothing in another.
- *Disable the free-text index while a scope is active*: removes the hits a user most often wants (a street
  typed as free text) instead of narrowing them.
- *Post-filter by the region name*: a name is not a geography; two same-named regions would keep admitting
  each other's hits, and a hit's region name is often empty.
Chosen because only a geometry makes hits from different databases comparable, and the reported verdict
lets a consumer keep a near-miss visible if it wants to.

**D2 — The extent comes from the region's own object.**
An area or a way contributes its bounding box; a region represented only by a position gets a documented
box around it (about 28 km of latitude half-size: wider than a city, narrower than a district).
Alternatives:
- *The database's bounding box*: far too coarse; it admits the whole map file.
- *The region's name matched against the hit's region chain*: needs every hit to carry a comparable chain,
  which the text index does not provide.
- *No extent for a node region*: would leave exactly the small, point-like regions uncluttered and
  reimport the noise for them.
Chosen because the object is the only per-region geometry the search has, and the node fallback is a
documented, bounded approximation.

**D3 — An unestablished extent means fail-open.**
No region, no loadable object or an invalid box leaves the filter off.
Alternatives:
- *Fail-closed (admit nothing)*: a scope whose area is unknown would silently empty the search, which is
  worse than the noise being fixed.
- *Reject the scoped search and require an unscoped retry*: the caller cannot tell the two situations apart
  and would have to implement the retry itself.
Chosen because a degraded filter must never reduce the result set below what the unscoped search finds.

**D4 — The verdict travels with the result.**
Each result reports whether it lies inside the extent; the default is "inside", so an older bridge and an
older consumer keep working together.
Alternatives:
- *Let a consumer re-derive the region's area*: it would have to load region objects on the app side and
  duplicate the derivation.
- *Return only narrowed results*: a consumer that wants a near-miss for ranking could not see one, and it
  would hide why a hit was dropped.
Chosen because the fact is native-side and comparable there; the consumer keeps the ranking policy.

**D5 — The geographic decision is a dependency-free helper.**
The box type, the derivation and the containment tests live in `search_scope.h` beside the expansion rule,
so they are host-tested without a database or a region object.
Alternatives:
- *Reuse the core `GeoBox` and the location service's helpers*: fewer types, but the helper would need
  libosmscout includes and could no longer be included by a host test without linking a database library.
- *Inline the arithmetic in the bridge*: the decision would then only be testable through JNI and a
  database, which is how the expansion rule's neighbours avoided the same gap.
Chosen because the property that matters — the extent is a superset of the region — is a pure function and
must be pinned by a test. A review that prefers the core type may swap it without changing the rule; that is
recorded as an open question.

## Sequence diagram

```
search caller            OSMScoutClient (JNI)              region object              databases
    |                          |                                |                        |
    | search(query, region) -->| resolve default admin region   |                        |
    |                          | load the region's object ----->|                        |
    |                          |<-- area / way / node / nothing-|                        |
    |                          | derive extent (or leave it unset)                       |
    |                          |-----------------------------------------------> search each db
    |                          |<---------------------------------------------- hits, incl. free text
    |                          | for each hit:                                           |
    |                          |   extent set? -> is the position inside it?             |
    |                          |   no extent?  -> admit                                  |
    |                          |   non-finite position with a set extent? -> reject      |
    |<-- results + inSearchScope verdict per result -------------------------------------|
```

## Risks / Trade-offs

- *A region's bounding box admits a hit that the region does not contain* → the box is a superset, and the
  requirement states it; the filter can only admit extra hits, never drop an in-region one.
- *A node region's fixed-size box is wrong for an unusually large or small region* → the size is documented
  and is a compromise between city and district scale; a caller sees the verdict and may reorder.
- *A scoped search now returns fewer results, which a consumer may read as "no results"* → a hit is only
  dropped when its position is known and outside the extent, and an unknown extent admits everything.
- *A hit without a usable position* → it is rejected by a set extent; an unscoped search still returns it.
- *The helper duplicates a box type the core library already has* → recorded as an open question; the rule
  is independent of the type, and the host test is the reason for the dependency-free choice.
- *The JNI part cannot be host-tested* → the geographic decision is host-tested; the bridge part is a
  derivation from a loaded object plus one containment call per hit.

## Migration Plan

Behavioural, additive at the type level: the search request is unchanged, one field is added to a result
with a default that reproduces the previous behaviour, and no data is persisted. A scoped search returns
fewer foreign hits, which is the point of the change. Rollback is a revert of the commits, which restores
the unscoped free-text search.

## Verification

- The geographic decision — building the box, normalizing and clamping it, containing a position, the
  fail-open rule for an unset box, the rejection of a non-finite position and the superset property — is
  host-tested: `Tests/src/SearchScopeTest.cpp` runs it without a database, which is the reason the helper in
  `search_scope.h` carries no libosmscout include (D5).
- The bridge part — deriving the extent from a loaded region object, applying it to free-text hits and
  writing the verdict into the result — cannot be host-tested: it needs an open database and a resolved
  admin region. The map databases on this machine are format v26 while the library expects v27 and cannot
  be opened, and importing a v27 database is out of scope for this change. That part is therefore verified
  by compiling it in both build systems, by `scripts/check-jni-signatures.sh` (no signature changed), and by
  the field being present in the built jar. The end-to-end map check stays undone and is recorded as such in
  `tasks.md` (4.4) and `verification.md`.
- The scenarios of this change that need a database (a hit inside or outside the extent being admitted or
  dropped, the per-result verdict of a scoped search) therefore have no executable test in this repository.
  The rules they state are the ones the host test pins, applied to a position the bridge obtains from the
  owning database.

## Open Questions

- Whether the helper should use the core `GeoBox` type instead of its own box, accepting a libosmscout
  include in the header: deferrable, and a reviewer-level decision that does not change the rule.
- Whether a very large region (a country) should be excluded from the filter as too coarse: deferrable, no
  installed data makes a country the default region today.
- Whether the box size for a node region should be a per-region value instead of one constant: deferrable,
  it needs data the region object does not carry.
