# Proposal

## Why

libosmscout's route description already carries everything a navigation client needs — exit number and
junction name, the destination sign list, way type, crossing ways, max speed, lanes, POIs along the route —
but nothing hands it to a client as data. The Java bridge implements only 3 of the 12 route-description hooks
for the route list, and discards the junction and destination descriptions on the navigation path, so both
Java applications must either reconstruct routing facts from prose that does not contain them, or go without.
Each client that wants them re-walks the C++ description in its own language with its own wording, and the
walks already disagree: Qt's route step drops two of the twelve hooks, the Java list drops nine.

## What Changes

- libosmscout-client gains a route step model: one record per manoeuvre that carries every description the
  route description holds for that node, plus the node's identity and its position along the route. It is
  free of presentation and of translated text, so a client renders and translates it itself.
- Two producers ship with the model: one that turns a whole route description into a list of steps (route
  visualization), and one that produces the step ahead of the vehicle with its remaining distance and time
  (live guidance). Both read the same model, so the route list and the live guidance can no longer describe
  the same manoeuvre differently.
- The Java client library exposes the model: route results gain a structured step list next to the existing
  description lines, and the navigation instruction gains the facts it drops today. Every existing Java
  member, constructor and behaviour is kept, so both Java applications can migrate one screen at a time.
- JavaScout renders the route dialog from the structured steps and loses the textual parser it carries for
  that purpose but does not use.
- The Qt client library is not changed in this step: its route step type keeps its names and its behaviour
  for clients outside this repository. The new types are named so that they cannot be mistaken for it.

## Capabilities

### New Capabilities
- `route-step-model`: the language-agnostic, complete per-manoeuvre route step model in the client library —
  what it carries, what a producer must guarantee, and how it reaches a non-C++ client without translated text.

### Modified Capabilities
- `route-calculation`: a calculated route also publishes its steps as structured data, not only as
  formatted description lines.
- `turn-by-turn-instructions`: a navigation instruction carries the manoeuvre facts the route description
  holds (junction and exit, destination, way type, max speed) instead of only prose.
- `route-description`: JavaScout's route description list renders from structured steps and therefore shows
  manoeuvres it currently omits.

## Impact

- `libosmscout-client/`: new model and producers under `include/osmscoutclient/` and `src/osmscoutclient/`;
  both `CMakeLists.txt` and `meson.build` (the repository keeps both build systems in parallel).
- `libosmscout-client-java/`: `src/OSMScoutClient.cpp` (JNI projection of the steps and of the extended
  instruction; the file already carries four in-flight changes), `java/com/framstag/libosmscout/client/`
  (`RouteStep.java` new, `RouteEntry.java`, `RouteInstruction.java`, `NavigationListener.java` unchanged).
- `JavaScout/`: `RoutePanel.java` (route dialog renders from steps), `MainController.java` (navigation
  overlay reads the extended instruction), `RouteInstruction.java` in the app package deleted — it is an
  unused textual parser whose name shadows the client class.
- `Tests/`: Catch2 coverage for the model and its producers, including a guard that a route description
  description kind which no producer handles fails the test.
- `openspec/specs/`: `route-step-model` new; `route-calculation`, `turn-by-turn-instructions`,
  `route-description` amended.
- Unchanged: `libosmscout` core, `libosmscout-client-qt` and its exported route step type, `Java/` examples,
  the Android and Apple bindings.
- Depends on: `client-java-route-instruction-metrics` (touches the same `RouteEntry` and `RouteInstruction`
  requirements and the same JNI translation unit) — this change lands after it.
