# Design

## Root cause

`LocationIndex` retains no deserialized region entry. Every traversal deserializes each region it visits
through the pooled `FileScanner` (`libosmscout/src/osmscout/db/LocationIndex.cpp:183` `LoadAdminRegion`,
called from `VisitRegionEntries` `:299`, the root loop of `VisitAdminRegions`, `VisitLocations`,
`VisitRegionPOIs`, `VisitPostalArea` and `ResolveAdminRegionHierachie`), and `FlushCache()`
(`:876`) only clears the scanner pool. Since each `LocationService` search walks the complete region tree
(`libosmscout/src/osmscout/location/LocationService.cpp:2082`, `:2217`, `:2341`), the number of regions the
index holds is paid on every lookup.

## The single fix

A `std::unordered_map<FileOffset,AdminRegionRef>` on `LocationIndex`, addressed by the region entry's offset
in `location.idx`:

- `LoadAdminRegion(scanner, offset, region)` answers from the map when the offset is present; otherwise it
  seeks, deserializes through the existing `LoadAdminRegion(scanner, AdminRegion&)`, inserts the result and
  hands it out. Every region load site above goes through this overload.
- `FlushCache()` clears the map next to the scanner pool, and resets the region-read diagnostic.
- `Database::FlushCache()` (`libosmscout/src/osmscout/db/Database.cpp:975-977`) already calls
  `LocationIndex::FlushCache()`, and `ScopeCacheCleaner` already calls it on scope exit, so the release needs
  no caller change.

The regions of one traversal are the regions of the next one, because a lookup walks the tree from its
roots; a retained entry is therefore always the entry the next traversal would have read.

```
search -> LocationIndex::VisitAdminRegions
             |  roots: LoadAdminRegion(scanner, offset, ref)
             v
        offset in regionCache ?
          yes -> return ref                       (no file access)
          no  -> scanner.SetPos(offset); LoadAdminRegion(scanner, region); insert; return ref
             |
             v
        VisitRegionEntries -> children: LoadAdminRegion(scanner, childOffset, ref) -> ...
             |
        visitor.Visit(region)   (unchanged)

FlushCache -> regionCache.clear(); regionReadCount = 0; fileScannerPool.Clear()
```

## Alternatives the evidence eliminated

**A capacity-bounded cache with eviction (FIFO or LRU), the shape the area-index lookup uses for a
request.** Rejected: the access pattern of this index is the complete tree, not a request. Three of the
search paths traverse every region of the index per call (`LocationService.cpp:2082`, `:2217`, `:2341`), so
with a capacity *c* below the region count *R* of the index a lookup still reads *R - c* regions, and the
cost the entry names — lookup cost following the region count — stands. Only a retention that survives a
whole traversal changes it. That also settles FIFO against LRU: with each traversal re-touching every
retained entry, both evict the same set, so there is no recency question to weigh.
**An unbounded cache with no release path.** Rejected by the owner's recorded contract, which this fix
follows rather than reverses: the class documents a small footprint and offers `FlushCache()` for it
(`libosmscout/include/osmscout/db/LocationIndex.h:88-89`, "To keep small LocationIndex memory footprint, user
may call FlushCache method when index is not needed anymore"), and `Database::FlushCache()` gives it a
caller. The retained entries are therefore bounded by the region entries of the index — never more than the
file holds — and released on demand.

## Policy

- **Size**: one entry per region entry of `location.idx` the index deserialized since the last flush; the
  bound is the index file itself (12 regions in the test fixture, the region tree of the index in a map).
- **Eviction**: none for a live index, deliberately — see above. The entry is dropped only by a flush.
- **Invalidation**: `LocationIndex::FlushCache()` clears the map (and resets the diagnostic), so
  `Database::FlushCache()` and `ScopeCacheCleaner` release the retained regions exactly where they release
  the scanners today. The index is read-only once loaded (`Load` reads only the header), so no other
  invalidation trigger exists.

## Concurrency

`Database::GetLocationIndex()` hands the same index to every caller and a lookup borrows its scanner
exclusively from the pool, so the map and the counter are shared state: both are guarded by
`regionCacheMutex`, taken around the lookup and around the insert, never across the file read. That guard is
defensive — it keeps a concurrent caller from seeing a half-inserted entry or a torn counter — and the delta
claims no concurrent-use contract: no scenario and no test asserts that the index is looked up from several
threads. Two lookups that miss the same offset may both read it; the last insert wins and both entries
describe the same region.

The member comments of the frozen revision (`libosmscout/include/osmscout/db/LocationIndex.h:149-152`, the cache
members, and `:294-299`, `GetRegionReadCount()`) still read as concurrency claims. They are not narrowed here
because that revision is pinned by the gate that measured it (`sha256 c8ad1ebd…`) and a change to it would
measure something else; the contract this change carries is the defensive-guard wording above.

## Blast radius

- `libosmscout/include/osmscout/db/LocationIndex.h`: the map, the mutex, the diagnostic and its getter, and
  the cache-aware load declaration. The class gains members, so it is not binary compatible with a previously
  built one — an ordinary rebuild, no serialized layout is touched.
- `libosmscout/src/osmscout/db/LocationIndex.cpp`: the load sites listed above, all of them a change of the
  local temporary from an `AdminRegion` value to an `AdminRegionRef`.
- No other production file changes. `Database::FlushCache()` and `GetLocationIndex()` stay as they are.
- A region handed to a visitor or through `ResolveAdminRegionHierachie` is now the retained object, shared
  with later traversals instead of rebuilt per call. Every in-tree caller reads regions only; the class
  hands out `AdminRegionRef` today already, so sharing is what the returned references express.
  `ResolveAdminRegionHierachie` consequently stops copying each ancestor into a fresh object.
- The `FILE_FORMAT_VERSION` stays 27: nothing on disk changes
  (`guidelines/FileFormatVersion.md`, "The content-versus-layout distinction"). The type config is not
  touched, and the new `ADMIN_REGION` types need no special handling — they grow the region count, which is
  exactly what the retained entries absorb.

## Risk

- **Retained memory** grows with the region count of the visited tree, where the fix is worth most. The
  mitigation is the release path the class already documents and `Database::FlushCache()` exercises; a client
  that never flushes holds the regions it visited, which is the region section of `location.idx`.
- **A stale entry after the file changes on disk**: the index is loaded once per database open
  (`Database.cpp:582-590`) and nothing rewrites `location.idx` under a live index.
- **A caller that mutates the region it is given** would now write into the retained object instead of a
  per-lookup copy: `ResolveAdminRegionHierachie` returns a retained, non-const `AdminRegionRef`
  (`libosmscout-client-qt/src/osmscoutclientqt/LookupModule.cpp:471` and the `Demos/*` consumers), and
  `AdminVisitor::Visit(const AdminRegion&)` blocks mutation only through the visitor. No in-tree caller
  writes. The out-of-tree case is filed as `TODO.md` §105 rather than asserted away here.

## Rollback

Revert the two `LocationIndex` hunks (the cache-aware load, the map/flush) and the `GetRegionReadCount`
diagnostic; the test file, the two build lists and the `TODO.md` line go with them. No format, no data file
and no caller depends on the change.
