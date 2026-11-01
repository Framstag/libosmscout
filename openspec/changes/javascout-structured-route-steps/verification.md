# Verification

## Commands and results

- `cmake --build build --target osmscout_client_java` → `ninja: no work to do.` The C++ JNI translation
  unit is unchanged by this change; it is built here so the JavaScout compile has a fresh client on which to
  stand.
- `cmake --build build --target java_jar` → `ninja: no work to do.` The client library's Java sources are
  unchanged by this change; the jar already carries the `onRouteSteps`/`onNextRouteStep` callbacks from the
  stacked `client-java-live-route-steps` change.
- `mvn -o -q -DskipTests -f JavaScout/pom.xml test-compile` → success (no output). Compiles every JavaScout
  source and test source against the installed `net.sf.libosmscout:libosmscout-client-java:1.0-SNAPSHOT` jar.
  This is what proves the step-driven code (`NextTurnText`, `MainController`, `RoutePanel`, `RouteRowBuilder`
  and the tests) compiles.
- `mvn -o -q -Dtest=RouteRowBuilderTest -f JavaScout/pom.xml surefire:test` → `Tests run: 7, Failures: 0,
  Errors: 0, Skipped: 0`. The ported row-builder test runs without a native library or a map database.
- `ctest --test-dir build -R RouteStep --output-on-failure` → 2/2 passed (`RouteStepTimeTest`,
  `RouteStepModelTest`).
- `bash scripts/check-jni-signatures.sh` → `JNI signatures match: 60 native declarations checked, 5 dead JNI
  functions reported above.` The five dead functions are pre-existing and belong to the client library, which
  this change does not touch.
- `openspec validate --strict javascout-structured-route-steps` → `Change 'javascout-structured-route-steps'
  is valid`.

## Not run, and why

- The JavaScout navigation tests (`OSMScoutClientNavigationTest`, `OSMScoutClientNavigationLiveTest`,
  `OSMScoutClientRouteStepTest`) and every other test that opens a map database. There is no map database
  usable in this environment: `maps/` carries data format v26 while the code reads v27, and the environment
  rule forbids importing one. Those tests are compile-verified only (`test-compile`), not weakened.
- `mvn -o -q test` (the whole JavaScout suite) for the same reason: the module's suite contains the
  database-backed tests, so only the pure `RouteRowBuilderTest` was run directly.

## Note

The `route-description` scenario "A route without steps falls back to the description lines" describes a code
path this change removes. Its retirement belongs to the later `retire-instruction-prose` change (which removes
`RouteEntry.descriptions`), so the scenario is left in the frozen spec and noted in the proposal rather than
edited here.
