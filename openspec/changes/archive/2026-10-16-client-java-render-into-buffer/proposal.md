# Proposal

## Why

Every render allocates a frame-sized pixel array inside the bridge and hands it to Java, so both sides hold
a complete frame at once and the peak grows with the viewport. A caller that already owns pixel storage — an
Android bitmap, a reused buffer — cannot ask the bridge to write into it, and has to copy the array into
that storage itself, keeping a second full frame alive.

## What Changes

- The Java client SHALL offer a render entry point that writes the frame into pixel storage the caller
  provides, and SHALL allocate no frame-sized storage of its own on that path.
- The entry point SHALL report, as a value, whether a frame was written, and SHALL answer an unusable
  request without faulting.
- Each render destination SHALL receive the frame in the pixel layout that destination reads, so writing
  one destination's layout into another SHALL be impossible by construction.
- Both render destinations SHALL produce the same frame for the same request.
- An area the style leaves unpainted SHALL be opaque black in every destination.
- The caller SHALL own the storage it provides: the bridge SHALL retain no reference to it after the call.

## Capabilities

### New Capabilities

- `client-java-render-buffer`: rendering into pixel storage the caller owns, the result a caller gets, and
  the pixel layout each render destination of the Java client receives its frame in.

### Modified Capabilities

<!-- None: the existing allocating render entry point keeps its signature, its result and its layout. The
     pixel layout of both destinations is pinned here for the first time, not changed. -->

## Impact

Affected files and modules:

- `libosmscout-client-java/src/frame_pixel_layout.h` — new dependency-free header: the pixel layouts, a
  destination that clears and writes a frame in one layout, and nothing else.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — one shared render body that fills a destination, the
  allocating entry point that reads it out, and the new buffer entry point.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the declaration and
  Javadoc of the new entry point.
- `Tests/src/FramePixelLayoutTest.cpp` — new host test for the layouts and the destination.
- `Tests/CMakeLists.txt`, `Tests/meson.build` — register the test in both build systems.

No database, map, style or file format change, so no `FileFormatVersion.md` version bump applies. No new
dependency. No existing signature changes. The native side only uses what the JNI environment provides
(direct-buffer access), not a new platform API.

Depends on `client-java-render-dpi`: the new entry point carries the request's projection DPI, so that
change is applied first or the two are reviewed together.
