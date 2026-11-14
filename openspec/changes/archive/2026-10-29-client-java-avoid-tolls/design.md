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
- The grade byte holds values 1..5 in its low bits. Five high bits are unused.
- `AccessFeatureValue`'s `uint8` is fully consumed (foot/bicycle/car forward+backward, oneway
  forward+backward — 8 of 8), so the toll bit cannot ride there.

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
mask. The list is short and enumerated in the tasks; a reader that forgets is an out-of-range index into a
five-element array, which the tasks call out as the thing to check.

## Decision 2 — how the importer learns about tolls

**Chosen: a new way feature beside `GradeFeature`, declared per type in the type files.**

Consistent with how `surface`/`tracktype` already reach the routing index as `grade`, and the type
configuration carries the feature set of a database, so an older database simply has no such feature and
its objects read as "no toll information".

`FEATURE Toll` is declared in `stylesheets/map.ost`, and `Toll` is added to the feature list of every
car-routable way type in `stylesheets/map.ost` and `stylesheets/motorways.ost` (24 types). The generator
(`GenRouteDat`) reads the feature from the way it is already inspecting when it registers an object
variant, so the index gains the bit without any extra pass over the data.

**Rejected: registering the feature in code for every path-like type**, the way the code adds `Grade`.
Measured, and it is the trap of this change: a feature injected in code exists on the reader's side for a
database whose type configuration never wrote it, so the way record is parsed at the wrong offsets — the
probe died with `Coordinate is not normalised` while reading `ways.dat` of a database imported before the
change, and with a vector assertion before that. `Grade` survives that asymmetry only because every
database that exists was imported after it. A new feature must therefore arrive through the type file,
where the database that has it says so.

**Rejected: a cost expression in the bridge.** The bridge has no access to a way during the search; it
would have to load ways itself, which is a second routing implementation.

## Decision 3 — the price of a toll road

**Chosen: the graded speed table is applied as usual and the time is multiplied by a factor when
`avoidTolls` is set and the way charges.**

A factor expresses "avoid, but take it when the detour is bad" without making the road unroutable, which
matters for the same reason as in `client-java-avoid-unpaved`: a profile whose slowest edge goes to zero
prices its own cost limit out of reach and the search reports "no route" instead of a detour. The exact
factor is fixed during the change with the probe that ships beside it, on data that actually charges.

## Consequence to state, not to hide

The default map provider is `osmscout.karry.cz` and the app's maps are prebuilt downloads. A database built
by an older importer carries no toll bit, and the flag is then inert — by design and provably so, which the
spec scenario asserts. The change therefore delivers correctness and an upstreamable import path; the
user-visible effect arrives with a provider rebuild.

## Verification

`evidence/TollProbe.cc` plus a hand-written two-way map (`evidence/toll-test-map.osm`: a 3.5 km motorway
that charges and a 7.6 km free detour), imported with the changed importer:

- the index records two object variants, one carrying the toll bit, and both keeping grade 1 in the low
  bits;
- A to B without the flag: 3 584.7 m, 2 points — the toll way;
- A to B with `avoidTolls`: 7 575.1 m, 3 points — the free detour; no route lost in 36 grid pairs.

The same probe on a database imported *before* the change (the Bremen extract): 179 object variants, **0
carrying a toll**, 30 routes, none changed, none lost — the absence of the signal is not a signal.

And through the bridge (`evidence/AvoidTollsBridgeProbe.java`): `avoidTolls=false` 2 points, `avoidTolls=true`
3 points, with the log line `avoiding toll roads (cost x3.0 …)` beside them.
