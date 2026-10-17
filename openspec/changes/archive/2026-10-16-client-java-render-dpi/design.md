# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: `renderWithRouteAndPois` builds a `MapParameter` and
  reads the projection DPI from the shared client settings (`Settings::GetMapDPI()`, default 96), so every
  surface that renders through one client projects with the same value. `setMapDpi(double)` sets that
  shared value; on master it exists only as a JNI function — `OSMScoutClient.java` declares no such method
  and no Java source calls it, so `scripts/check-jni-signatures.sh` reports it as dead bridge code.
- `projectToPixel(width, height, centerLat, centerLon, magnification, dpi, angle, lat, lon)` already takes
  the DPI per call, so the projection helper and the render entry points disagree about where the DPI
  comes from.
- `scripts/check-jni-signatures.sh` compares the Java native declarations with the JNI functions, so a
  parameter added on one side only is caught as bridge drift.

## Goals / Non-Goals

**Goals:**

- Make the projection DPI a property of the render request, so a frame depends only on the request that
  produced it.
- Keep existing call sites compiling and keep the previous behaviour for a request that states no DPI.
- Keep the JNI declaration and the JNI function in step (the signature check must pass).

**Non-Goals:**

- Changing `libosmscout-map`: `MapParameter` already carries the DPI.
- Changing the magnification, the tile level derivation or the viewport rules.
- Adding a per-surface DPI cache or a DPI change callback.
- Changing how icons and fonts are sized (that follows the projection DPI already).

## Decisions

**D1 — The DPI is a parameter of the render request.**
Both native render entry points take it; the shared render body projects with it and never reads the shared
settings value for a request that stated one.
Alternatives:
- *Keep the client-wide setter and let each surface call it before its render*: two surfaces of one client
  race, the last writer wins, and a frame can be produced between the two calls with the wrong density —
  which is the defect reported.
- *Derive the DPI inside the native side from the viewport size*: the viewport does not know the physical
  size of the display, and the same pixel size means different densities on a phone and a head unit.
- *Read the DPI from the platform at render time*: the native client is shared by both surfaces, so it
  cannot tell which one is calling.
Chosen because the caller is the only party that knows the surface, and the request is the natural carrier.

**D2 — An unusable DPI falls back to the configured value instead of rejecting the frame.**
A request that states an undefined or non-positive DPI projects with the client's configured DPI.
Alternatives:
- *Reject the request*: a convenience overload and a desktop caller that never heard of DPI would start
  returning `null`, which turns a scaling question into a rendering failure.
- *Assume a fixed 96*: silently wrong on any non-96 surface, and it discards a value the client was
  configured with.
Chosen because the parameter is additive: an explicit "no DPI stated" must mean exactly the previous
behaviour.

**D3 — The client-wide setter's JNI function is removed.**
Every surface states its own DPI, and the configured value stays as the fallback for a request that states
none. The setter is only a JNI function on master: no Java declaration names it and no Java source calls
it, so its removal changes no Java source — the signature check's dead-function report drops from six to
five. Alternatives:
- *Keep the function*: the value it writes is still shared and still changed by the last writer, so the
  second surface's frames keep depending on the first surface's calls; but it is the smaller diff. Nothing
  in the repository can reach it, so keeping it only preserves the defect for a caller that does not exist.
- *Keep it but ignore it after the first per-request DPI*: two mechanisms for one value, and the setter
  silently stops working.
Chosen because the shared value is the defect; the fallback keeps a client configured at build time
(`withPhysicalDpi`) usable. No Java source break follows from the removal.

**D4 — The previous Java argument lists stay as non-native forwarders.**
`render(w, h, lat, lon, angle, mag)` and the long overlay form forward with "no DPI", so Java callers and
tests keep compiling and the signature check still sees a declaration for every native function.
Alternatives:
- *Change the declarations and update every caller*: a wider diff in the Java sources and tests, and a
  source break for every consumer of the library.
- *Keep only native methods and let callers pass `NaN`*: forces every caller to know the sentinel.
Chosen because the sentinel is an implementation detail of the fallback and should not leak into every call
site.

## Sequence diagram

```
Surface A (phone)            OSMScoutClient (JNI)                     Surface B (head unit)
      |                              |                                        |
      | render(..., dpi=420) ------->| projection DPI = 420                   |
      |<-- frame projected at 420 ---|                                        |
      |                              |<---------- render(..., dpi=160) -------|
      |                              | projection DPI = 160                   |
      |                              |----------- frame projected at 160 ---->|
      | render(..., no DPI) -------->| dpi <= 0 -> configured DPI (fallback)  |
      |<-- frame at configured DPI --|                                        |
      |                              |   (no render request writes a shared value)
```

## Risks / Trade-offs

- *A caller that relied on a client-wide setter* → none can: the setter is a JNI function without a Java
  declaration, unreachable from Java, so no Java source breaks; a surface states the DPI in its render
  request now.
- *A caller that states no DPI gets the configured value, which may be the default 96* → it is the previous
  behaviour for a client nobody configured, so nothing regresses; a surface that wants its own density
  states it.
- *Two nearly identical render signatures (with and without DPI)* → the short forms are non-native
  forwarders, so there is exactly one JNI function per render entry point and the signature check keeps
  passing.
- *The DPI plumbing itself is hard to unit-test on the host* (it needs a database and a JNI environment) →
  the observable rule (fallback for an unusable value, request value used otherwise) is verified through
  the Java-level render tests and, where no map is available, by the existing skip convention; recorded as
  a gap in the tasks.

## Migration Plan

Java source-compatible: the previous render argument lists remain as non-native forwarders, and the
removed setter is a JNI function no Java source can call, so no caller has to change. A surface that wants
its own density states the DPI in its render calls. No persisted data, no format change. Rollback is a
revert of the commits, which restores the setter and the shared value.

## Open Questions

- Whether a caller should be able to configure the client-wide fallback DPI at runtime at all (a builder
  option would cover the common case): deferrable, and a separate change if a consumer needs it.
- Whether the DPI should also be stated per request for the projection helper's overlay uses: it already
  is, so no change.
