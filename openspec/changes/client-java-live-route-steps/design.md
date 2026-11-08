# Design

## Context

Evidence from the tree:

- `libosmscout/include/osmscout/navigation/RouteInstructionAgent.h` is a template over the record it
  produces. Its contract on that record is default-constructibility, copyability and `GetDistance()`.
- `libosmscout-client/include/osmscoutclient/routing/RouteStepBuilder.h` provides
  `osmscout::client::RouteStepBuilder`, which already satisfies that contract and returns the step ahead with
  `remainingDistance`/`remainingTime` and, within the announcement window, `nextNext` as a full step. It is
  the same producer the route step model spec's "Live guidance extends the same step with progress" describes.
- `libosmscout-client-java/src/OSMScoutClient.cpp` already has `CreateJavaRouteStep` (and `CreateJavaRouteSteps`)
  with the cached `g_RouteStepJni` class, constructor and field ids, used by the two route-calculation paths.
- The bridge's live guidance instead used `JavaRouteInstruction`/`JavaRouteInstructionBuilder`, a second walker
  over the same route description that composes English sentences (`MoveToDescription`, "Exit N"), maps the
  move to a turn-type string (`MoveToTurnType`) and assembles its own "next next" hint.
- `NavigationListener`'s guidance callbacks are Java `default` methods, so a rename is silent for a listener
  that does not override them. `JavaScout`'s `MainController` overrides both, so the rename is a compile error
  there and the app is adapted in the same change.

## D1 — Publish the step model on the listener, not a converted instruction

| Option | Rationale | Risk |
|---|---|---|
| **A. `onRouteSteps(RouteStep[])`/`onNextRouteStep(RouteStep)`, marshalled with the existing `CreateJavaRouteStep` (chosen)** | Live guidance publishes exactly what a calculated route publishes; one projection, one record, no prose in the library. Reuses the existing JNI marshalling, so the boundary encoding is already exercised by the route tests | Source-breaking for a listener that overrides the guidance callbacks; there are two in the tree and both are adapted here |
| B. Keep `onRouteInstruction`, build the instruction from the step | No listener change | Keeps the prose type and a second wording path in the consumer-facing API — the defect being removed; the library would still have to word the manoeuvre |
| C. Publish both callbacks | No break | Two shapes for one manoeuvre drift again; the spec exists so they cannot |

Chosen A. `RouteInstruction` stays in the API untouched; a later change retires it. The consumer that wants
instruction-shaped rows words them itself, which is where the wording belongs.

## D2 — Where the wording lives

`JavaScoutInstructions` is a new class in the JavaScout application package, not in `libosmscout-client-java`.
The client library keeps no prose and no turn-type mapping; the scout's English wording and its choice of a
turn icon are the application's, read from the step's `kind`, `turn`/`motorway*` move, exit ordinal, street
name and way type. This mirrors the intent of `route-step-model` ("A route step carries no translated or
formatted text"). A second Java application with a different language words the same steps differently.

## D3 — Deleted prose machinery

`JavaRouteInstruction`, `JavaRouteInstructionBuilder`, its `CollectCallback` and the
`MoveToDescription`/`MoveToTurnType` helpers are removed, together with the `RouteInstruction`/
`TurnType` global refs and `CreateJavaRouteInstruction`/`DispatchRouteInstructions`/`DispatchNextRouteInstruction`.
`RouteInstruction` the *Java* type and its JNI projection are not deleted — they are what a calculated route's
description path and the migrated JavaScout rows still use. Only the guidance path that produced it from the
prose builder goes.

## D4 — Measurement of the per-position path

One coordinate-free `osmscout::log.Info()` line per publication reports the step count and the marshalling
time, because the step record is richer than the instruction it replaces (nested records, boxed values, a
nested next-next step) and this is the per-position update path. The log line carries no coordinate, so a
release build does not write a position to the log.

## Sequence — live guidance after this change

```
GPS fix -> NavigationEngine
              |
              v
   RouteInstructionAgent<client::RouteStep, client::RouteStepBuilder>
        |  builder: step ahead + remaining distance/time + nested nextNext
        v
   NextRouteInstructionsMessage<client::RouteStep>
        |
        v
   DispatchMessage -> DispatchNextRouteStep / DispatchRouteSteps
        |  CreateJavaRouteStep (existing projection, nested sub-records)
        v
   NavigationListener.onNextRouteStep(RouteStep) / onRouteSteps(RouteStep[])
        |
        +--> JavaScout MainController -> JavaScoutInstructions -> instruction rows
```

## Risks

| # | Risk | Mitigation |
|---|---|---|
| 1 | A listener that overrides the guidance callbacks stops compiling | The two callbacks are `default` methods and the only overriding class in the tree (JavaScout `MainController`) is adapted here; the JavaScout build is the guard |
| 2 | The step projection misses a sub-record the instruction carried | `CreateJavaRouteStep` already marshals nested `nextNext`; `OSMScoutClientRouteStepTest` matches a live step list against the route's published steps |
| 3 | The per-position path becomes slower | One log line measures the marshalling; the design records it rather than assuming |

## Out of scope / follow-ups

- Retiring `RouteInstruction` (Java type, its constructors and the description-derived instruction path) and
  the `turn-by-turn-instructions` requirements that still name the prose bridge.
- Wording the steps for the second Java application.
- Any core (`libosmscout`) change: none is needed; the agent template already accepts the step builder.
