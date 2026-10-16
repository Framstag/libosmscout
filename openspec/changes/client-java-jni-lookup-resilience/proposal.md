# Proposal

## Why

The bridge caches the Java method ids it needs when a navigation session starts. `FindClass`, `GetMethodID`
and `GetStaticMethodID` report a failure by returning null **and leaving an exception pending** — and while
one is pending, the next JNI call does not fail, it aborts the process:

```
JNI DETECTED ERROR IN APPLICATION: JNI FindClass called with pending exception
java.lang.NoSuchMethodError: no non-static method "...RouteInstruction;.<init>(...)"
```

Every null guard downstream is therefore unreachable, and the bridge's own defensive design is a fiction: a
`RouteInstruction` constructor descriptor that does not match the Java class in the application (a newer
`.so` with an older jar, or the reverse) crashes at bridge init instead of degrading. Measured on an emulator
(2026-10-14): with the constructor descriptor mutated to match no constructor, the abort landed on the
*next* lookup — the `FindClass` for `TurnType` — and the instrumentation run reported "Process crashed", not
a failed case.

The dispatch has the same shape of defect: it allocates an array sized for the input instructions and fills
in the ones it could build, so an instruction that could not be built becomes a null entry a listener
dereferences far from its cause.

## What Changes

- Every class, instance-method and static-method lookup the navigation bridge performs SHALL clear the
  exception a failed lookup leaves pending, so that the lookup's null return is what the call site sees and
  the process does not abort on the following JNI call.
- The failure SHALL stay a refusal, not an error: a class the bridge cannot find and a descriptor that
  matches nothing both mean "this bridge cannot dispatch that message", which the bridge already handles by
  returning early.
- The instruction dispatch SHALL build every instruction before it allocates the array it delivers and SHALL
  dispatch nothing when it could build none, so the array never carries null entries.

## Capabilities

### New Capabilities

- `client-java-jni-method-lookup`: how the Java client's JNI bridge resolves the classes and methods it
  calls, and what a mismatch between the bridge and the Java classes it talks to degrades into.

### Modified Capabilities

- None.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — three small lookup wrappers
  (`FindClassOrClear`, `GetMethodOrClear`, `GetStaticMethodOrClear`), every lookup in
  `GetNavigationListenerMethods` converted to them, and `DispatchRouteInstructions` changed to build all
  instructions before it allocates the delivered array.
- `openspec/changes/client-java-jni-lookup-resilience/` — this change.

No database, map or style file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No JNI signature change, no Java source change, no behaviour change on the success path: with
every class and descriptor matching, the same ids are cached and the same callbacks are dispatched.

Stacked on `client-java-roundabout-instruction-data`: both changes edit the same `instructionCtor` descriptor
and the comment that explains it, and this one's text assumes the roundabout fields are in the Java
constructor. Retarget this PR to `master` once that one is merged.

Out of scope: the two remaining unguarded `GetFieldID` lookups in the route call are the routing-profile
change's subject, not this one's.

Consumers: any application whose native bridge and Java classes are not exactly the same build — that is, a
ship upgrade in progress — sees a missing instruction list instead of a process abort.
