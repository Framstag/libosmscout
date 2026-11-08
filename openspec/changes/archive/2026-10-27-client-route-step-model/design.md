# Design

## Context

Evidence gathered while exploring (all verified in this tree):

- `libosmscout/include/osmscout/routing/RouteDescription.h` defines 20 typed `Description` kinds on a route
  node — including `MotorwayJunctionDescription` (`NameDescription`: name **and** exit `ref`),
  `DestinationDescription` (raw `destination` tag, semicolon separated), `MaxSpeedDescription`,
  `CrossingWaysDescription`, `LaneDescription`, `SuggestedLaneDescription`, `POIAtRouteDescription`,
  `ViaDescription`, `TypeNameDescription`, `NameChangedDescription`.
- The Java route path already registers all 14 postprocessors that populate them
  (`libosmscout-client-java/src/OSMScoutClient.cpp:6938-6953`), so the data exists in memory and is discarded
  at the client boundary.
- `RouteDescriptionPostprocessor::Callback` (core) is the existing visitor over that data: 12 hooks plus
  `BeforeNode`/`AfterNode`.
- Hook coverage today: Demos 12/12, Qt 10/12, Java route list 3/12, Java navigation 10/12.
- `RouteInstructionAgent<RouteInstruction, RouteInstructionBuilder>` is a template; its only requirements on
  the instruction type are default-constructibility, copyability and `GetDistance()`. Qt instantiates it with
  its own `osmscout::RouteStep` (`NavigationModule.h:164`), Java with `JavaRouteInstruction`
  (`OSMScoutClient.cpp:2929`).
- `libosmscout-client-qt` exports `osmscout::RouteStep` (QObject, `Q_PROPERTY`, `Q_ENUM Roles`,
  `data(int role)`). Qt clients exist outside this repository → the Qt type must not move, be renamed, or
  change behaviour in this step.
- Both `libosmscout-client-qt` (`CMakeLists.txt:143`) and `libosmscout-client-java`
  (`CMakeLists.txt:117-122`) already link `OSMScout::Client`.
- `JavaScout/src/main/java/com/framstag/libosmscout/RouteInstruction.java` is an unused textual parser
  (`parse(String)` splitting on `" ["`, turn keywords, `highway_` prefix); every use of `RouteInstruction` in
  `RoutePanel`/`MainController` imports the client class. It shadows that class by name.

Constraints carried in by the proposer: new data structure; complete (all information the description holds,
including `Node::objects[]` and the route's database mapping); language-agnostic (no translated text);
aligned with Qt but Qt untouched; both Java applications in scope, one of them out of tree; existing APIs
stay.

## Field inventory — the "complete" contract

One `RouteStep` record per described route node. Every member below maps 1:1 to a `RouteDescription` member;
nothing is derived that the description does not hold, except `kind` (see D3).

| Model member | Source |
|---|---|
| `databaseId`, `currentNodeIndex`, `pathObject`, `targetNodeIndex` | `RouteDescription::Node` |
| `coord`, `distanceFromStart`, `timeFromStart` | `Node::GetLocation/GetDistance/GetTime` |
| `objects[]` (`offset`, `type`) | `Node::GetObjects()` |
| `legDistance`, `legTime` | delta to previous step (D4) |
| `name{name,ref}` | `NameDescription` |
| `nameChanged{from{name,ref}, to{name,ref}}` | `NameChangedDescription` |
| `typeName` | `TypeNameDescription` |
| `crossingWays{exitCount, origin{name,ref}, target{name,ref}, others[]{name,ref}}` | `CrossingWaysDescription` |
| `direction{turnAngle, curveAngle, turn, curve}` | `DirectionDescription` |
| `turn` | `TurnDescription` |
| `roundaboutEnter{clockwise}` / `roundaboutLeave{exitCountTaken, clockwise}` | `RoundaboutEnter/LeaveDescription` |
| `motorwayEnter{to{name,ref}}` | `MotorwayEnterDescription` |
| `motorwayChange{from{name,ref}, to{name,ref}, move}` | `MotorwayChangeDescription` |
| `motorwayLeave{from{name,ref}, move}` | `MotorwayLeaveDescription` |
| `junction{name,ref}` | `MotorwayJunctionDescription` |
| `destination` (raw string) | `DestinationDescription` |
| `maxSpeed` (km/h) | `MaxSpeedDescription` |
| `lanes{oneway,count,turns[]}` / `suggestedLanes{from,to,turn}` | `LaneDescription`, `SuggestedLaneDescription` |
| `poi{databaseId, object, name{name,ref}, distance}` | `POIAtRouteDescription` |
| `via{sectionNumber, nodeCount}` | `ViaDescription` |
| `start{label}` / `target{label}` | `StartDescription`, `TargetDescription` |
| `databaseMapping{databaseId → directory}` | `RouteDescription::GetDatabaseMapping()` |
| `remainingDistance`, `remainingTime`, `nextNext` | live guidance only (D4) |

The raw `destination` string is kept as one string: splitting on `;` is the client's job (the Qt builder does
it today in `SplitDestinations`), and splitting in the model would lose the original text.

