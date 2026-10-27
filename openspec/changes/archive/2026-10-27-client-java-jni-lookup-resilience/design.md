# Design

## Context

See `proposal.md` — Why. The relevant current state, on this change's base
(`client-java-roundabout-instruction-data`, PR #1888):

- `libosmscout-client-java/src/OSMScoutClient.cpp`, `GetNavigationListenerMethods`: the bridge caches
  every method id a navigation session needs — the eleven listener callbacks via `GetObjectClass` +
  `GetMethodID`, and five classes (`NavigationPosition`, `NavigationState`, `RouteInstruction`,
  `TurnType`, `LaneTurn`) via `FindClass`, each followed by `NewGlobalRef` and a constructor or static
  method id. Every class and method id lookup is a raw `env->FindClass` / `env->GetMethodID` /
  `env->GetStaticMethodID` call. Only some of the listener ids are checked at their use site; the rest are
  simply `nullptr` when the lookup failed.
- A failed `FindClass`, `GetMethodID` or `GetStaticMethodID` returns null **and leaves an exception
  pending**. While an exception is pending, the JVM does not return a failure from the next JNI call: it
  aborts the process (`JNI DETECTED ERROR IN APPLICATION: ... called with pending exception`). Every null
  guard between the failing lookup and the abort is therefore unreachable code — including the guard in
  `CreateJavaRouteInstruction` and the defensive shape of the dispatch.
- `DispatchRouteInstructions` allocates the array first, sized for the **input** instruction list, and
  fills in the instructions it could build; an instruction the bridge could not build leaves a null entry
  in the array (a missing constructor id or a missing `TurnType` method makes *every* instruction
  unbuildable, so the whole array is null).
- The base's own change is what makes the descriptor case concrete: `instructionCtor` is looked up with
  the 14-parameter roundabout descriptor, so a `.so`/jar mismatch is a descriptor that matches nothing.

This change is stacked on that base because it edits the same lookup (`instructionCtor`) and the comment
above it, and because that change's own comment points here for the dispatch and pending-exception
subject.

## Goals / Non-Goals

**Goals:**

- Every class, instance-method and static-method lookup the navigation bridge performs clears the
  exception a failed lookup leaves pending, so the lookup's null return is what the call site observes
  and the following JNI call proceeds.
- Keep the failure a refusal, not an error: a class the bridge cannot find and a descriptor that matches
  nothing both mean "this bridge cannot dispatch that message" and stay on the existing early-return
  path.
- Build every instruction before the delivered array exists, and dispatch nothing when none could be
  built, so the array never carries null entries.
- Change nothing on the success path: same ids cached, same callbacks dispatched with the same arguments.

**Non-Goals:**

- The two unguarded `GetFieldID` lookups in the route call (`vehicle`/`avoidTolls`/… and the
  routing-profile speed field) — the profile change `client-java-routing-profile-max-speed` owns the
  latter and the field it reads does not exist on this base.
- Any other lookup site in the file: the `RouteCallback` method ids, the `Vehicle.ordinal` id, the
  `LocationEntry` / `FavoriteLocation` / `DescriptionEntry` construction lookups. They produce the same
  shape of abort, but none of them is on the navigation-listener path this change is about.
- Any Java source change: this change adds no declaration and changes no signature, so no `.java` file is
  touched.
- Reporting or logging a mismatch to the caller: the observable outcome stays "no instruction list".
- Any database, map, style or file-format change: no `FileFormatVersion.md` bump applies and no
  dependency is added.

## Decisions

**D1 — Three small helpers that return the raw call's value and clear only on null.**
`FindClassOrClear`, `GetMethodOrClear` and `GetStaticMethodOrClear` each call the raw JNI lookup, and if
the result is null call `env->ExceptionClear()` before returning it.
Alternatives:
- *Call `env->ExceptionClear()` at each failing call site*: twenty duplicated clears, each easy to omit —
  which is exactly the defect (the id whose null nobody checked is the one that was never cleared).
- *One `ExceptionCheck()`/`ExceptionClear()` at the end of `GetNavigationListenerMethods`*: too late. The
  abort happens on the *next* lookup, i.e. before the end of the function is reached.
- *Clear unconditionally in the helper, whatever the lookup returned*: a successful lookup would then also
  discard any exception state, and the helper would no longer be "a lookup that reports its failure".
- *A boolean helper with the id as an out-parameter*: more call-site noise for no gain; the id itself is
  the value the call site uses, and `nullptr` already carries "it failed".
Chosen because it makes the failure mode of the lookup explicit at the point where the id is taken, and
leaves a successful lookup a pass-through.

**D2 — Every lookup inside `GetNavigationListenerMethods` is converted, none left raw.**
The eleven listener `GetMethodID` calls, the five `FindClass` calls and the six constructor / static
method ids taken on those classes.
Alternatives:
- *Convert only the lookups whose result is guarded downstream*: the abort lands on the next lookup,
  whichever it is, so a partially converted function still aborts — and the unguarded listener ids are
  precisely the ones whose failure was never handled.
- *Convert only `instructionCtor`* (the measured failure): fixes the reported crash and leaves the same
  defect for a missing `TurnType`, `LaneTurn` or listener method.
- *Convert all lookups in the file*: wider than this unit and touches the routing-profile path another
  change owns.
Chosen because the function is one unit of "resolve everything the listener dispatch needs", and a raw
lookup left inside it keeps the whole function abort-prone.

**D3 — The instruction dispatch builds first, allocates second, and returns early when it built
nothing.**
`DispatchRouteInstructions` collects the instructions `CreateJavaRouteInstruction` could build into a
`std::vector<jobject>` (reserved to `instructions.size()`), returns before allocating when that vector is
empty, and only then allocates an array of `built.size()` elements and fills it, deleting each local ref.
Alternatives:
- *Pre-check the cached method ids (`instructionCtor`/`turnTypeValueOf`/globals) and return early*: covers
  the constructor-descriptor case but not an instruction that fails on its own, and duplicates the guard
  that already lives in `CreateJavaRouteInstruction`.
- *Keep the input-sized array and skip the nulls* (the base's shape): the array keeps its null entries and
  the listener dereferences them far from here — the defect this hunk fixes.
- *Dispatch an empty array instead of not dispatching*: an empty array is the legitimate value for "the
  route has no instructions", so sending it would claim the route has none instead of "the bridge could
  not build them". Not calling the callback is distinguishable from an empty list.
- *Abort the whole message dispatch (return false and stop) when an instruction is unbuildable*: a single
  unbuildable instruction would then also suppress the position, speed and lane callbacks of the same
  message.
Chosen because it makes the array a faithful image of what the bridge could build, and because the
"could build nothing" case is answered by not calling the callback at all.

**D4 — The comment above `instructionCtor` states the reachability the dispatch change creates.**
The base's comment says the descriptor must match the full Java constructor; this change adds that a
mismatch leaves `instructionCtor` null, which `CreateJavaRouteInstruction` reports by returning null and
`DispatchRouteInstructions` answers by sending no instructions, and that this path is only reachable
because the lookup clears its own pending exception.
Alternatives:
- *Leave the base's two-line comment*: it would keep describing `DispatchRouteInstructions` as "sending no
  instructions" — behaviour that does not exist until this change lands, and states nothing about why the
  guard is reachable.
- *Put the explanation only in this change's documents*: the next reader of the file is the one who needs
  it, and the file is where a descriptor mismatch will be debugged.
Chosen because D1 and D3 make exactly that sentence true, and it is the sibling's clause completed rather
than replaced.

**D5 — The defect is not reproducible on the host, so the change is verified by build, signature check
and the C++ suite.**
The helpers wrap `JNIEnv`, and a pending exception only aborts a process with a live JVM. The evidence
that the defect is real is the base reference's measurement, taken on the branch (2026-10-14,
emulator-5554) with the constructor descriptor mutated: before the change the abort landed on the
following `FindClass` for `TurnType` and the instrumentation reported "Process crashed"; after it the same
mutation delivers no instruction list.
Alternatives:
- *A host unit test over the helpers*: there is no JNI environment in the test process; a fake `JNIEnv`
  would test the fake, and the behaviour under test (the abort) cannot occur there.
- *An instrumented test on this machine*: no emulator, no device, and `maps/` holds format v26 while the
  library expects v27, so no navigation session can be started at all.
Chosen because it is honest about coverage, and because the parts that *can* be checked here (compiles,
no signature change, no regression in the suite) are checked.

## Sequence / data flow

```
startNavigation (Java)
        |
        v
GetNavigationListenerMethods(env, listener, methods)
        |
        +-- FindClassOrClear  ---> jclass  | null  (no exception pending)
        |        +-- NewGlobalRef
        |        +-- GetMethodOrClear / GetStaticMethodOrClear ---> jmethodID | null
        +-- GetMethodOrClear(listenerCls, ...) ---> jmethodID | null
        |
        v
NavigationListenerMethods (cached ids; a null id means "this bridge cannot use that lookup")
        |
        v
DispatchMessage  --> DispatchRouteInstructions(env, instructions)
        |
        +-- for each instruction: CreateJavaRouteInstruction ---> jobject | null
        +-- built = the non-null ones, in their original order
        +-- built.empty() ---> return (no callback, no array)
        +-- arr = NewObjectArray(built.size()); set elements; DeleteLocalRef each
        +-- CallVoidMethod(listener, onRouteInstructions, arr); DeleteLocalRef(arr)
```

## Risks / Trade-offs

- *A lookup that used to succeed now returns null* → the wrappers return exactly what the raw call
  returned; the only statement added on the success path is a null test, and the ids are taken from the
  same `JNIEnv` with the same arguments. This is the regression the change cannot test here (D5), and it
  is named in the PR body.
- *The dispatch now sends a shorter list than the input for a partially buildable message* → that is the
  intended contract (spec: "A partially buildable list has no null entries"); a fully buildable list keeps
  its length and order, and a message with nothing buildable is observable as a missing callback rather
  than a list of nulls.
- *An exception a caller would have wanted to see is discarded* → before this change that exception ended
  the process; the null id is the report, and refusal is the documented degradation.
- *The wrapper names say "OrClear", which hides a side effect behind a lookup name* → the comment above
  them states the side effect and why it is necessary, and the alternative (call-site clears) is what
  failed before.
- *The device-level behaviour stays unverified here* → recorded as NOT RUN in `verification.md`, with the
  branch's emulator measurement cited as the evidence for the defect, clearly labelled as not taken here.

## Migration Plan

Pure C++ change inside one translation unit: three static helpers, the lookups in one function converted
to them, and one dispatch function restructured. No JNI entry point, Java declaration, constructor
descriptor, database, map, style or file format changes, so nothing has to be re-imported, no jar has to
be rebuilt for a Java consumer, and no caller has to change. A client that runs a matching `.so` and jar
sees identical behaviour; a client whose two sides differ now loses the instruction list instead of
aborting. Rollback is a revert of the commit, which restores the raw lookups and the input-sized array.

## Open Questions

- Should the same wrappers be applied to the other lookup sites in this file (the `RouteCallback` ids, the
  `Vehicle.ordinal` id, the construction lookups of `LocationEntry`, `FavoriteLocation`,
  `DescriptionEntry`)? They can abort the same way; it is a wider change than this unit, and none of them
  is on the navigation-listener path.
- Should the bridge log a mismatch once (a `log.Warn()` naming the class or descriptor) instead of
  degrading silently? It would help diagnose a `.so`/jar upgrade in progress, and it is a behaviour
  change a consumer could observe.
- Should the emulator mutation measurement be repeated against the final commit here (it needs an
  emulator and a matching test app, neither of which is on this machine)?
