# Verification

Branch `routing-failure-reason` (base `master`). Reference: commit `dff960e34` on
`origin/naviveylin-local`, of which this change ports the six `libosmscout/` files only. The
`libosmscout-client-java/` half of that commit is deliberately excluded (a stacked PR follows).

## Port fidelity

Net change of this branch equals the reference's net change for the six `libosmscout/` routing
files:

```
git diff --stat
 libosmscout/include/osmscout/routing/AbstractRoutingService.h      |  66 +
 libosmscout/include/osmscout/routing/MultiDBRoutingService.h       |  13 +
 libosmscout/include/osmscout/routing/SimpleRoutingService.h        |  18 +
 libosmscout/src/osmscout/routing/AbstractRoutingService.cpp        |  11 +
 libosmscout/src/osmscout/routing/MultiDBRoutingService.cpp         |  14 +-
 libosmscout/src/osmscout/routing/SimpleRoutingService.cpp          | 141 +-
 6 files changed, 253 insertions(+), 10 deletions(-)
```

Only whitespace differs from the reference tree: this branch keeps a single blank line before the
way-selection comment of `SimpleRoutingService::GetClosestRoutableNode`, the reference has two. The
reference's `SimpleRoutingService.cpp` hunk applied cleanly to master; no re-implementation of its
intent was needed.

## CMake build

```
cmake --build /home/tim/projects/libosmscout/build --target OSMScout -j 4
```

Result: passed. `AbstractRoutingService.cpp`, `SimpleRoutingService.cpp` and
`MultiDBRoutingService.cpp` recompiled, `libosmscout.so.1.1.1` linked. No new warning from the
touched files.

```
cmake --build /home/tim/projects/libosmscout/build --target MultiDBRoutingTest -j 4
```

Result: passed. Rebuilt the test executable (stale `build/` binary replaced) before running it.

## MultiDBRoutingTest (routes inside `Tests/data/testregion`)

```
/home/tim/projects/libosmscout/build/Tests/MultiDBRoutingTest \
  50.412 14.534 50.424 14.6013 /home/tim/projects/libosmscout/Tests/data/testregion
```

Result: passed - the router opened `Tests/data/testregion`, retrieved start and target route nodes,
and produced a real route (`Way 2868[...]` -> `Way 15852[...]`, 5.0 km air-line, actual cost
00:11:46, 205 route nodes loaded). The endpoint-aware resolution still selects a routable way and
the calculation still produces a route.

The `ctest` entry runs it with exactly those arguments (`Tests/CMakeLists.txt`).

## Other reachable tests (CMake)

```
ctest --test-dir /home/tim/projects/libosmscout/build -R \
  "MultiDBRoutingTest|RoutePostprocessorTest|RoutingProgressThrottleTest|RouteStepTimeTest|RouteLengthTest|RoutingProfileSpeedTest|LocationLookupTest|LocationDescriptionServiceTest|AreaIndexLookupTest|WaterIndexTest" \
  --output-on-failure
```

Result: passed, 10/10 tests (routing, route postprocessing/step/length/speed, location lookup and
description, area and water index) - no regression. No v27 map database was needed or used.

## Meson build

```
meson compile -C /home/tim/projects/libosmscout/build-meson-check MultiDBRoutingTest
```

Result: passed. Meson regenerated the tree and built the shared library and the test:
`src_osmscout_routing_AbstractRoutingService.cpp.o`, `src_osmscout_routing_SimpleRoutingService.cpp.o`,
`src_osmscout_routing_MultiDBRoutingService.cpp.o`, `libosmscout.so.1.1.1` and
`Tests/MultiDBRoutingTest`.

```
/home/tim/projects/libosmscout/build-meson-check/Tests/MultiDBRoutingTest \
  50.412 14.534 50.424 14.6013 /home/tim/projects/libosmscout/Tests/data/testregion
```

Result: passed - same route as the CMake-built binary.

## No unit test for the attachability decision

Neither the reference commit nor this PR carries a unit test that exercises
`SimpleRoutingService::IsEndpointAttachable` on its own, and none was invented. The decision needs a
routing database that actually holds route nodes plus a routing profile that permits a way in one
direction only; no test fixture in the repository provides that in isolation, and a fabricated
assertion would have to mock the routing database to be meaningful. `MultiDBRoutingTest` covers the
path end to end: it opens a real database and routes, so `GetClosestRoutableNode` - the function the
attachability decision sits in - runs against route nodes and a real profile on every call.

## OpenSpec

```
openspec validate routing-failure-reason --strict
```

Result: `Change 'routing-failure-reason' is valid`.

## Not verified

- No unit-level assertion on the attachability decision (see above).
- The JNI/Java half of the reference commit (`libosmscout-client-java/`) is out of scope here and
  was not built or tested in this branch.
