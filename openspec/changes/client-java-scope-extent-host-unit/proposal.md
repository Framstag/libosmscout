# Proposal

## Why

The derivation of a resolved search scope's geographic extent lives as a `static` function inside the
JNI translation unit (`libosmscout-client-java/src/OSMScoutClient.cpp`), which the host test suite cannot
compile. A regression that derived the box from the wrong object — or stopped deriving one — was therefore
caught by no test at all (NaviVeylin issue #20).

## What Changes

- The extent derivation moves out of the JNI translation unit into its own host-compilable unit beside the
  existing scope helpers; it touches no JNI, so the host test suite can compile and link it.
- The one call site in the search path uses the moved, namespace-qualified name; the derivation's behaviour
  is unchanged.
- A host test imports a location fixture and asserts, over every outline region it holds, that the derived
  box is the bounding box of the region's own object, that a hit inside it is kept and one a degree outside
  is dropped, and that a region without a loadable object leaves the extent unset and admits everything.
- Both build systems register the moved source and the new test.

No behaviour of a search changes: this is an extraction that makes an existing decision reachable by the
host suite. No JNI signature, database, map or style file format changes.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `search-context-region`: adds the requirement that the derivation of a resolved region's extent is a unit
  the host suite compiles and links without the JNI build, so a derivation from the wrong object is
  observable in the host suite.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/search_scope_extent.h` — the moved derivation's declaration, beside
  `search_scope.h`.
- `libosmscout-client-java/src/search_scope_extent.cpp` — the moved derivation, verbatim.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the function is removed and the one call site uses the
  qualified name; no other change.
- `Tests/src/SearchScopeExtentTest.cpp` — the host test, which imports its own fixture.
- `libosmscout-client-java/CMakeLists.txt`, `libosmscout-client-java/src/meson.build` — the moved source.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — the new test executable and its registration.

No database, map or style file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change.

Consumers: the host test suite, which now exercises the derivation that every scoped search rests on.
