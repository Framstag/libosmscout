# Verification

## What was verified

| # | Command | Result |
|---|---|---|
| 1 | `cmake --build build --target osmscout_client_java` | PASS — rebuilt the JNI translation unit; `libosmscout_client_java.so` linked. The build re-ran CMake and compiled the Java sources (`[14/39] Compiling Java sources and generating JNI headers`). |
| 2 | `cmake --build build --target java_jar` | PASS — `libosmscoutclientjava.jar` repackaged with the renamed `NavigationListener`. |
| 3 | `mvn -q org.apache.maven.plugins:maven-install-plugin:3.1.1:install-file -Dfile=build/libosmscout-client-java/libosmscoutclientjava.jar -DgroupId=net.sf.libosmscout -DartifactId=libosmscout-client-java -Dversion=1.0-SNAPSHOT -Dpackaging=jar` | PASS — the freshly built JAR replaces the stale local-repository copy so JavaScout compiles against the new listener. |
| 4 | `mvn -o -q -DskipTests test-compile` (cwd `JavaScout`) | PASS — JavaScout main and test sources compile against the new listener, including the `@Override public void onNextRouteStep(RouteStep)` / `onRouteSteps(RouteStep[])` in `MainController` and the three navigation tests. |
| 5 | `javap -s -classpath build/libosmscout-client-java/java/classes com.framstag.libosmscout.client.NavigationListener` | PASS — reports `onRouteSteps` descriptor `([Lcom/framstag/libosmscout/client/RouteStep;)V` and `onNextRouteStep` descriptor `(Lcom/framstag/libosmscout/client/RouteStep;)V`, exactly what `GetNavigationListenerMethods` registers. |
| 6 | `bash scripts/check-jni-signatures.sh` | PASS — `JNI signatures match: 60 native declarations checked, 5 dead JNI functions reported above.` The 5 reported functions are pre-existing and unrelated (`getMaxSpeedAt`, `reloadBasemap`, `getAddressAt`, `getDatabaseBoundingBox`, `searchLocationByForm`). |
| 7 | `openspec validate --strict client-java-live-route-steps` | PASS — `Change 'client-java-live-route-steps' is valid`. |
| 8 | `cmake --build build --target RouteStepModelTest RouteStepTimeTest` | PASS — rebuilt the two model test binaries against the current tree. |
| 9 | `ctest --test-dir build -R RouteStep --output-on-failure` | PASS — `100% tests passed out of 2` (`RouteStepTimeTest`, `RouteStepModelTest`). |

## What could not be run, and why

The three JavaScout navigation tests (`OSMScoutClientInstructionDistanceTest`,
`OSMScoutClientNavigationLiveTest`, `OSMScoutClientRouteStepTest`) were **not run**. They need a map database
built by the import pipeline, and the only data available in the tree (`maps/`) is data-format v26 while the
current build reads v27 — there is no usable database. Importing one is out of scope for this change. The
tests were compile-verified (command 4) and were not weakened to make them run. Their runtime behaviour is
covered by the same step projection the route-calculation tests exercise.

This change touches only the Java JNI bridge and the Java application, not `libosmscout` core; the two
`RouteStep` Catch2 tests were run anyway (commands 8–9) and pass.

## Port notes

- The two source commits `de37a2bd1` (bridge + listener) and `0a84ce126` (JavaScout) were ported by their exact
  delta: `git diff HEAD` of the seven changed files was byte-identical to
  `git diff de37a2bd1^ 0a84ce126` before the two fixes below (both sides `md5 a0e481c1c2116328bb3183b1d37719ab`).
- Two one-line mechanical fixes were required to make the tree build. `0a84ce126` is not self-contained: it
  relies on the later, out-of-unit commit `999acc16f` for both.
  1. `JavaScout/.../MainController.java`: added `import com.framstag.libosmscout.client.RouteStep;` — the new
     `onNextRouteStep(RouteStep)`/`onRouteSteps(RouteStep[])` overrides do not resolve without it.
  2. `JavaScout/.../client/OSMScoutClientRouteStepTest.java`: `return instructions;` → `return steps;` in
     `stepList()`, whose local was renamed to `steps` by the ported commit.
- The ported `LogDispatch` comment in `OSMScoutClient.cpp` references the source change name
  `route-steps-live-guidance`; this change is named `client-java-live-route-steps`. The reference was kept
  byte-identical to the source delta rather than edited; it is a comment only.
