# Design

## Root cause

`TypeSetBytes` (`libosmscout-map/src/osmscoutmap/StyleConfig.cpp:527`-`:531`) estimates the bytes a family's
per-level sets retain as `typeSets.size()*(sizeof(TypeInfoSet)+typeConfig.GetTypeCount()*sizeof(TypeInfoRef))`.
That is the storage a set had before `fix-typeinfoset-bitmap-storage`: one 16 byte slot per defined type. A set
now holds a bitmap of one bit per defined type, rounded up to 64 bit words, plus one slot per contained type
(`libosmscout/include/osmscout/TypeInfoSet.h`), so the formula overstates every set by the ratio of the
defined to the contained types - for the scaling case of the test, 3024 bytes against 14224 bytes for
configurations whose sets hold the same types.

## The single fix

Ask the sets:

```text
  typeSetBytes = sum over the per-level sets of  sizeof(TypeInfoSet) + set.AllocatedBytes()

  TypeInfoSet::AllocatedBytes() = words.capacity()*sizeof(uint64_t)      // one bit per defined type
                                + contained.capacity()*sizeof(TypeInfoRef)  // one slot per contained type
```

`AllocatedBytes()` is the smallest accessor that lets the diagnostic read the storage of a set from another
module; `Size()` returns the number of contained types, on which the set's own cases and the resolution path
depend, so changing its meaning is a wider contract change than adding one method. The accessor names the
allocated storage (capacity), which is what the type-set requirement speaks about and what the allocation
counter of `Tests/src/TypeInfoSetStorageTest.cpp` measures, and not the number of elements a set still holds
after a removal.

## Alternatives the evidence eliminated

**Keep the formula of HEAD.** Eliminated by the red case: the same stylesheet reports 3024 and 14224 bytes for
type configurations of 10 and of 60 defined types, although the sets of the two configurations are the same
size (`evidence/red-head-ctest-StyleConfigLookupCost.log`).

**Recompute the bitmap size from the type count inside `TypeSetBytes`**
(`typeSets.size()*(sizeof(TypeInfoSet)+((typeCount+63)/64)*sizeof(uint64_t))` plus a slot count taken from
elsewhere). Eliminated: it repeats the representation a second time - the same defect, one representation
later - and it still cannot see the slots a set holds, because the sets are private to `StyleConfig` and the
only public count is `Size()`.

**Make the test measure the allocations and assert a constant.** Eliminated: the diagnostic is a runtime
report of any loaded configuration, not a test constant; the allocation counter of
`Tests/src/TypeInfoSetStorageTest.cpp` is disabled in sanitizer builds, and a figure that has to be correct
there cannot come from the counter.

**Make `TypeInfoSet::Size()` report bytes.** Eliminated: it is the contained type count, asserted by the model
case of the set and used by the lookup positions of the style configuration; redefining it would change a
contract the bitmap change deliberately kept.

## Blast radius

- `libosmscout/include/osmscout/TypeInfoSet.h` - the accessor.
- `libosmscout-map/src/osmscoutmap/StyleConfig.cpp` - `TypeSetBytes` and its comment.
- `libosmscout-map/include/osmscoutmap/StyleConfig.h` - the comment of the reported figure.
- `Tests/src/StyleConfigLookupCostTest.cpp` and `Tests/src/TypeInfoSetStorageTest.cpp` - the figure assertion
  and the accessor case.
- No other source file changes. No class layout, database file, type configuration or serialized layout
  changes (`guidelines/FileFormatVersion.md`: content versus layout, no layout touched, no version bump).

## Rollback

Revert the two library files and the two test files. No data and no stored file depends on the figure, so the
tree returns to HEAD and the scaling case reports 3024 against 14224 bytes again.

## Risks

| Risk | Handling |
|---|---|
| `capacity()` overstates a set that held more types and removed them | the diagnostic reports the storage a set allocates, which is what the type-set requirement states; the storage cases measure the same quantity |
| the accessor drifts from the real storage when the representation changes again | the accessor sits on the representation itself, and the new case compares it against the bytes the construction allocates, so the pair moves together |
| the reported number changes for the shipped stylesheet and breaks the measurement case | `Tests/src/StyleConfigLookupCostTest.cpp:589`-`:594` asserts that the figure is reported and the two runs are within tolerance, not its value |
| the sanitizer build cannot check the accessor against the counter | the counter is disabled there, as in the other storage cases; the accessor case reports it and the diagnostic cases still run |
