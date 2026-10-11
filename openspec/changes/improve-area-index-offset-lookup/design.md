# Design

## Context

See `proposal.md` — Why. What shapes the approach:

- `AreaIndex` (`libosmscout/include/osmscout/db/AreaIndex.h`) is a base class. `AreaWayIndex`
  (`areaway.idx`) and `AreaRouteIndex` (`arearoute.idx`) derive from it; `AreaAreaIndex` and
  `AreaNodeIndex` are separate classes. `AreaNodeIndex::GetOffsets` already iterates the requested
  `TypeInfoSet` (`libosmscout/src/osmscout/db/AreaNodeIndex.cpp:337-346`), so only the base class's
  lookup is request-independent, and fixing the base fixes both of its derived indexes at once.
- `typeData` holds one entry per index entry, in file order (`AreaIndex.cpp:86-110`), and the entry's
  type is read by the derived class: `AreaWayIndex::ReadTypeData` reads a *way* type id
  (`AreaWayIndex.cpp:30-36`), `AreaRouteIndex::ReadTypeData` a *route* type id
  (`AreaRouteIndex.cpp:30-38`). `typeData[i]` is therefore **not** addressable by an area type id,
  which is why the pattern of the node index does not transfer literally.
- The request arrives as a `TypeInfoSet`, which is dense over the global type index:
  `IsSet()` tests `types[type->GetIndex()]` (`TypeInfoSet.h:160-166`) and `TypeInfo::GetIndex()` is in
  `[0..GetTypeCount())` (`TypeConfig.h:306-308`, `:1284`). The global type index is the one key that
  the request and the entries share, whatever type space the derived class read.
- Today's loop reports `loadedTypes.Set(data.type)` for every *requested* type that has an entry —
  including an entry with `bitmapOffset==0` that resolves nothing (`AreaIndex.cpp:218-225`), and with
  no `IsInternal()` filter.

## Goals / Non-Goals

**Goals**

- The number of index entries a lookup examines follows the requested types.
- The result (resolved offsets, reported loaded types) is exactly what the current lookup produces.
- The cost behaviour is observable by a test, so the spec's scenarios are assertions rather than
  measurements of wall-clock time.

**Non-Goals**

- No change to the public lookup signature, to the index files, to the importer, or to any database.
- No change to `AreaNodeIndex` (already request-bound) or to `AreaAreaIndex` (own class, own loop).
- Not a rewrite of the index storage into a type-indexed layout; the on-disk format stays.
- No general index-cost accounting framework; the counter is a test diagnostic, not an API.

## Decisions

### D1: An entry lookup table keyed by the global type index, built at `Open()`

`Open()` already iterates the entries as it reads them. Store, for each entry position `i`, the entry's
type index in a dense table:

```
std::vector<uint32_t> entryOfType;   // sized typeConfig->GetTypeCount(), kNoEntry = no entry
...
entryOfType[data.type->GetIndex()] = i;      // per entry, while reading
```

`GetOffsets` then iterates the requested `TypeInfoSet` and, per requested type, consults
`entryOfType[type->GetIndex()]`.

**Alternatives**

1. *Keep the loop over `typeData` (status quo).* Rejected: it is the defect; every request pays for
   every entry.
2. *Key the table by the derived class's type id (area/way/route) instead of the global index.*
   Rejected: the id space differs per derived class (`AreaWayIndex` vs `AreaRouteIndex`), so the base
   class cannot name it without a new virtual; the global type index needs no per-class hook.
3. *`std::unordered_map` from type index to entry position.* Rejected: ~48 B per entry (~20-50 KB per
   open index) against 4 B per defined type (6 KB at 1527 types), and a hash per requested type.
4. *Sort the entries by type index at `Open()` and binary search.* Rejected: it assumes the file order
   equals ascending type index, which no writer guarantees, and it costs `O(log n)` per requested type
   instead of `O(1)`.
5. *Build the table lazily on the first lookup.* Rejected: the public lookup does not hold `lookupMutex`
   (only the per-entry read does, `AreaIndex.cpp:151`), so a lazily built table would be a data race
   under the concurrent lookups this class supports, and `Open()` already has both the entries and the
   type config in hand.

### D2: The examined-entry count is a per-instance test diagnostic

The spec's cost scenarios need "the entries the lookup examined" to be observable. Add a private
counter, incremented once per examined entry, with a getter — the pattern the map backends already use
(`MapPainterCairo.h:125` `resolvedFontCount{0}` "diagnostic for tests", incremented in
`MapPainterCairo.cpp:327`).

**Alternatives**

1. *No counter; build a fixture whose unnamed entries are unreadable (truncated/poisoned) and assert
   the lookup still succeeds.* Rejected as the sole mechanism: it proves only one direction and ties the
   test to an error path it does not exercise otherwise.
