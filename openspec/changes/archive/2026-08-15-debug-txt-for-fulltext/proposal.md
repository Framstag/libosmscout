# Proposal: debug-txt-for-fulltext

## What Changes

The marisa full-text index generator (`TextIndexGenerator` in `libosmscout-import/src/osmscoutimport/GenTextIndex.cpp`) currently writes only the binary trie files (`textpoi.dat`, `textloc.dat`, `textregion.dat`, `textother.dat`). Unlike the location index generator (`GenLocationIndex`), it produces no human-readable debug output.

This change adds four debug `.txt` files, one per index file, written to the destination directory during import:

- `textpoi.txt`
- `textloc.txt`
- `textregion.txt`
- `textother.txt`

Each line lists one indexed entry with four fields:

```
<name> <object type> <type name> <id>
```

- `name` — the indexed text (name, name alt, or ref)
- `object type` — `Node`, `Way`, or `Area`
- `type name` — the OSM type name (e.g. `highway_residential`, `amenity_cafe`)
- `id` — the file offset of the object in its data file

The files are registered as provided analysis files in `GetDescription()`, matching how `location_region.txt` / `location_full.txt` are handled by `GenLocationIndex`. Debug output is written unconditionally, consistent with the location index behavior.

## Capabilities

### New Capabilities

None.

### Modified Capabilities

None. This is a tooling/debug-output change only; no index format or lookup behavior changes.

## Impact

- `libosmscout-import/src/osmscoutimport/GenTextIndex.cpp` — open four debug streams in `Import()`, select the stream alongside the keyset in `AddNodeTextToKeysets` / `AddWayTextToKeysets` / `AddAreaTextToKeysets`, write one line per entry in `AddKeyStr`
- `libosmscout-import/include/osmscoutimport/GenTextIndex.h` — new filename constants, four `std::ofstream` members, extended `AddKeyStr` signature
- Import output directory gains four new `.txt` files
- No changes to `libosmscout` core, index readers, or search behavior
