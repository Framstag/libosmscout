# Proposal

## Why

A calculated route already reaches Java as the structured route step model (`RouteEntry.steps`), but live
guidance does not: the Java navigation engine runs the instruction agent over an ad-hoc prose record
(`JavaRouteInstruction`) that carries composed English sentences, a turn-type name and a separately assembled
"next next" hint. The Java navigation listener therefore publishes a different, lossier shape than the route
result, and the client library keeps a prose builder and a turn-type mapping of its own. The client library
is supposed to hold no prose and no turn-type mapping; a surface words a manoeuvre itself from the values the
model publishes.

The engine's instruction agent is a template over the record it produces, and `RouteStepBuilder` already
satisfies that contract for C++ clients. Wiring the step model to live guidance removes the duplicate,
lossier shape without new engine machinery.

## What Changes

- `JavaNavigationController` runs `RouteInstructionAgent<osmscout::client::RouteStep, osmscout::client::RouteStepBuilder>`
  instead of the prose builder.
- `NavigationListener`'s guidance callbacks become `onRouteSteps(RouteStep[])` and `onNextRouteStep(RouteStep)`.
  The dispatch marshals each step through the existing `CreateJavaRouteStep` projection, so a live step carries
  the same sub-records, identity and progress fields a calculated route publishes, with the following manoeuvre
  as a nested `RouteStep`.
- The prose machinery is deleted: `JavaRouteInstruction`, its builder, `CollectCallback` and the
  `MoveToDescription`/`MoveToTurnType` helpers. Nothing publishes guidance text any more.
- `JavaScout` is adapted in the same change so the tree builds: a new consumer-side `JavaScoutInstructions`
  words the published steps into the instruction rows the app's overlay and route panel render, and
  `MainController` implements the two new callbacks.
- The `RouteInstruction` type itself stays in the Java API for now; retiring it is a later change.

## Capabilities

### Modified Capabilities

- `route-step-model`: the Java navigation listener publishes live guidance as the route step model — the same
  record a calculated route publishes, with the position's progress and a nested following manoeuvre — instead
  of an ad-hoc prose instruction record.

## Impact

- `libosmscout-client-java/java/com/framstag/libosmscout/client/NavigationListener.java`: the two guidance
  callbacks are renamed and retyped to `RouteStep`. This is a source-breaking change for a listener that
  overrides them.
- `libosmscout-client-java/src/OSMScoutClient.cpp`: `JavaNavigationController` uses the step builder; the prose
  struct, builder, callback and converter are removed; the dispatch builds each step with the existing
  `CreateJavaRouteStep`; one coordinate-free log line reports the marshalling time and step count of each
  publication.
- `JavaScout/src/main/java/com/framstag/libosmscout/JavaScoutInstructions.java` (new): consumer-side wording of
  a step (and a step list) into `RouteInstruction` rows.
- `JavaScout/src/main/java/com/framstag/libosmscout/MainController.java`: the two listener callbacks.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/`: the three navigation tests read the step
  callbacks.
- `JavaScout` tests that need a map database cannot run in this environment (no usable database for the
  current data format); see verification.
- Untouched: `RouteInstruction.java` (stays for now), the Qt client library, the core `libosmscout` engine.
- Supersedes the bridge requirements of `turn-by-turn-instructions` that name the prose bridge; those are
  retired together with `RouteInstruction` in a later change.