## D1 — Where the model lives

| Option | Rationale | Risk |
|---|---|---|
| **A. `libosmscout-client` (chosen)** | Both consumers already link it; Qt-free; it is the app-facing C++ layer; no new dependency edge; additive for ABI | New territory: that library has no routing code today; new files need CMake **and** Meson |
| B. `libosmscout` core | The data it projects already lives there; every backend reaches it | Blurs the engine/app boundary; core would own an app-facing view; larger public surface in a library with a documented file-format/ABI discipline |
| C. New small library | Clean ownership | New subproject, packaging, CI job, two build systems and a third dependency for Qt and Java — cost without a benefit over A |

Chosen A. Risk mitigation: the model header is the only new public header; the library keeps its existing
`namespace osmscout` conventions (nesting described in D2).

## D2 — Naming, so nothing shadows Qt's `osmscout::RouteStep`

| Option | Rationale | Risk |
|---|---|---|
| **A. `osmscout::client::RouteStep` (chosen)** | Same concept, same word, different scope. A translation unit that includes both headers cannot silently bind to the wrong type — unqualified use inside `namespace osmscout` resolves to Qt's, and reaching the new one requires `client::`. Verified: no `namespace client` exists anywhere in the tree | Slightly inconsistent with the library's flat `namespace osmscout`; the Qt adapter must qualify explicitly (which is the point) |
| B. `osmscout::RouteStepModel` | No collision, flat namespace kept | Reads like a Qt model class (Qt already has `RoutingListModel`); invites the "model = QAbstractItemModel" confusion; does not fail loudly if someone forgets |
| C. Top-level `namespace osmscoutclient` | Fully disjoint from `osmscout` | Inconsistent with the 18 existing `osmscout` headers in that library; `using namespace osmscout` no longer reaches anything, which surprises every caller |

Chosen A, with companions `osmscout::client::RouteSteps` (list + database mapping),
`osmscout::client::RouteStepCollector`, `osmscout::client::RouteStepBuilder`.

Java side: `com.framstag.libosmscout.client.RouteStep`. Verified: no Java `RouteStep` exists in the tree, and
JavaScout's app-package class that shadows the client type is deleted in this change, so no
`com.framstag.libosmscout.RouteInstruction` / `.client.RouteInstruction` duplication pattern repeats.

## D3 — Model shape

