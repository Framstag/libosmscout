# Design

## Root cause

`TypeInfoSet` (`libosmscout/include/osmscout/TypeInfoSet.h:112`) holds
`std::vector<TypeInfoRef> types` and keeps the vector's size at the number of types the type configuration
defines: `TypeInfoSet(const TypeConfig&)` resizes it to `typeConfig.GetTypeCount()`
(`libosmscout/src/osmscout/TypeInfoSet.cpp:26-29`), `Adapt` does the same, and `Set(const TypeInfoRef&)`
grows it to the type's index plus one. Every set therefore allocates one slot per defined type - 24 432
bytes at 1527 types, because a `TypeInfoRef` is 16 bytes wide - and `Intersection`, `Intersects`,
`operator==` and `operator!=` walk that slot array type by type. Sets are copied per tile load
(`libosmscout-map/src/osmscoutmap/MapService.cpp:499-501`, `:596-598`, `:668-670`, `:766-768`) and retained
per cached tile (`libosmscout-map/include/osmscoutmap/DataTileCache.h:57`, returned by value at `:212`), so
the slot array is paid for many times over, although a set usually contains a handful of types.

## The single fix

Replace the slot per defined type with a bit per defined type and keep a slot only for the types the set
contains:

```text
                    HEAD                             this change
  +--------------------------------+   +-----------------------------------+
  | vector<TypeInfoRef> size = N   |   | vector<uint64_t> bitmap  (N/64)   |
  |   [0] = ref or null            |   |   1 bit per defined type index    |
  |   ...                          |   +-----------------------------------+
  |   [N-1] = ref or null          |   | vector<TypeInfoRef> slots (count) |
  +--------------------------------+   |   the contained types, ascending  |
  | size_t count                   |   +-----------------------------------+
  +--------------------------------+   | size_t count                      |
                                       +-----------------------------------+
```

- `Set(const TypeInfoRef&)` sets the type's bit and appends the type to the slot list at its index order
  position; `Remove` clears the bit and drops the slot; `Add` sets the types of the other set one by one.
- `Intersection` keeps the slots whose bit the other set has; `Intersects` and the comparison compare whole
  64 bit words; `IsSet` tests one bit.
- `Clear` empties both containers; `Adapt` and `Set(const TypeInfoSet&)`/the copy and move operations carry
  both containers. `Adapt` to a smaller type count drops the bits and slots above the new count, so that
  `count` keeps meaning "number of contained types" - a state HEAD could reach (`Adapt` shrank the slot array
  while leaving `count` as it was) but no caller produced, since a type configuration only grows.
- The public interface is unchanged: the constructors, `Set`, `Add`, `Remove`, `Intersection`, `Intersects`,
  `IsSet`, `Empty`, `Size`, `Clear`, `Adapt`, `operator==`, `operator!=`, `begin` and `end` keep their
  signatures and their results, and `TypeInfoSetConstIterator` keeps iterating a
  `std::vector<TypeInfoRef>`.

## Alternatives the evidence eliminated

**Keep the slot per defined type (HEAD).** Eliminated by the measurement: an empty set of a 1527-type
configuration allocates 24 432 bytes where a bit per type needs 192 (`evidence/red-test.log`), and the set
operations walk 1527 slots to answer a question about three types.

**Bit per defined type plus a pointer to the `TypeConfig` for iteration.** Instead of the slot list, the
iterator would map a set bit back to its type through the type configuration. Eliminated: a set can be
constructed without a type configuration (the default constructor, `TypeInfoSet(const std::vector<TypeInfoRef>&)`),
which is why `Adapt(const TypeConfig&)` exists, and a set outlives the configuration it may hold a pointer
to - a `TypeInfoSet` is retained in a tile cache whose owner is a `Database` that a caller may destroy
first. The slot list carries the types themselves and adds no lifetime coupling.

**A hash container of the contained type indices.** Eliminated: iteration of an `unordered_set` yields an
order derived from the hash and the bucket count, while the iterator of a set yields the contained types in
ascending type index order today; restoring that order needs a sort per iteration or an ordered container
anyway, and per-element overhead exceeds the one bit per defined type it would save.

**The slot list alone, without the bitmap.** Eliminated: `IsSet` would become a search over the contained
types and `Intersects` would compare one slot list against the other, so the membership and intersection
cost would grow with the contained types where today it is a bounds check and a word compare; the bitmap
keeps a type's membership a constant time bit test.

## Blast radius

- `libosmscout/include/osmscout/TypeInfoSet.h`, `libosmscout/src/osmscout/TypeInfoSet.cpp` - the class layout
  and every algorithm of the set.
- `Tests/src/TypeInfoSetStorageTest.cpp` with its registrations in `Tests/CMakeLists.txt` and
  `Tests/meson.build`, and the comment of `Tests/src/StyleConfigLookupCostTest.cpp:413`-`:415` that named the
  storage of a set.
- No other source file changes. The class layout changes, so a shared library build has a changed ABI for
  this class; `OSMSCOUT_LIBRARY_VERSION` versions the library as a whole and a caller rebuilds against it,
  and no database file, type configuration or serialized layout changes
  (`guidelines/FileFormatVersion.md`: content-versus-layout, no layout touched, no version bump).

## Rollback

Revert the two library files. No data, no stored file and no other source file depends on the
representation, so the tree returns to HEAD and the storage cases of `Tests/src/TypeInfoSetStorageTest.cpp`
fail again.

## Risks

| Risk | Handling |
|---|---|
| Iteration order drifts from ascending type index | the slot list is kept in index order; case "Iteration of a set yields the contained types in type index order" asserts it against the order of the types the set was built from |
| `count` drifts from the number of contained types, or an operation misses a slot | the model case compares the contained types, `Size()`, `IsSet`, `Intersects`, `Intersection` and comparison against a `std::set<size_t>` model after every step, for both storage-backed sequences |
| a set that contains nearly every defined type pays the bitmap on top of the slots | bounded: +N/8 bytes and one allocation, about +0.8 % at 1527 contained types (192 B of bitmap on the 24 432 B of slots), and it buys a 127x saving for a set of a few types |
| insertion into the slot list is not constant time | a caller that sets indices in descending order pays one shift per contained type, not per defined type |
| the storage cases say nothing in a sanitizer build | the allocation counter is disabled there, as in `Tests/src/MapPainterAreaPreparationTest.cpp`; the cases report the disabled counter instead of asserting |
