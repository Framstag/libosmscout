# Design

## Context

Evidence from the tree (on the stacked base `client-java-live-route-steps`):

- `NavigationListener` publishes live guidance as `RouteStep`: `onRouteSteps(RouteStep[])` for the route
  ahead and `onNextRouteStep(RouteStep)` for the step ahead, the latter carrying the remaining distance and a
  nested `nextNext` step. (#1921.)
- `RouteStep` carries values only — its `kind`, the `turn`/`motorway*` move, `name`, `junction`, the
  `roundaboutLeave.exitCountTaken` ordinal, `legDistance`/`legTime`, `remainingDistance` and the nested
  `nextNext`. It carries no text (`route-step-model`: "A route step carries no translated or formatted
  text").
- Before this change JavaScout converted each step to a `RouteInstruction` (`JavaScoutInstructions.of`) and
  rendered that row's `description`/`shortDescription`/`streetName`/`turnType`. That is a second shape and it
  pins the wording to a type the library retires.
- `RouteRowBuilder` already built its list rows from the steps; only its `nextTurnDetail` and its
  description-line fallback still took a `RouteInstruction` or read `route.descriptions`.

## D1 — The wording lives on the consumer side

`NextTurnText` (renamed from `JavaScoutInstructions`) is a step-to-text helper in the JavaScout application
package, not in the client library. It exposes `turnType`, `move`, `street`, `distanceTo`, `wording` and
`line`, all reading the step's own values. The library holds no prose and no turn-type mapping; the scout's
English wording is the application's, and a second application with a different language words the same steps
differently.

## D2 — The display is driven by the step, not by an instruction

`MainController.updateNextTurnOverlay(RouteStep)` and `RoutePanel.updateInstructionList(RouteStep[])` take the
steps directly. The overlay's line is `NextTurnText.line(step)`, its arrow `NextTurnText.turnType(step)`, its
detail line `RouteRowBuilder.nextTurnDetail(step)`, and the "next next" row is the nested `step.nextNext`
worded by the same helper. No `RouteInstruction` is constructed anywhere in JavaScout.

## D3 — The prose fallback goes, the prose type stays

`RouteRowBuilder` no longer falls back to `route.descriptions`: the list carries the steps or nothing. The
`RouteInstruction` type itself is left untouched in `libosmscout-client-java`, and the `route-description`
scenario that describes the fallback is retired with the later `retire-instruction-prose` change; this change
only stops JavaScout using the prose path.

## Risks

| # | Risk | Mitigation |
|---|---|---|
| 1 | A wording difference reaches a screen (a manoeuvre worded by the app instead of the helper's prose) | `NextTurnText.line` reproduces the helper's wording for the same step; `RouteRowBuilderTest` covers the detail line, the list rows are unchanged |
| 2 | Removing the description-line fallback drops rows for a library of older vintage | The library publishes the steps for every calculated route; the fallback path is removed with the prose path in `retire-instruction-prose` |
| 3 | The app stops compiling against the new listener callbacks | `mvn -o -q -DskipTests test-compile` in `JavaScout` against the freshly built client JAR is the guard |

## Out of scope / follow-ups

- Retiring `RouteInstruction` (the Java type, its constructors and the description-derived instruction path)
  and the `turn-by-turn-instructions` requirements that still name the prose bridge — the later
  `retire-instruction-prose` change.
- Wording the steps for a second Java application.
- Any core (`libosmscout`) or client-library change: none is needed.
