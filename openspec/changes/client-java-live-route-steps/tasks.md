# Tasks

Precondition: the route step model and its producer `RouteStepBuilder` are on master (`route-step-model`,
archived as `2026-10-27-client-route-step-model`). This change consumes them; it adds no model code.

## 1. Bridge: publish steps on the listener

- [x] 1.1 `JavaNavigationController` runs
      `RouteInstructionAgent<osmscout::client::RouteStep, osmscout::client::RouteStepBuilder>`; include
      `<osmscoutclient/routing/RouteStepBuilder.h>`. *(route-step-model: "The Java navigation listener publishes
      route steps")* Verify: the JNI translation unit compiles
      (`cmake --build build --target osmscout_client_java`).
- [x] 1.2 Rename the listener callbacks to `onRouteSteps(RouteStep[])` and `onNextRouteStep(RouteStep)` and
      register those descriptors in `GetNavigationListenerMethods`. *(route-step-model: "The listener receives
      steps, not prose")* Verify: `javap -s` on the built `NavigationListener` reports the registered
      descriptors; `scripts/check-jni-signatures.sh` passes.
- [x] 1.3 Dispatch `RouteInstructionsMessage<RouteStep>`/`NextRouteInstructionsMessage<RouteStep>` through
      `CreateJavaRouteStep`, withholding a list the bridge cannot fully build.
      *(route-step-model: "The following manoeuvre arrives as a nested step"; "A list the bridge cannot fully
      build is withheld")* Verify: `CreateJavaRouteStep` already marshals the nested `nextNext`; the live step
      list test (3.3) exercises the boundary.
- [x] 1.4 Delete the prose machinery: `JavaRouteInstruction`, `JavaRouteInstructionBuilder`, `CollectCallback`,
      `MoveToDescription`/`MoveToTurnType`, `CreateJavaRouteInstruction`, `DispatchRouteInstructions`,
      `DispatchNextRouteInstruction` and the `RouteInstruction`/`TurnType` global refs.
      *(route-step-model: "The listener receives steps, not prose")* Verify: the JNI translation unit compiles
      and `scripts/check-jni-signatures.sh` reports no new dead function.

## 2. JavaScout: read the step callbacks

- [x] 2.1 Add `JavaScout/src/main/java/com/framstag/libosmscout/JavaScoutInstructions.java`: word a step (and a
      step list) into the `RouteInstruction` rows the overlay and route panel render.
      *(route-step-model: "The listener receives steps, not prose")* Verify: the app's sources compile.
- [x] 2.2 `MainController` implements `onNextRouteStep`/`onRouteSteps` instead of the retired callbacks and
      renders the rows from `JavaScoutInstructions`.
      *(route-step-model: "The step ahead carries the progress values")* Verify: `@Override` on the new
      callbacks is a compile error against the old listener, so the JavaScout build is the guard.
- [x] 2.3 `MainController` imports `com.framstag.libosmscout.client.RouteStep` (the ported source relied on a
      later commit for this import; the app does not compile without it).
      Verify: `mvn -o -q -DskipTests test-compile` succeeds.

## 3. Tests

- [x] 3.1 `OSMScoutClientInstructionDistanceTest` reads the step callbacks: the arrival is the last step and
      carries the largest distance from the start; the remaining distance shrinks as the position advances.
      *(route-step-model: "The step ahead carries the progress values")* Verify: compiles; needs a map
      database to run.
- [x] 3.2 `OSMScoutClientNavigationLiveTest` waits for a next step instead of a next instruction.
      *(route-step-model: "The listener receives steps, not prose")* Verify: compiles; needs a map database.
- [x] 3.3 `OSMScoutClientRouteStepTest` compares the live step list with the route's published steps (same node,
      same kind) instead of instructions with steps, and fixes the ported `return steps;`.
      *(route-step-model: "The following manoeuvre arrives as a nested step")* Verify: compiles; needs a map
      database.
- [x] 3.4 Existing Catch2 tests still pass: `ctest -R "RouteStep|Navigation"` stays green (no C++ core change).

## 4. Verification

- [x] 4.1 `cmake --build build --target osmscout_client_java` succeeds.
- [x] 4.2 The JavaScout Java sources compile against the freshly built client JAR
      (`mvn -o -q -DskipTests test-compile` in `JavaScout`, after installing the built JAR to the local
      repository). This is what proves the callback rename reaches the `@Override`s.
- [x] 4.3 `bash scripts/check-jni-signatures.sh` reports a match.
- [x] 4.4 `openspec validate --strict` passes.
- [x] 4.5 The three navigation tests cannot be run here (no map database usable with the current data format);
      recorded in `verification.md` rather than weakened.
