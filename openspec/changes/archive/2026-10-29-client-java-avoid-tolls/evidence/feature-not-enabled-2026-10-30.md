# The toll feature is present but not enabled

**State:** `TollFeature` is compiled into the library and covered by the evidence below, but nothing enables
it — `TypeConfig::Setup()` does not register it, `stylesheets/map.ost` carries the `FEATURE Toll` block
commented out, and none of the 24 car-routable way types lists `Toll`. A type configuration therefore cannot
name it, no import can record a toll, and `avoidTolls` is inert on every database — old or newly imported.

**Why:** registering a new feature class is `guidelines/FileFormatVersion.md` rule 4 — a
`FILE_FORMAT_VERSION` bump (27 → 28), with the precedents `f2c4c6953` (Fee/MaxStay → 26) and `34d3c3039`
(HighwayMilestone → 27). `MIN_FORMAT_VERSION == MAX_FORMAT_VERSION` means exact match or refuse, so the bump
makes the app reject every database that exists today — including everything the map provider serves, which
keys its downloads by `typeConfigVersion`. The bump is therefore a coordinated release with the provider, not
a code edit, and this change was not the place for it.

## Measured, in this state

| what | value |
|---|---|
| re-imported extract, `db.json` | `typeConfigVersion: 27` |
| `types.dat` / `ways.dat` / `router2.dat` | 81 350 / 2 870 207 / 541 bytes — identical to the pre-feature import |
| object variants recording a toll (two-way test map) | 0 |
| `avoidTolls` off vs on, 36 grid pairs | 0 differing, 0 lost |

The enabled-state measurements (1 toll variant of 2; 3 584.7 m / 2 points → 7 575.1 m / 3 points; bridge
2 → 3 points) are in `toll-probe-2026-10-29.log` and `avoid-tolls-bridge-probe-2026-10-29.log` beside this
file. They hold for an importer whose type file declares `Toll`; `evidence/toll-test-map.osm` plus a
`--typefile` that uncomments the block reproduces them.

## What enabling costs

1. Uncomment the registration in `TypeConfig::Setup()`, the `FEATURE Toll` block in `map.ost` and `Toll` on
   the 24 car-routable types in `map.ost`/`motorways.ost`.
2. Bump `FILE_FORMAT_VERSION` to 28.
3. Re-import, and wait for the provider to publish its v28 slot; until then the app has no maps.

## Guard that makes the disabled state safe

`FeatureValueReader` asserts on a feature the type configuration does not know, so `GenRouteDat` builds its
`TollFeatureValueReader` only when the type config declares `Toll`, and `TollBit()` returns `0` when that
reader is absent.
