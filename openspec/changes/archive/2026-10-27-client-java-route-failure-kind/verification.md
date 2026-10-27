# Verification

Branch `client-java-route-failure-kind`, stacked on `routing-failure-reason` (#1905); its base
branch is `origin/routing-failure-reason`, not `master`. Reference: commit `dff960e34` on
`origin/naviveylin-local`, of which this change ports the `libosmscout-client-java/` half only (its
six `libosmscout/` routing files are already in the base branch and are untouched here).

## Port fidelity

```
git diff --stat
 Tests/CMakeLists.txt                                       |   6 +
 Tests/meson.build                                          |   9 +
 .../framstag/libosmscout/client/RouteCallback.java         |  66 ++++
 libosmscout-client-java/src/OSMScoutClient.cpp             | 186 +++++++++++------
 4 files changed, 219 insertions(+), 48 deletions(-)
```

plus the two new files (`libosmscout-client-java/src/route_failure_kind.h`,
`Tests/src/RouteFailureKindTest.cpp`).

The changed-line multiset of the bridge diff equals the reference's:

```
git show dff960e34 -- libosmscout-client-java/src/OSMScoutClient.cpp > /tmp/ref_bridge.diff
git diff -U1 -- libosmscout-client-java/src/OSMScoutClient.cpp > /tmp/mine_bridge.diff
grep -E "^[+-]" /tmp/ref_bridge.diff | grep -vE "^(\+\+\+|---)" > /tmp/ref_lines.txt
grep -E "^[+-]" /tmp/mine_bridge.diff | grep -vE "^(\+\+\+|---)" > /tmp/mine_lines.txt
diff /tmp/ref_lines.txt /tmp/mine_lines.txt
```

Result: no output - every added and removed line, in order, is the reference's. The reference commit
touches no search function of that file, so nothing of the search code (PR #1904, not in this base)
was ported or touched.

`route_failure_kind.h` differs from the reference in one line only: its file comment names this
openspec change (`client-java-route-failure-kind`) instead of the reference's working name
(`distinguish-routing-failures`). `RouteCallback.java` is byte-identical to the reference's version
(`diff` against `git show dff960e34:...RouteCallback.java`: no output).

## Host test for the kind (an addition over the reference)

The reference commit has **no** host test for `route_failure_kind.h`. This PR adds
`Tests/src/RouteFailureKindTest.cpp` (3 test cases, 21 assertions): every enumerator of
`RouteFailureCause` maps to its kind, `Unknown` maps to `kRouteFailureUnknown`, and the seven
`kRouteFailure*` constants are pinned to the numbers `RouteCallback.java` declares (2 and 5 also
checked in the other direction, as the kind a cause is reported with). That is a deliberate
deviation, stated in the PR body.

### CMake

```
cmake --build /home/tim/projects/libosmscout/build --target RouteFailureKindTest
```

Result: passed.

```
/home/tim/projects/libosmscout/build/Tests/RouteFailureKindTest
```

Result: passed - `All tests passed (21 assertions in 3 test cases)`.

### Meson

```
meson compile -C /home/tim/projects/libosmscout/build-meson-check RouteFailureKindTest
meson test -C /home/tim/projects/libosmscout/build-meson-check "Check route failure kind" --print-errorlogs
```

Result: passed - `1/1 libosmscout:Check route failure kind OK 0.01s`, `Ok: 1 Fail: 0`.

## Native library and jar

```
cmake --build /home/tim/projects/libosmscout/build --target osmscout_client_java java_jar
```

Result: passed. `OSMScoutClient.cpp` compiled with no warning, `libosmscout_client_java.so` linked,
the jar was repackaged. No new warning from the touched translation unit.

## JNI callback descriptor (checked by hand, not covered by the script)

```
javap -s -p -classpath /home/tim/projects/libosmscout/build/libosmscout-client-java/libosmscoutclientjava.jar com.framstag.libosmscout.client.RouteCallback
```

```
Compiled from "RouteCallback.java"
public interface com.framstag.libosmscout.client.RouteCallback {
  public static final int KIND_UNKNOWN;
    descriptor: I
  public static final int KIND_ROUTING_DATA_UNAVAILABLE;
    descriptor: I
  public static final int KIND_START_UNROUTABLE;
    descriptor: I
  public static final int KIND_TARGET_UNROUTABLE;
    descriptor: I
  public static final int KIND_NO_ROUTE_FOUND;
    descriptor: I
  public static final int KIND_ROUTING_DATA_FAILURE;
    descriptor: I
  public static final int KIND_ENGINE_FAILURE;
    descriptor: I
  public abstract void onProgress(int);
    descriptor: (I)V

  public abstract void onSuccess(com.framstag.libosmscout.client.RouteEntry);
    descriptor: (Lcom/framstag/libosmscout/client/RouteEntry;)V

  public abstract void onError(java.lang.String);
    descriptor: (Ljava/lang/String;)V

  public default void onErrorDetail(int, java.lang.String, double);
    descriptor: (ILjava/lang/String;D)V

  public abstract void onCancel();
    descriptor: ()V
}
```

The `onErrorDetail` descriptor is `(ILjava/lang/String;D)V`, character for character what the bridge
looks up:

```
OSMScoutClient.cpp: methods.onErrorDetail = env->GetMethodID(cls, "onErrorDetail", "(ILjava/lang/String;D)V");
```

and the call passes the arguments in that order - `static_cast<jint>(kind)`,
`env->NewStringUTF(message)`, `static_cast<jdouble>(nearestRoadDistanceMeters)`. The kind is a
`default` method of the interface, so `GetMethodID` on the implementing class finds it.

```
bash /home/tim/projects/libosmscout/scripts/check-jni-signatures.sh
```

Result: `JNI signatures match: 59 native declarations checked, 5 dead JNI functions reported above.`
The five reported functions (`getMaxSpeedAt`, `searchLocationByForm`, `reloadBasemap`,
`getAddressAt`, `getDatabaseBoundingBox`) are pre-existing and untouched here.

## JavaScout against the freshly built jar

```
./JavaScout/mvnw -f JavaScout/pom.xml install:install-file \
  -Dfile=/home/tim/projects/libosmscout/build/libosmscout-client-java/libosmscoutclientjava.jar \
  -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java \
  -Dversion=1.0-SNAPSHOT -Dpackaging=jar -q
```

Result: passed (`-Dfile` has to be absolute: Maven resolves a relative one against `JavaScout/`).

That the artifact in the local repository really is the jar built above was checked by resolving the
artifact back out of the repository and hashing it:

```
./JavaScout/mvnw -f JavaScout/pom.xml -q \
  org.apache.maven.plugins:maven-dependency-plugin:3.6.1:copy \
  -Dartifact=net.sf.libosmscout:libosmscout-client-java:1.0-SNAPSHOT \
  -DoutputDirectory=/home/tim/projects/libosmscout/build/m2-check
sha256sum build/m2-check/libosmscout-client-java-1.0-SNAPSHOT.jar \
          build/libosmscout-client-java/libosmscoutclientjava.jar
```

Result: both `aa54847f04da05d509ef7bb7f04f7615d4495b440214eadb45a0a61969b7ac09` - byte-identical.
`javap` against the resolved copy shows the same `onErrorDetail` descriptor, so the `.m2` copy used
by the compile is the new one and not a stale jar.

```
./JavaScout/mvnw -f JavaScout/pom.xml clean test-compile
```

Result: `BUILD SUCCESS` - 25 main sources and 36 test sources compiled against the new jar. No caller
change was needed: `onErrorDetail` is a default method.

## Route probe (real database, real callback)

`maps/nordrhein-westfalen` is a v27 database on this machine, so it was used directly (nothing was
imported). A throwaway Java probe (`build/probe/RouteFailureProbe.java`, under the git-ignored
`build/`, not part of the commit) builds the client with `OSMScoutClientBuilder`, opens the database
and calls `calculateRouteAsync`, overriding `onErrorDetail` to print kind, detail and distance:

```
javac -cp build/libosmscout-client-java/libosmscoutclientjava.jar -d build/probe build/probe/RouteFailureProbe.java
java -Djava.library.path=/home/tim/projects/libosmscout/build/libosmscout-client-java \
     -cp build/libosmscout-client-java/libosmscoutclientjava.jar:build/probe \
     RouteFailureProbe /home/tim/projects/libosmscout/maps/nordrhein-westfalen
```

Result (`client built: true`, `openDatabase: true`; the probe pairs are `start→target`):

```
probe [51.514227, 7.465279, 51.515, 7.466]        -> onSuccess(distance=90.1883428083891)
probe [51.5139, 7.4653, 51.5175, 7.4593]          -> onSuccess(distance=1983.2587290290337)
probe [48.1372, 11.5756, 51.514227, 7.465279]     -> onErrorDetail(kind=2, detail=No routable node near start position, nearestRoadDistanceMeters=NaN)
probe [51.514227, 7.465279, 48.1372, 11.5756]     -> onErrorDetail(kind=3, detail=No routable node near destination, nearestRoadDistanceMeters=NaN)
probe [51.37, 6.172, 51.514227, 7.465279]         -> onSuccess(distance=104430.47345948369)
probe [52.2799, 8.0472, 51.514227, 7.465279]      -> onSuccess(distance=124365.27113654146)
probe [51.46, 8.95, 51.514227, 7.465279]          -> onErrorDetail(kind=4, detail=Route calculation failed, nearestRoadDistanceMeters=NaN)
probe [51.05, 9.05, 51.514227, 7.465279]          -> onErrorDetail(kind=2, detail=No routable node near start position, nearestRoadDistanceMeters=NaN)
probe [50.7, 6.05, 51.514227, 7.465279]           -> onSuccess(distance=163007.34309988134)
probe [50.5, 6.25, 51.514227, 7.465279]           -> onSuccess(distance=184653.214858137)
probe [51.37, 6.05, 51.514227, 7.465279]          -> onSuccess(distance=96920.14339475632)
probe [51.9, 6.1, 51.514227, 7.465279]            -> onErrorDetail(kind=4, detail=Route calculation failed, nearestRoadDistanceMeters=NaN)
probe [52.1, 8.45, 51.514227, 7.465279]           -> onSuccess(distance=124513.7460144085)
probe [52.27, 8.3, 51.514227, 7.465279]           -> onErrorDetail(kind=2, detail=No routable node near start position, nearestRoadDistanceMeters=8107.7136569021495)
```

What this shows:

- `kind=2` and `kind=3` are the kinds `KIND_START_UNROUTABLE`/`KIND_TARGET_UNROUTABLE` of
  `RouteCallback.java`, and the failing end is the one that is named.
- `nearestRoadDistanceMeters=8107.71` is the 10 km probe's measurement for a start that has no road
  within the 5 km search radius but one within 10 km - the distance branch works, not only the
  `NaN` branch (the `NaN` cases are points with no usable road within 10 km at all).
- `kind=4` is `KIND_NO_ROUTE_FOUND`, reached with both ends resolvable (Marsberg and Kleve
  surroundings) - the router's `NoRouteFound` is translated, and an unattachable end does not pretend
  to be it.
- The successful routes confirm the resolution change did not break the normal path, and no
  `onErrorDetail` is called for them.

## OpenSpec

```
openspec validate client-java-route-failure-kind --strict
```

Result: `Change 'client-java-route-failure-kind' is valid`.

## Not verified

- **`kind=1` (routing data unavailable), `kind=5` (routing data failure) and `kind=6` (engine
  failure) were not reached by any probe.** They need a client without loaded routing data, a
  database whose route node/area/way file fails to load, or a native failure inside the routing
  thread; the probe has a working v27 database and a working engine, and no such fixture was
  fabricated. Their paths are the ones the reference has (same lines, see the port fidelity check).
- **A cancellation was not probed.** Reporting a cancellation as a cancellation is the pre-existing
  path: `RoutingError::Aborted` maps to `RouteFailureCause::Unknown` in the translation, and the
  `breaker->IsAborted()` check in front of the error report calls `onCancel()` and returns, so no
  error kind is reported for it. Code inspection only, no live breaker abort was driven.
- **No test asserts the bridge against a Java-side callback** (`onErrorDetail` reached from JNI with
  the descriptor) automatically; this was checked by hand here (javap + probe), not by CI.
- The route path for kinds 1/5/6 and the fall-back to `onError` for a class that lacks
  `onErrorDetail` (an implementation that overrides only `onError`) are not covered by an automated
  test; the fall-back is the reference's `CallOnRouteError` (`nullptr` method ID -> `onError`).