2. *Compare wall-clock time of a narrow request against indexes of different entry counts.* Rejected:
   flaky, and the project's own import analysis records a ±20 % run-to-run noise floor.
3. *Reconstruct the examined set in the test from the index file.* Rejected: it re-implements the
   reader in the test and still cannot see which entries the lookup touched.

A **file-static or thread-local** counter was also considered to avoid the class-layout change; it was
rejected because `AreaIndex` instances are independent and the existing precedent is a per-instance
member. The counter is a `std::atomic<size_t>` stored with `memory_order_relaxed`: the public lookup
does not hold `lookupMutex` (only the per-entry read does), so concurrent lookups, which
`Tests/src/ThreadedDatabaseTest.cpp` exercises, would otherwise race on it. Reading it therefore
reports the count of whichever lookup finished last.

### D3: The reported loaded types keep their current meaning

The new loop sets `loadedTypes` under the same condition as today: a requested type is reported as
loaded when `typeData` carries an entry for it, whether or not that entry has data
(`bitmapOffset==0`) and whatever the entry's type space. The `IsInternal()` skip that
`AreaNodeIndex` applies is deliberately **not** copied: it would change the reported set for requests
that name internal types.

**Alternatives**

1. *Adopt the node index's `IsInternal()` skip for consistency.* Rejected: it changes the result
   (spec: "reports the request's types as loaded"), and the consistency argument belongs to the node
   index, not here.
2. *Report as loaded only the types that contributed an offset.* Rejected: it is a behaviour change
   and would make a caller treat "entry present, empty box" as "no data in this index".

### D4: The mapping is validated, not assumed

`Open()` builds the table from the entries it just read, so no assertion about file order is needed.
Two cheap invariants are worth a debug assertion: the entry's type index is below the type count, and
two entries never claim the same type index. Both are invariant violations the current loop would hide.

**Alternatives**

1. *Assert nothing.* Rejected: a duplicate or out-of-range index would silently make one entry
   unreachable only for the requests that name that type — the worst kind of latent difference.
2. *Assert the table is fully populated for every type.* Rejected: entries exist only for the types
   the importer indexed; an absent entry is the normal case.

## Sequence

```
before (today)                      proposed
----------------------------        --------------------------------------
GetOffsets(box, types)              GetOffsets(box, types)
  for entry in typeData:  <-- N       for type in types:   <-- |request|
    if types.IsSet(entry.type)          e = entryOfType[type->GetIndex()]
      read entry                        if e != kNoEntry:
      loadedTypes.Set(entry.type)         read entry e
                                          loadedTypes.Set(type)
  append offsets to result            append offsets to result
```

`N` is the number of entries the index file carries (444 area-way types on the current type set, 1097
on the parked one); `|request|` is the number of types the caller asks for, typically a handful.

## Risks / Trade-offs

| Risk | Mitigation |
|---|---|
| The result changes subtly, so callers see different areas loaded (the change's main risk) | Keep the loaded-types rule and the append semantics as they are (D3); assert result equality against the current implementation for a set of requests in the test |
| `offsets` order changes for the same request: the new loop inserts into the `unordered_set` in a different order, and bucket iteration can reorder | The order is already bucket order, not file or sorted order, so it was never part of the contract; noted in `proposal.md`. No test asserts an order today |
| The dense table costs `4 B × GetTypeCount()` per open index instance (~6 KB at 1527 types, per `areaway.idx` and `arearoute.idx`) | Accepted: it replaces a per-request scan of up to 1097 entries; the table is built once per `Open()` and freed with the index. A `uint32_t` entry position is enough for any index |
| Adding a member and a getter changes `AreaIndex`'s layout for binary consumers | The member is private and the getter additive; the alternative (a file-static counter) was rejected in D2. Record it in the change's notes for the release that ships it |
| A wrong `GetIndex()`-keyed table would go unnoticed if the entries' types are not comparable across type spaces | The table is keyed by the global type index, which `TypeInfoSet` itself uses, so the key is the same one the request is expressed in; D4 asserts the invariants |
| The spec's cost scenario could be satisfied by a lookup that examines the right entries but still reads the whole file | The spec only constrains the examined entries; the read is per entry (`GetOffsets(entry, ...)` positions the scanner), so no whole-file pass remains |

## Migration Plan

None. This is a read-side change to a library lookup: index files, database content and the importer
are untouched, and existing databases keep working. Old and new clients read the same index; a client
built with this change shows the behaviour on an index generated before it.

## Open Questions

None that would change the specs, the approach or the tasks. Whether the same diagnostic should be
added to `AreaAreaIndex` (a separate class with its own `IndexCache` keyed by file offset) is a later
question and does not affect this change.
