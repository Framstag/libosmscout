# Tasks

Precondition: `client-java-route-instruction-metrics` is archived. It rewrites the same `RouteEntry` and
`RouteInstruction` requirement text and the same JNI translation unit, and the deltas here are written against
its result.

## 1. Model in libosmscout-client

- [x] 1.1 Add `libosmscout-client/include/osmscoutclient/routing/RouteStep.h` with
      `osmscout::client::RouteStep`, `RouteSteps` (steps + database mapping) and the nested sub-records of the
      field inventory in design.md, plus `Kind`. No member carries translated or formatted text.
      *(route-step-model: "A route step carries every description its route node holds"; "A route step carries
      no translated or formatted text")*
      Verify: header compiles standalone; a translation unit that includes it and the Qt client's route step
      header must qualify either type (compile check kept as a test in 7.2).
- [x] 1.2 Add the header to both build descriptions: `libosmscout-client/CMakeLists.txt` and
      `libosmscout-client/meson.build`. *(route-step-model: "The new model does not shadow the Qt client's route
      step")* Verify: `cmake --build build` and `meson compile -C build-meson-check` both succeed and the header
      appears in the installed include set (`install_manifest.txt` / `meson install --dry-run`).
- [x] 1.3 Carry node identity and the intersecting objects in the model: database, path object, node index,
      target node index, objects with type and offset, coordinate, distance and time from the start, and the
      route's database mapping. *(route-step-model: "A route step identifies its route node"; "A route publishes
      the references its steps point into")* Verify: Catch2 test in 2.4 asserts identity differs between two
      steps and that a two-database mapping resolves.

## 2. Collector — route description to steps

- [x] 2.1 Add `RouteStepCollector` implementing `RouteDescriptionPostprocessor::Callback` over all 12 hooks,
      one step per described node, in route order, with `kind` derived and nothing translated.
      *(route-step-model: "A route description converts into one step per manoeuvre")*
      Verify: Catch2 test over a fixture route description asserts the step count, the order and that an
      undescribed node produces no step.
- [x] 2.2 Fill leg values as the delta to the preceding step, and the first step's leg from its own node, so the
      legs sum to the route totals. *(route-step-model: "A route description converts into one step per
      manoeuvre")* Verify: Catch2 test asserts the sum of `legDistance` equals the last step's
      `distanceFromStart` and the sum of `legTime` equals the last step's `timeFromStart`.
- [x] 2.3 Report a description kind that no hook handles instead of dropping it silently.
      *(route-step-model: "A route description converts into one step per manoeuvre")*
      Verify: Catch2 test installs a synthetic unknown kind and asserts the collection reports it.
- [x] 2.4 Add the completeness guard: compare the description keys the collector handles against every
      `Key` constant in `RouteDescription.h`. *(route-step-model: "A route step carries every description its
      route node holds")* Verify: Catch2 test fails when a kind is added to core and not handled; run it, then
      run `ctest -R RouteStep` green.
- [x] 2.5 Add the no-text guard: assert that no model member is a composed instruction sentence and that the
      junction exit reference, junction name and destination arrive as their own values.
      *(route-step-model: "A route step carries no translated or formatted text"; "Two clients present the same
      step in different languages")* Verify: Catch2 test over a motorway-leave fixture asserts exit reference,
      junction name and destination are separately readable.

## 3. Navigation producer

- [x] 3.1 Add `RouteStepBuilder` satisfying the existing `RouteInstructionAgent` contract (default-constructible,
      copyable, `GetDistance()` = distance from the route start) and returning the step ahead with
      `remainingDistance`/`remainingTime` and, within the announcement window, `nextNext` as a full step.
      *(route-step-model: "Live guidance extends the same step with progress")*
      Verify: Catch2 test with two positions on one segment asserts the later step has the smaller remaining
      distance and time and that neither is negative.
- [x] 3.2 Test that list steps carry absolute values and never remaining values, and that the arrival is a step
      carrying the destination distance. *(route-step-model: "List entries carry absolute values"; "The arrival
      is a step")* Verify: Catch2 test asserts non-zero arrival distance and that it is the largest in the list.

## 4. Java projection of the model

- [x] 4.1 Add `libosmscout-client-java/java/com/framstag/libosmscout/client/RouteStep.java` with one nested type
      per sub-record, absent sub-records `null`, and `Kind`; document that values are untranslated.
      *(route-step-model: "A route step carries no translated or formatted text")*
      Verify: class compiles; a JUnit test constructs it directly and asserts every accessor.
- [x] 4.2 Publish the steps on the route result: `RouteEntry.steps` plus the database mapping, leaving
      `descriptions`, `distance`, `duration` and the four per-instruction arrays untouched.
      *(route-calculation: "A calculated route publishes structured steps"; "The steps carry the route's database
      mapping"; "Existing description lines are unchanged")*
      Verify: JUnit test over a real map asserts `steps` is non-empty, ordered, and that `descriptions` is
      unchanged compared to the pre-change output for the same route.
- [x] 4.3 Marshal every sub-record across JNI in `libosmscout-client-java/src/OSMScoutClient.cpp`, on both the
      calculate and the recalculation path (the file has two route paths — both must be covered).
      *(route-step-model: "A route step carries every description its route node holds")*
      Verify: one JUnit assertion per sub-record with a known non-default value on a route fixture; run with
      `-Dnative.lib.dir=` pointing at the built native library.
- [x] 4.4 Test that a step's facts do not depend on the description lines: a route whose lines cannot be aligned
      still publishes its steps. *(route-calculation: "A calculated route publishes structured steps")*
      Verify: JUnit test asserts `steps` is non-null and non-empty while the four instruction arrays are null.

## 5. Navigation instruction gains the facts it drops

- [x] 5.1 Extend the C++ `JavaRouteInstruction` and its builder so the junction exit reference and name, the
      destinations, the way type and the max speed are filled from the step of the same node instead of being
      ignored. *(turn-by-turn-instructions: "Java RouteInstruction data class")*
      Verify: Catch2/JNI test asserts a motorway-leave instruction carries the exit reference and destinations.
- [x] 5.2 Extend the Java `RouteInstruction` with those fields as "absent"-capable values, keeping every existing
      constructor arity and keeping `description`/`shortDescription` byte-identical.
      *(turn-by-turn-instructions: "Java RouteInstruction data class")*
      Verify: JUnit test asserts old constructors compile and default the new fields to absent, and that the two
      text fields equal the values recorded before the change.
- [x] 5.3 Assert that the description-derived instruction and the step of the same manoeuvre agree on turn type,
      exit reference, junction name and destinations. *(turn-by-turn-instructions: "Route list instructions and
      the step model agree")* Verify: JUnit test over a motorway route compares both representations.

## 6. JavaScout consumes the model

- [x] 6.1 Render the route description list from `RouteEntry.steps`, one row per manoeuvre step and one per
      step that carries a POI, with the row's distance and time from the step's leg values, keeping the
      `descriptions` fallback for a result without steps.
      *(route-description: "The route description list renders from structured steps")*
      Verify: unit test on the row-building code asserts one row per manoeuvre step, no row for a step with
      only segment facts, the POI row and the fallback path.
- [x] 6.2 Show the exit reference and the destination sign texts in a motorway row, taken from the step.
      *(route-description: "Every manoeuvre of a motorway route appears")*
      Verify: test over a motorway fixture asserts the row text contains the reference and the destination.
- [x] 6.3 Delete `JavaScout/src/main/java/com/framstag/libosmscout/RouteInstruction.java` (the unused textual
      parser) and remove any text-based header filtering from the list rendering.
      *(route-description: "The application does not interpret description text")*
      Verify: `grep -rn "startsWith(\"---\|indexOf(\" \[" JavaScout/src/main/java` returns nothing; the module
      compiles with `mvn -q -DskipTests package`.
- [x] 6.4 Show the exit reference and destination in the next-turn overlay from the instruction's fields.
      *(turn-by-turn-instructions: "The next-turn display shows the exit and the destination as data")*
      Verify: test over an instruction carrying reference and destinations asserts both appear; a turn
      instruction without them renders as before.

## 7. Integration and guards

- [x] 7.1 Full CMake build and the whole existing suite stay green:
      `cmake --build build`, then `cd build && ctest -j 2 --output-on-failure` (Qt GUI tests under `xvfb-run`).
      Verify: no new failures against the pre-change baseline.
- [x] 7.2 Meson build stays green: `meson compile -C build-meson-check` and
      `meson test --timeout-multiplier 2 -C build-meson-check --print-errorlogs`.
      Verify: no new failures; the new library files are compiled by both systems.
- [x] 7.3 Java side: `libosmscout-client-java` JAR and native library build, then
      `cd JavaScout && mvn test -Dnative.lib.dir=<build>/lib` (JavaFX tests under `xvfb-run`).
      Verify: all tests pass, including the tests added in groups 4-6.
- [x] 7.4 Compile-level collision check: a small translation unit including both the new model header and
      `osmscoutclientqt/RouteStep.h` fails unqualified and compiles with either type qualified
      *(route-step-model: "The new model does not shadow the Qt client's route step")*.
      Verify: the check is part of the Catch2 suite and passes.
- [x] 7.5 Qt untouched: `libosmscout-client-qt` headers and their exported symbols are unchanged.
      *(route-step-model: "Existing Qt behaviour is unchanged")*
      Verify: `git diff --stat` shows no change under `libosmscout-client-qt/`; the Qt GUI tests still pass.
- [x] 7.6 Documentation: update `libosmscout-client-java/AGENTS.md` (new `RouteStep` type in the data-type table,
      the two route paths in `OSMScoutClient.cpp`) and `JavaScout/AGENTS.md` (route rows rendered from steps).
      Verify: the type tables list `RouteStep`; `libosmscout-client/AGENTS.md` names the new public header.
- [x] 7.7 Static checks on the new and changed C++/Java files: `uncrustify` with `.uncrustify`, `clang-tidy` with
      `.clang-tidy`, `scripts/cppcheck.sh`, and `cspell` with `cspell.json`.
      Verify: no new findings in the files this change touches.

## Workflow follow-up

- Follow-up change (not this one): make `libosmscout-client-qt`'s route description builder an adapter over
  `osmscout::client::RouteStep` and surface the new fields in `osmscout::RouteStep` additively.
- Follow-up change: retire the `descriptions` text path from the Java API once both Java applications read steps.
