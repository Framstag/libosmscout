# Verification — client-java-avoid-tolls

Every command below was run in this checkout (`client-java-avoid-tolls`, branched from `origin/master`),
against a fresh build of the touched targets. Output is quoted as it appeared.

## Ported against the reference net state

Each ported file was compared to the reference branch, per file. Every file except the JNI bridge is
byte-identical to `origin/naviveylin-local`; the bridge carries only the reference's `if (avoidTolls)` hunk
(the branch's file also holds other, unrelated units' changes).

```
$ git diff origin/naviveylin-local -- <the 15 wholly-ported files>
(no output)

$ git hash-object libosmscout/include/osmscout/feature/TollFeature.h \
                libosmscout/src/osmscout/feature/TollFeature.cpp
42a2c4688e29f5ba90acdf41a3eeaff61589077a
d9d4aa386935bb029c921a31204b7519952b534c
# identical to `git rev-parse origin/naviveylin-local:<the same two paths>`
```

`SetAvoidTolls`, `TollFeature`, `TollBit` and `GetTollPenaltyFactor` did not exist on `origin/master`
before the port (a tree-wide search returned no match).

## Build

Shared `build/` was rebuilt for each target immediately before use.

```
$ cmake --build build --target OSMScout
[21/23] Linking CXX shared library libosmscout/libosmscout.so.1.1.1
# no error, no new warning from the changed code

$ cmake --build build --target OSMScoutImport
[8/9] Linking CXX shared library libosmscout-import/libosmscout_import.so.1.1.1
# only the pre-existing -Wdeprecated-enum-enum-conversion warning in GenTextIndex.cpp (marisa), untouched

$ cmake --build build --target osmscout_client_java
[16/16] Linking CXX shared library libosmscout-client-java/libosmscout_client_java.so
# no error
```

Target names had to be discovered: the core library target is `OSMScout`, the import library is
`OSMScoutImport`, and the JNI bridge is `osmscout_client_java`.

## Tests

The routing-related test executables were rebuilt, then run.

```
$ ctest --test-dir build -R "RoutingProfileSpeedTest|MultiDBRoutingTest|RoutePostprocessorTest|NavigationAgentTest" --output-on-failure
1/4 Test #34: MultiDBRoutingTest ...............   Passed
2/4 Test #35: NavigationAgentTest ..............   Passed
3/4 Test #62: RoutePostprocessorTest ...........   Passed
4/4 Test #76: RoutingProfileSpeedTest ..........   Passed
100% tests passed out of 4
```

No existing test was weakened or skipped.

## JNI signatures

```
$ bash scripts/check-jni-signatures.sh
JNI signatures match: 60 native declarations checked, 5 dead JNI functions reported above.
```

The 5 warnings are pre-existing (functions declared native in `OSMScoutClient.java` with no C++ double);
this change adds no JNI native method and does not touch the signature set.

## The feature is present but not enabled — evidence

- `libosmscout/src/osmscout/TypeConfig.cpp` keeps the `featureToll` registration commented out, with the
  reason beside it.
- `stylesheets/map.ost` keeps the `FEATURE Toll` block commented out, and a search for the `Toll` token in
  `*.ost` returns only comment lines — no type lists `Toll`.
- `stylesheets/motorways.ost` carries the explanatory comment block and lists `Toll` on no type.
- `FILE_FORMAT_VERSION` is unchanged: `libosmscout/include/osmscout/TypeConfig.h` still defines it as `27`,
  and no version-bearing file is in the diff.

Because the feature is unregistered, a type configuration cannot name it and
`GenRouteDat::Import` builds no `TollFeatureValueReader` (the `GetFeature(TollFeature::NAME)` guard is
false), so `TollBit()` returns `0` for every way. A database imported by this build is therefore byte-for-
byte the database the previous build produced.

## What was NOT run, and why

- **The enabled-state behaviour end to end** (a toll variant recorded in the index; a detour chosen with
  `avoidTolls`; a database that records a toll) cannot be exercised here. `maps/` holds v26/v27 data with no
  routable database, and no shipped type file declares `Toll`. Reproducing the measurements requires
  enabling the registration and the style entries, bumping `FILE_FORMAT_VERSION` and re-importing a
  routable extract — the coordinated release this change deliberately does not make. The reference branch's
  equivalent probes were run on a hand-written `--typefile` and a synthetic test map, neither of which is
  in `maps/`.
- **The user-visible effect** waits on the map provider importing with a libosmscout that enables the
  feature and publishing the matching version slot.
- `scripts/format-check.sh` is red repo-wide (unrelated to this change) and was not used; the changed code
  matches its surroundings.

## `openspec validate --strict`

```
$ openspec validate client-java-avoid-tolls --strict
Change 'client-java-avoid-tolls' is valid
```
