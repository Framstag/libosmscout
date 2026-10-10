# Proposal

## Why

A render request does not state the display it is rendered for. The projection DPI is a value configured
once on the client and shared by every surface that renders from it, so one surface's frames are scaled for
another surface's density — a head-unit frame rendered at a phone's density is about 1.8× too zoomed, and
the reverse is equally wrong. A render caller has no way to ask for a frame at the density of the surface
it will be drawn on.

## What Changes

- A render request SHALL be able to state the physical DPI of the display the frame is rendered for, and
  SHALL project with that value for that request.
- A request that states no usable DPI SHALL render with the DPI configured on the client, so a caller that
  does not care about the density keeps the present behaviour and existing call sites keep compiling.
- The request's DPI SHALL be a property of that request and SHALL NOT change the client's configured DPI,
  so two surfaces of one client cannot affect each other's frames.
- The Java client's JNI implementation of the client-wide render-DPI setter SHALL be removed: every
  surface now states its own DPI per request, and a client-wide value that a second surface can change is
  the defect this change removes. This is not a Java source break: on master the setter has no declaration
  in `OSMScoutClient.java` and no Java source calls it — `scripts/check-jni-signatures.sh` reports it as
  one of six "dead" JNI functions that no Java caller can reach, and that count drops to five with the
  removal. The configured DPI (the builder's `withPhysicalDpi`) stays as the fallback for a request that
  states none.
- The Java client SHALL offer both the request-with-DPI form and a convenience form without it.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `map-rendering`: the native render entry points take the projection DPI of the frame as a request
  parameter and fall back to the client's configured DPI for a request that states none.

## Impact

Affected files and modules:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — both native render entry points take the DPI; the
  shared render body projects with it and falls back to the configured DPI for an unusable value; the JNI
  function of the client-wide DPI setter is removed (it has no Java declaration on master, so nothing on
  the Java side changes with it, and the signature check's dead-function report drops from six to five).
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the native
  declarations carry the DPI, and the previous argument lists become non-native forwarders that pass "no
  DPI", so existing callers keep compiling and the JNI-signature check stays satisfied.

No change to `libosmscout-map`: the render parameters already carry a DPI, and this change decides which
value a request puts there. No database, map or style file format change, so no `FileFormatVersion.md`
version bump applies. No new dependency.

Consumers: the Java client's callers (JavaScout, the Android client) and every other binding that renders
through `OSMScoutClient`; they keep compiling unchanged (the previous argument lists remain as non-native
forwarders), and a surface that wants its own density states the DPI in its render request.
