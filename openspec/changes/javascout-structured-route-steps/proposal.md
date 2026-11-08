# Proposal

## Why

Live guidance now reaches Java as the structured route step model — `NavigationListener.onRouteSteps` and
`onNextRouteStep` carry `RouteStep`, the same record a calculated route publishes. JavaScout, however, still
words its route list and its next-turn display by converting each step into an instruction-shaped row
(`JavaScoutInstructions` building a `RouteInstruction`) and then reading that row's textual fields. The app
therefore keeps a second, lossier shape alive on the consumer side, and its wording is pinned to a type the
library is about to retire.

The wording belongs to the application: a step carries values only, so a surface words the manoeuvre itself
from the step's kind, its move, its exit ordinal and the road it leads onto. This change moves JavaScout onto
that footing for both the route list and the next-turn display.

## What Changes

- `JavaScoutInstructions` becomes `NextTurnText`: a step-to-text helper that words a manoeuvre from the step's
  own values (its kind, the move it carries, the ordinal of a roundabout exit, the road it leads onto) in the
  application's own language. It never builds an instruction.
- `MainController`'s next-turn overlay is driven by `RouteStep`: the manoeuvre line, the arrow and the
  "next next" hint come from the step and its nested follower, and the exit/destination detail line comes from
  `RouteRowBuilder.nextTurnDetail(RouteStep)`.
- `RoutePanel.updateInstructionList` takes `RouteStep[]` and builds its cards with `NextTurnText`.
- `RouteRowBuilder` builds its rows from the published steps only; the description-line fallback is removed,
  because the prose path it falls back to is retired by the later `retire-instruction-prose` change.
- The JavaScout tests of the retired instruction helper are deleted (`RouteInstructionTest` and the
  instruction cases of `RouteStepModelTest`); `RouteRowBuilderTest` and `OSMScoutClientRouteStepTest` read
  steps.

## Capabilities

### Modified Capabilities
- `turn-by-turn-instructions`: the next-turn display words the manoeuvre from the published step and renders
  the exit reference and the destination from the step's fields.
- `route-description`: the route list words each row from the published steps.

## Impact

- `JavaScout/src/main/java/com/framstag/libosmscout/`: `NextTurnText.java` (renamed from
  `JavaScoutInstructions.java`), `MainController.java`, `RoutePanel.java`, `RouteRowBuilder.java`.
- `JavaScout/src/test/java/com/framstag/libosmscout/`: `RouteRowBuilderTest.java`,
  `client/OSMScoutClientRouteStepTest.java`, deleted `client/RouteInstructionTest.java` and the instruction
  cases of `client/RouteStepModelTest.java`.
- Untouched: `libosmscout-client-java` — `RouteInstruction` stays for now and a later change retires it; the
  Qt client library and the `libosmscout` core are unchanged.
- Supersedes the `route-description` scenario "A route without steps falls back to the description lines": its
  code path is gone and that requirement is retired with `retire-instruction-prose`.
- Stacked on `client-java-live-route-steps` (#1921): the `onRouteSteps`/`onNextRouteStep` callbacks and
  `JavaScoutInstructions` this change consumes are added there. Retarget to master once #1921 merges.
