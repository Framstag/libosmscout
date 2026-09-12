# File format version (FILE_FORMAT_VERSION)

Guideline for when a new type config / database file format version is required.

## The core model

```text
FILE_FORMAT_VERSION = 27        (libosmscout/include/osmscout/TypeConfig.h, TypeConfig::FILE_FORMAT_VERSION)
```

A single number serves three purposes:

1. **Binary layout version of `types.dat`** — the only database file that carries
   a version header. Written by `TypeConfig::StoreToDataFile()`, checked by
   `TypeConfig::LoadFromDataFile()`.
2. **`typeConfigVersion` entry in `db.json`** — the map repository slot key
   (`Documentation/MapRepository.md`).
3. **Proxy for the whole database** — `nodes.dat`, `ways.dat`, `areas.dat`,
   `routes.dat` and the index files carry **no version header of their own**.

Reader enforcement:

```cpp
static const uint32_t MIN_FORMAT_VERSION = FILE_FORMAT_VERSION;
static const uint32_t MAX_FORMAT_VERSION = FILE_FORMAT_VERSION;
```

(see `TypeConfig.h`). MIN == MAX means exact match or refuse. `Database::Open()`
reads the first `uint32` of `types.dat` and fails hard on mismatch:

> "File '...' does not have the expected format version! Actual X, expected: 27"

