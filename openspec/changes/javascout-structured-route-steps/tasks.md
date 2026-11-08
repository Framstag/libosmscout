# Tasks

Precondition: the live-guidance listener publishes `RouteStep` (`client-java-live-route-steps`). This change
consumes that; it adds no listener or model code.

## 1. JavaScout words and renders from steps

- [x] 1.1 Rename `JavaScoutInstructions.java` to `NextTurnText.java` and turn it into a step-to-text helper
      (`turnType`, `move`, `street`, `distanceTo`, `wording`, `line`), wording a manoeuvre from the step's
      values. *(turn-by-turn-instructions: "The next-turn display is worded from the published step")*
      Verify: the app sources compile.
- [x] 1.2 `MainController` drives the next-turn overlay from `RouteStep` and its nested `nextNext`; the
      listener callbacks pass the steps straight through. *(turn-by-turn-instructions: "The next-turn display
      is worded from the published step")* Verify: `mvn -o -q -DskipTests test-compile`.
- [x] 1.3 `RoutePanel.updateInstructionList` takes `RouteStep[]` and builds its cards with `NextTurnText`.
      *(route-description: "JavaScout words the route list from the published steps")* Verify: the app sources
      compile.
- [x] 1.4 `RouteRowBuilder` builds its rows from the published steps only and takes the next-turn detail from
      the step. *(route-description: "JavaScout words the route list from the published steps";
      turn-by-turn-instructions: "The next-turn display shows the exit and the destination as data")*
      Verify: `ctest -R RouteStep --output-on-failure`.

## 2. Tests

- [x] 2.1 `RouteRowBuilderTest` builds the next-turn detail from steps; the description-line fallback tests
      are removed with the path. Verify: the test compiles and runs without a native library.
- [x] 2.2 Delete `RouteInstructionTest.java` and the instruction cases of `RouteStepModelTest.java`;
      `OSMScoutClientRouteStepTest` reads the step callbacks. Verify: the tests compile.

## 3. Verification

- [x] 3.1 `cmake --build build --target osmscout_client_java` succeeds.
- [x] 3.2 The JavaScout Java sources compile against the freshly built client JAR (`cmake --build build
      --target java_jar` and `mvn -o -q -DskipTests test-compile` in `JavaScout`).
- [x] 3.3 `ctest -R RouteStep --output-on-failure` stays green.
- [x] 3.4 `bash scripts/check-jni-signatures.sh` reports a match.
- [x] 3.5 `openspec validate --strict` passes.
- [x] 3.6 The JavaScout navigation tests cannot run here (no map database usable with the current data
      format); recorded in `verification.md` rather than weakened.
