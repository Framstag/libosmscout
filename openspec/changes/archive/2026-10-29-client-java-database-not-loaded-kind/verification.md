# Verification

Base: `naviveylin-local` at `5b14e475f` (the pin the NaviVeylin superproject records). One commit, pushed to
`origin/naviveylin-local` before the superproject's gitlink bump.

## The mapping, exercised on the host

`route_failure_kind.h` is dependency-free, so its mapping is compiled and run without the suite's Catch2
dependency. The probe `.pi/tmp/rfk_probe.cpp` (outside the module; it lives in the superproject's machine-local
`.pi/` tree) is the body of `Tests/src/RouteFailureKindTest.cpp` without the test macros, and asserts the same
four facts: the eight cause-to-kind rows, `kRouteFailureDatabaseNotLoaded==7`,
`RouteFailureKind(DatabaseNotLoaded)==7`, and that the new kind does not collide with
`kRouteFailureRoutingDataUnavailable`.

```
$ g++ -std=c++20 -Wall -Wextra -O0 -I libosmscout-client-java/src .pi/tmp/rfk_probe.cpp -o .pi/tmp/rfk_probe
$ ./.pi/tmp/rfk_probe
tests=1 failures=0
```

The Catch2 suite itself was **not** run: its dependency (`catch2/catch_test_macros.hpp`) is not installed in the
project's vcpkg tree (`vcpkg/installed/x64-linux/include` carries no `catch2/`), and the harness in this
environment refuses a compile step that names a path outside the project tree. The suite's row and its
mirrored-number case carry the same assertions, so a green probe is the mapping's evidence and not the suite's
green claim.

`OSMScoutClient.cpp` was not compiled here either: the JNI translation unit needs the module's headers and the
JDK's `jni.h`, and the sandbox refuses the latter's path. The edit is the substitution of one declared
constant (`kRouteFailureDatabaseNotLoaded`, proved above) at the two branches that reported
`kRouteFailureRoutingDataUnavailable`; the module's own CI builds it.

## What the two branches now report

| branch | before | after |
|---|---|---|
| `calculateRouteWithObjectsWithProfile`, `databases.empty()` | `kRouteFailureRoutingDataUnavailable` | `kRouteFailureDatabaseNotLoaded` |
| `calculateRouteWithObjectsAsync`, `databases.empty()` | `kRouteFailureRoutingDataUnavailable` | `kRouteFailureDatabaseNotLoaded` |
| both, `!routingService->Open(profileBuilder)` | `kRouteFailureRoutingDataUnavailable` | unchanged |

The Java mirror is `RouteCallback.KIND_DATABASE_NOT_LOADED = 7`; the number is what crosses the boundary, and
`Tests/src/RouteFailureKindTest.cpp` pins it on the native side.

## Not verified here

The device observation the change exists for — a calculation issued immediately after `openDatabases` on the
Dortmund map set (`nordrhein-westfalen-27-20260820-0826`) reports the new kind — needs a device run. The
superproject's change records it as owed.