(`TypeConfig.cpp`, `LoadFromDataFile()`). The query API
`TypeConfig::GetDatabaseFileFormatVersion(directory)` exists so clients can
probe a database before opening it (added in commit `8c55d7bfe`, #1606).

## What `types.dat` actually stores

`TypeConfig::StoreToDataFile()` writes, self-describing (count-prefixed):

```text
file format version (uint32)
feature count
  per feature: name, descriptions (UI metadata; used by StyleEditor/dump tools)
type count
  per type:
    name
    capabilities: CanBeNode, CanBeWay, CanBeArea, CanBeRelation, IsPath
    route flags: CanRouteFoot, CanRouteBicycle, CanRouteCar
    index flags: IndexAsAddress, IndexAsLocation, IndexAsRegion, IndexAsPOI
    OptimizeLowZoom, SpecialType (uint8), PinWay, MergeAreas
    IgnoreSeaLand, Ignore
    Lane counts: Lanes, OnewayLanes
    feature names []
    groups []
    descriptions []
```

Important: **tag conditions are NOT serialized.** The `.ost` tag → type matching
exists only at import time. At runtime the library never re-matches tags; every
object in the data files carries its resolved type id.

## The content-versus-layout distinction

All decisions about bumping the version reduce to one question:

> Does the change alter the **binary layout** of `types.dat` or of any data/index
> file, or does it only change **content** (which types exist, what they mean)?

Content changes keep the version; layout changes require a bump.

### Why content changes break existing data (the id ordinal trap)

Type ids are **ordinal positions**, assigned during `.ost` parsing
(`TypeConfig.cpp`, `TypeConfig::AddTypeInfo()`):

```cpp
typeInfo->SetIndex(types.size());             // global position in .ost order
typeInfo->SetNodeId(nodeTypes.size() + 1);    // 1-based position per kind
...
nodeTypeIdBytes = BytesNeededToEncodeNumber(typeInfo->GetNodeId());
```

Data files store object type ids with widths derived from those counts:

```cpp
scanner.ReadTypeId(typeConfig.GetNodeTypeIdBytes());   // Node.cpp, Way.cpp,
                                                       // Area.cpp, Route.cpp
```

Feature values are stored per object via `FeatureValueBuffer`, whose parse
layout is derived from the per-type feature list in `types.dat` plus each
feature class's `Read()`/`Write()` (`TypeConfig.h`, `FeatureValueBuffer::Read`).

There is **no CRC or checksum cross-check** between `types.dat` content and the
data files. A mismatched pair (data imported with .ost set A, `types.dat` from
set B) silently misparses — wrong type ids, wrong feature value boundaries.
Coherence is guaranteed only by regenerating everything in one import run.

## Changes that KEEP the version (content-only) — but require full re-import

| Change                                          | Reason                                                            |
|-------------------------------------------------|-------------------------------------------------------------------|
| Add a new type at any position                  | Ids of following types shift (ordinal). Reimport regenerates all  |
| Remove / reorder types                          | Same — ids shift                                                   |
| Edit tag conditions                            | Conditions not in `types.dat`; import-time classification changes |
| Change capability flags (NODE/WAY/AREA/RELATION, PATH, INDEX_AS_*, OPTIMIZE_LOW_ZOOM, PIN_WAY, MERGE_AREAS, IGNORESEALAND, IGNORE) | Id lists per kind shift                                            |
| Add/remove existing features on a type (`{Name, MaxSpeed, ...}`) | `FeatureValueBuffer` layout per type changes                        |
| Change groups / descriptions                   | Self-describing, content-only                                      |
| Rename a type (keep position)                  | Ids stable; styles reference names, so `.oss` must follow          |
| Change lane counts / other type option values  | Content values, layout unchanged                                    |

Rule of thumb: any `map.ost` / `basemap.ost` edit falls into this category.
The `FILE_FORMAT_VERSION` constant is **never** touched for `.ost` changes.

## Changes that REQUIRE a version bump (layout changes)

1. **`types.dat` record layout change** — add/remove/reorder any field in
   `TypeConfig::StoreToDataFile()` / `LoadFromDataFile()`, change an encoding
   (`Write` vs `WriteNumber`), add a new block (e.g. tag registry in file).
   Precedent: `8463c1e22` "Increase file format version (type.subType)".
2. **Object record layout change** — `Node`/`Way`/`Area`/`Route` `Read`/`Write`
   implementations (rings, attributes, coordinate encodings). These files have no
   own version; the `types.dat` version is their only guard.
3. **Index layout change** — water.idx, area/way/node.idx, location index,
   region index, routing data. Precedent: `7fdfdd1d1` water.idx compression,
   `22895ec5d` new area store alternative (multiple outers).
4. **New feature class in the feature registry** — feature value serialization
   lives in the library's feature classes. An older build cannot parse values of
   a feature it does not know, even though `types.dat` names features
   self-descriptively. Precedents: `f2c4c6953` "Added Fee and MaxStay feature"
   (→ 26), `34d3c3039` bump to 27 because HighwayMilestone "newly generated
   databases will not be usable with older library builds".
5. **Serialization format change inside an existing feature class** — e.g. value
   field order/encoding changes in `NameFeature`, `MaxSpeedFeature`, ...
6. **Encoding width change** of a stored field (e.g. `Lanes` from `uint8_t` to
   wider).

Note: type id byte width changing (kind count crossing 255/65535/… boundaries)
does **not** itself require a bump — the widths are derived from content, and a
full re-import regenerates matching data. Current counts (map.ost): ~335 node,
~451 area, ~72 way types, all already in the 2-byte range with headroom to
65535. Only layout changes to the serialization code bump the version.

## Why the bump exists — protection model

MIN == MAX == current version means:

- an older library **refuses** to open newer databases instead of misparsing;
- a newer library refuses older databases.

There is deliberately no multi-version reader support right now. The bump is
the compatibility contract: bump ⇔ "data from the new build is not readable by
older builds". Content changes never violate that contract, which is why they
never bump.

## Map repository implications

```text
/repository/public/europe/germany/berlin/
  v27/     ← one slot per typeConfigVersion
  v26/     ← older slots kept per retention policy
```

- `typeConfigVersion` in `db.json` **equals** `FILE_FORMAT_VERSION`
  (`Documentation/MapRepository.md`).
- Clients request `/v<theirVersion>/db.json`. Old clients keep working because
  old slots are retained.
- Content change → reimport → same slot **replaced atomically** (staging dir +
  rename); clients re-download when their local `generatedAt` is older.
- Layout change → library bump → new slot `v28/`; old builds keep fetching v27.

So `typeConfigVersion` detects **layout breakage only**; content drift is
handled by `generatedAt` freshness, not by the version number. A same-version
mismatched pair (old data + new-original `types.dat`) cannot be detected at
open time — only by regenerating everything together.

## Practical workflow for `.ost`/`.oss` changes

1. Edit `stylesheets/map.ost` (types) and `map.oss` (styles).
2. Re-import the whole database: `Import --typefile stylesheets/map.ost ...`
   Everything is regenerated in one run; `db.json` is written only on success
   and acts as the completeness marker.
3. Version stays 27; `typeConfigVersion` unchanged; slot content refreshed.
4. Add new features (new `FeatureDefinition` class) → bump the version.

## Pitfalls

- **Do not rename/delete these types** — import and routing code look them up
  by name via `GetTypeInfo("...")`:
  `boundary_administrative`, `boundary_country`, `boundary_county`,
  `boundary_state`, `highway_mini_roundabout`, `highway_motorway_junction`.
- New types are invisible until `map.oss` has style rules for them; styles load
  type names from `types.dat` at runtime.
- `.oss` (style) changes alone never require re-import — styles are read at
  runtime. Only `.ost` changes make stored data stale.
- When touching `TypeConfig::StoreToDataFile()`, update `LoadFromDataFile()` in
  the same change and bump `FILE_FORMAT_VERSION` — both directions must stay in
  sync.

## Decision flow

```text
change to types / type parameters / features
      |
      +--> touches C++ serialization?            --> bump FILE_FORMAT_VERSION
      |     (StoreToDataFile / LoadFromDataFile,
      |      Node/Way/Area/Route Read+Write,
      |      index layouts,
      |      NEW feature class in registry,
      |      feature value encoding change)
      |
      +--> content only                           --> re-import database
            (new/removed/reordered types,           version unchanged
             tag conditions, capability flags,
             feature assignments of existing
             features, groups, descriptions,
             lane counts, renames)
      |
      +--> style only (.oss)                      --> nothing
                                                  (runtime-loaded)
```

## Change history (empirical evidence)

| Commit | Change |
|--------|--------|
| `34d3c3039` | bump to 27 — HighwayMilestone feature added |
| `f2c4c6953` | bump to 26 — Fee and MaxStay feature added |
| `8463c1e22` | bump — new per-type field (`type.subType`) |
| `7fdfdd1d1` | bump — water.idx compression changed |
| `22895ec5d` | bump — new area store alternative (multiple outers) |
| `8c55d7bfe` | **no** bump — added version query API (#1606) |