| Option | Rationale | Risk |
|---|---|---|
| **A. Record with one optional sub-record per description kind (chosen)** | "Contains all information" is checkable: one member per `Description` kind, plus `Node` fields. A node routinely carries `Turn` + `Direction` + `CrossingWays` + `Name` at once, so no information is forced into one slot | Wide record (~600-800 bytes/step); the collector must touch 20 kinds; a new kind added to core can be forgotten |
| B. `kind` enum + curated fields (today's Java/Qt shape) | Compact, easy on both sides of the boundary | Lossy by construction — this is precisely the defect being fixed; a second description on one node is silently dropped |
| C. Expose `RouteDescription` as it is | Zero conversion, no judgement calls | Leaks `std::unordered_map<std::string, DescriptionRef>` + `dynamic_pointer_cast` into every client and every language; JNI would need C++ RTTI across the boundary; no stable contract to spec |

Chosen A. `kind` still exists as a *derived* convenience member for icons, computed by the collector, never
used as storage. Risk mitigation for the forgotten kind: a Catch2 test asserts that the set of description
keys handled by the collector equals the set of `Key` constants in `RouteDescription.h`.

## D4 — Producers, and what navigation adds

| Option | Rationale | Risk |
|---|---|---|
| **A. Collector (whole route) + builder (navigation) in `libosmscout-client`, both over the same record (chosen)** | One model, two readers: the route list and the live guidance cannot drift again. The navigation builder satisfies the existing agent template unchanged | Two producers to keep consistent; the builder must fill `remaining*`/`nextNext` without changing the static fields |
| B. One producer used by both paths | Fewer moving parts | The navigation path needs progress and a "next next" hint that a whole-route list does not; one function would carry both concerns and a stop-window parameter — the API gets muddy and the list path pays for the nav path |
| C. Each client keeps its own walker (status quo) | No library work | Three divergent walkers today (Qt 681 lines, Java 388, Demos ~200); the drift *is* the defect |

Chosen A. Rules the producers guarantee:

- Exactly one step per route node that carries at least one description; none for undescribed nodes.
- Steps in route order. `legDistance`/`legTime` = difference to the previous step; the first step's leg is its
  own node's values (so the legs sum to the route totals).
- List steps carry absolute values only. The builder's step carries `remainingDistance`/`remainingTime` and,
  when the following manoeuvre is within the announcement window, `nextNext` as a full step.
- The builder's step satisfies the agent template contract (`GetDistance()` = distance from the route start).

## D5 — Java side: extend the existing instruction, add a step type

| Option | Rationale | Risk |
|---|---|---|
| **A. Add `RouteStep` DTO + `RouteEntry.steps`, extend `RouteInstruction` with the dropped facts (chosen)** | Existing members, constructors and text stay byte-identical; both apps migrate screen by screen; the two Java apps share one JAR, so one contract | Three overlapping Java types (`RouteEntry.descriptions`, `RouteStep`, `RouteInstruction`) coexist until the apps finish migrating |
| B. Replace `RouteInstruction` by `RouteStep` | One type in Java | Removes fields and constructors that in-flight changes and both apps rely on; violates "existing APIs stay"; the navigation DTO carries progress semantics a list step does not |
| C. Publish only the C++ model and let Java re-walk it | No JNI DTO work | Java cannot walk `Description` pointers; that is the whole problem |

Chosen A. `RouteInstruction` is the adapter target: the bridge fills its new fields from the step of the same
node, so `description`/`shortDescription` stay byte-identical (existing spec requirement) while the facts become
readable.

## D6 — Boundary encoding

| Option | Rationale | Risk |
|---|---|---|
| **A. Nested DTO objects, one per sub-record, absent ones `null` (chosen)** | "Absent" is distinguishable from "zero"; mirrors the model; Java code reads like the data | ~500-650 lines of mechanical JNI marshalling in an already 9.7k-line translation unit; a misordered field is a silent wrong value |
| B. Flat field list with sentinel values (Java's `routeInstruction*` style today) | Fewest Java classes | "Absent" and "zero" become indistinguishable — the same defect in a new place |
| C. Pass an opaque native handle and let Java call getters | No marshalling per field | Every access is a JNI call; lifetime tied to the route handle; the DTO cannot be used off the callback thread |

Chosen A. Mitigation: one JUnit test per sub-record asserting a known non-default value survives the boundary.
`objects[]`/`poi.object` cross as `long offset` + `int objectType` pairs (the `ObjectFileRef` content); the
database mapping crosses as `java.util.Map<Long,String>` on `RouteEntry`.

## Sequence — route calculation and the route dialog

```
JavaScout              JNI bridge (OSMScoutClient.cpp)      libosmscout-client          core libosmscout
    |                            |                                  |                          |
    | calculateRouteAsync(...)   |                                  |                          |
    |--------------------------->| TransformRouteDataToRouteDescription                        |
    |                            |--------------------------------------------------------->   |
    |                            | RoutePostprocessor::PostprocessRouteDescription (14 stages) |
    |                            |--------------------------------------------------------->   |
    |                            | RouteDescription (20 kinds populated)                       |
    |                            |-----------------------------> RouteStepCollector::Collect  |
    |                            |                                  (12 hooks -> 1 step/node) |
    |                            |<----------------------------- RouteSteps{steps, dbMapping}  |
    |                            |                                  |                          |
    |                            | marshal: RouteStep[] + descriptions + geometry              |
    | onSuccess(RouteEntry)      |                                  |                          |
    |<---------------------------|                                  |                          |
    |                            |                                  |                          |
    | render rows from steps     |                                  |                          |
    | (no text parsing)          |                                  |                          |
```

## Sequence — live guidance

```
GPS fix -> NavigationEngine -> DataAgent -> PositionAgent
                                      |
                                      v
   RouteInstructionAgent<client::RouteStep, client::RouteStepBuilder>
        |  builder: step ahead + remaining distance/time + nextNext
        v
   NextRouteInstructionsMessage<client::RouteStep>
        |
        v
   DispatchMessage (JNI) -> Java RouteInstruction
        (exit ref, junction, destinations, typeName, maxSpeed filled from the step)
```

## Risks

| # | Risk | Mitigation |
|---|---|---|
| 1 | Third walker (Qt 681, Java 388, new ~400 lines) drifts while Qt is untouched | The model is the contract; collector guarded by the "every Description kind handled" test; step 2 converts Qt's builder into an adapter |
| 2 | JNI marshalling errors are silent (no type checking across the boundary) | One JUnit assertion per sub-record; a step's static fields checked against the description line that describes the same manoeuvre |
| 3 | File collision: four in-flight changes touch `OSMScoutClient.cpp` and two of them the same requirements (`RouteEntry`, `RouteInstruction`) | Land after `client-java-route-instruction-metrics`; deltas are written against the post-change text |
| 4 | Wide record copies on every navigation tick | The builder returns one step per tick, not the list; measure if needed before optimising |
| 5 | Two build systems | New files in `libosmscout-client/CMakeLists.txt` and `meson.build`, both verified |
| 6 | JavaScout's existing monospace/columnar line requirement is stale relative to the code; rendering from steps changes row content | Existing line-list behaviour is preserved as the fallback path; the existing requirement is not rewritten here |

## Out of scope / follow-ups

- Qt: converting `RouteDescriptionBuilder` into an adapter over the model, and surfacing the new fields in
  `osmscout::RouteStep` (additive only). Recorded as the follow-up step.
- Removing the now-redundant `descriptions` text path from the Java API once both apps read steps.
- Any core (`libosmscout`) change: none is needed; the callback walk already exists there.
