# Design

## Context

See `proposal.md` — Why.

`libosmscout-client-java/src/search_scope.h` is a header-only, dependency-free pair of helpers in
namespace `naviveylin`:

- `NormalizeDepthToAdminLevel(size_t depth)` — a hierarchy depth (root=1) mapped to the OSM
  admin_level scale (root=0, +2 per level), returning 0 for depth 0.
- `ShouldExpandScope(uint8_t parentLevel, uint8_t maxLevel)` — `parentLevel != 0 && parentLevel >=
  maxLevel`, i.e. expansion is allowed when the parent level is known and no coarser than the cap.
- `kMaxSearchRegionLevel = 5` — the cap the location search uses, documented as covering the common
  "one up, one down" case (a kreisfreie Stadt and the towns around it sharing a district parent).

Constraints that shape the approach:

- The header lives next to the JNI bridge, and `OSMScoutClient.cpp` includes it as `"search_scope.h"`
  from the same directory. No test target has that directory on its include path, and the JNI library
  cannot be linked into a test without JNI and a JVM.
- `GetRegionLevel()` in `OSMScoutClient.cpp` feeds the depth of a real hierarchy chain
  (`ResolveAdminRegionHierachie`) into `NormalizeDepthToAdminLevel()`, so the two helpers compose:
  the depth mapping is only ever consumed by the cap comparison.

No control flow is added by this change, so the design carries no sequence diagram.

## Goals / Non-Goals

**Goals:**

- Pin both helpers at their boundaries, and pin the composed rule against a realistic chain, so a
  change to the cap or to the scale becomes a visible test failure rather than a silent behavior
  change.
- Keep the test self-contained: no database, no fixture, no data file, fast enough for every run.
- Register the test in both build systems, so neither build loses it.

**Non-Goals:**

- Testing the scope *decision* in the search (`DoSearchLocations`) — it needs the JNI entry point, an
  open database and an admin-region handle.
- Exercising `GetRegionLevel()` — it loads database objects to read the `admin_level` feature.
- Moving the helper out of `libosmscout-client-java/src/` so the test needs no extra include path.
  That is a production refactor touching both build systems and the JNI bridge; TODO.md records it.

## Decisions

### D1 — Test the pure helpers, not the search

Alternatives: (a) unit-test `search_scope.h`, (b) test the scope decision in `DoSearchLocations`
through the JNI entry point, (c) leave the helpers untested and rely on the search tests.

Chosen: (a). (b) needs an open database, an admin-region handle and the JNI bridge, and would add a
database-dependent test that skips in continuous integration; the rule it would cover is decided by
the helpers, which (a) tests directly. (c) is the status quo the proposal rejects: the helpers exist
precisely because they were extracted to be testable.

### D2 — Include the header through its own directory, not through a copy or a move

Alternatives: (a) add `libosmscout-client-java/src` to the test target's include path in both build
systems, (b) duplicate the header into `Tests/include`, (c) move the header into `libosmscout-client/`.

Chosen: (a). (b) would test a copy, which can drift from what the search compiles against — worse than
no test. (c) is the better long-term home, but it edits the JNI bridge and both build systems' target
definitions for a three-line helper, and a reviewer of a test-only change should not have to assess a
production move; (a) costs one include-path entry per build system and keeps the test coupled to the
file the search really uses.

### D3 — Pin the cap value, and say why

Alternatives: (a) assert `kMaxSearchRegionLevel == 5`, (b) derive the cap from the header and let its
value float.

Chosen: (a). The value is a tuned product decision — the header explains it covers a kreisfreie Stadt
and its neighbouring towns under one district parent — so a change to it changes how much data a
search reads and how many results it returns. A test that follows the constant silently would not
notice. The test states that changing the cap is a product decision, which is what a reader of a
failing assertion needs to know.

### D4 — Plain assertions and comments, no table

Alternative: express the cases as a table of rows, as the matcher tests do. Rejected here: the cases
are two functions with a handful of scalar expectations each, there is no repeated setup to factor
out, and a table would add a row type and its initialization for no reduction in boilerplate.

## Risks / Trade-offs

- [The test is coupled to a file in another module's source directory] → Deliberate (D2), with the
  follow-up recorded in TODO.md rather than done here.
- [Pinning the cap makes a deliberate change fail the test] → Intended (D3); the comment next to the
  assertion names the decision behind the value.
- [The test covers the rule but not the call site] → Stated as a limitation in the change's
  verification notes; the call site needs a database and is untested before and after.
- [A test in `Tests/` needs registration twice] → Done in both build systems and verified by building
  and running the target in each, since no check compares the two rosters (TODO.md).
