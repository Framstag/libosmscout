# Design — client-java-avoid-tolls

## Context

The routing search prices an edge from `ObjectVariantData{type, maxSpeed, grade}`, a lookup table beside
the routing graph (`RouteNode.h`, `ObjectVariantDataFile`). `AbstractRoutingProfile::GetCosts` and
`CanUse(RouteNode)` read that record and nothing else — the `Way` itself, with all its feature values, is
not available during the search. So a per-way toll signal has exactly one place to live: that record.

Two facts decide the shape:

- `ObjectVariantData` is written as `uint32 typeIndex, uint8 maxSpeed, uint8 grade` with **no version
  header** (`ObjectVariantDataFile::Load` reads a count and then raw records). Adding a field silently
  misparses every existing database.
- The grade byte holds values 1..5 in its low bits. Three high bits are unused.
- `AccessFeatureValue`'s `uint8` is fully consumed (foot/bicycle/car forward+backward, oneway
  forward+backward — 8 of 8), so the toll bit cannot ride there.

Flow, from tag to priced edge:

```
OSM way (toll=yes)
   │  import, GenRouteDat inspects the way it already registers
   ▼
ObjectVariantData.grade |= TollBit (0x80)          ── index, no layout change
   │  stored in the object-variant file
   ▼
routing search reads ObjectVariantData
   │  GetGrade() masks the bit for the speed table
   ▼
FastestPathRoutingProfile::GetCosts
   └── avoidTolls && variant.IsToll()  →  outPrice *= tollPenaltyFactor
```

## Decision 1 — where the toll signal lives

**Chosen: one bit in the spare high bits of `grade` (0x80), with masking at every read.**

The file layout is untouched: a database imported before this change reads as `grade & 0x7F` in 1..5 and
`grade & 0x80 == 0`, i.e. "no toll information", which is true. A database imported after it carries the
bit where the search can see it.

**Rejected: a new `uint8 toll` field.** Correct in isolation, but with no version header in the file it
turns every existing database into a misparse — a silent one, since the record is fixed width and the
loader has already sized its vector from the count.

**Rejected: widening `AccessFeatureValue`.** Same format problem, and it would push a change into the OSM
tag-mapping path for a single bit.

Cost of the choice: the byte is now two fields, so every reader that indexes the speed table by grade must
mask. `GetGrade()` returns the low bits; `RoutingProfile.h` `GetMaxSpeed` and `RoutingProfile.cpp`
`CanUse(RouteNode)` call it. A reader that forgets is an out-of-range index into a five-element array, or a
wrong-vehicle decision.

Risk: a future field added to the byte collides with `TollBit`. Mitigated by the named constant and the
"every reader of grade must mask it" comment at its definition.

## Decision 2 — how the importer learns about tolls

**Chosen: a new way feature beside `GradeFeature`, declared per type in the type files.**

Consistent with how `surface`/`tracktype` already reach the routing index as `grade`, and the type
configuration carries the feature set of a database, so an older database simply has no such feature and
its objects read as "no toll information". `GenRouteDat` reads the feature from the way it is already
inspecting when it registers an object variant, so the index gains the bit without any extra pass.

**Rejected: registering the feature in code for every path-like type**, the way the code adds `Grade`.
Measured, and it is the trap of this change: a feature injected in code exists on the reader's side for a
database whose type configuration never wrote it, so the way record is parsed at the wrong offsets — the
probe died with `Coordinate is not normalised` while reading `ways.dat` of a database imported before the
change. A new feature must arrive through the type file, where the database that has it says so.

## Decision 3 — the price of a toll road

**Chosen: the graded speed table is applied as usual and the time is multiplied by a factor when
`avoidTolls` is set and the way charges.**

A factor expresses "avoid, but take it when the detour is bad" without making the road unroutable, which
matters for the same reason as in `client-java-avoid-unpaved`: a profile whose slowest edge goes to zero
prices its own cost limit out of reach and the search reports "no route" instead of a detour. The factor is
fixed at `3.0`.

**Rejected: cost zero for a toll way.** It makes the way unroutable in practice — the cost limit is
crossed before the search reaches it, and the failure is reported as "no route" rather than a detour.

**Rejected: a cost expression in the bridge.** The bridge has no access to a way during the search; it
would have to load ways itself, which is a second routing implementation.

## Decision 4 — the feature is present but not enabled

**Chosen: ship the class compiled, and enable nothing.**

Registering a new feature class is `guidelines/FileFormatVersion.md` rule 4 — a `FILE_FORMAT_VERSION` bump
(27 → 28). `MIN_FORMAT_VERSION == MAX_FORMAT_VERSION` means exact match, so the bump makes this build
reject every database that exists today, including everything the map provider serves, which keys its
downloads by `typeConfigVersion`. The bump is a coordinated release with the provider, not a code edit. So
`TypeConfig::Setup()` keeps the `featureToll` registration commented out, `stylesheets/map.ost` keeps the
`FEATURE Toll` block commented out and no type lists `Toll`, and no version is bumped.

Because `FeatureValueReader` asserts on a feature the type configuration does not know,
`GenRouteDat::Import` builds its `TollFeatureValueReader` only when `typeConfig->GetFeature(TollFeature::NAME)`
resolves, and `TollBit()` returns `0` while that reader is null. A type configuration that declares `Toll`
(a hand-written `--typefile`) gets a recording importer; a normal build records nothing and produces exactly
the database the previous build produced.

**Rejected: enabling the feature now.** It would make every database built from the shipped type files
unreadable for older builds and force the version bump before the provider can publish a matching slot.

**Rejected: registering the feature in `Setup()` but not listing it on a type.** A registered feature the
type file never names is still a feature of the type configuration, so the database records it and the
incompatibility (and the bump) arrives anyway.

## Consequence to state, not to hide

The default map provider is `osmscout.karry.cz` and the app's maps are prebuilt downloads. A database built
by an older importer carries no toll bit, and the flag is then inert — by design and provably so, which the
spec scenario asserts. The change therefore delivers correctness and an upstreamable import path; the
user-visible effect arrives when the feature is enabled and the provider re-imports.
