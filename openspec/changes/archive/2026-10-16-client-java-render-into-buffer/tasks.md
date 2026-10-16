# Tasks

## 1. Pixel layout header (spec: client-java-render-buffer)

- [x] 1.1 Add `libosmscout-client-java/src/frame_pixel_layout.h`: the two layouts (one word per pixel for
  array destinations, four bytes per pixel for bitmap storage), and a destination that clears a frame to
  opaque black and writes one pixel, each in the destination's own layout. Verify: the header includes no
  JNI and no libosmscout header, so it is host-testable.
- [x] 1.2 Document in the header why the write happens in one place and that each entry point states the
  layout its destination needs. Verify: a reader can tell which layout belongs to which destination without
  reading the bridge.

## 2. Shared render body and the buffer entry point (spec: client-java-render-buffer)

- [x] 2.1 In `libosmscout-client-java/src/OSMScoutClient.cpp`, extract the render body so it fills a
  `FrameDestination` and returns whether a frame was drawn, and make the allocating entry point use it with
  an array-backed destination. Verify: the allocating entry point's signature, result and layout are
  unchanged.
- [x] 2.2 Add the buffer entry point: validate the request and the buffer (direct, size at least
  width\*height\*4) before writing, then run the shared body with the caller's storage and an
  `RgbaBytes` destination. Verify: a rejected request reports false and never reports success.
- [x] 2.3 Verify the buffer path allocates no frame-sized storage of its own. Verify: no array or vector of
  viewport size appears on that path.

## 3. Java declaration (spec: client-java-render-buffer)

- [x] 3.1 Add the declaration and Javadoc of the new entry point to
  `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java`: the buffer's
  requirements (direct, at least `width * height * 4` bytes), the returned value, the layout the frame is
  written in, caller ownership, and that one buffer must not serve two renders at once.
- [x] 3.2 Run `scripts/check-jni-signatures.sh` and verify it passes. Verify: the new native declaration has
  a matching JNI function with the same argument list.

## 4. Unit tests (spec: client-java-render-buffer)

- [x] 4.1 Add `Tests/src/FramePixelLayoutTest.cpp`: cover that each layout writes a pixel in its own byte
  order and that reading a pixel back in the destination's layout returns the colour that was written.
- [x] 4.2 Cover the clear: an unpainted frame reads as opaque black in both layouts, in each layout's own
  byte sequence.
- [x] 4.3 Cover the two-layout seam: writing the same colour into both destinations and reading each back in
  its own layout yields the same red, green and blue; the case fails if one destination is written in the
  other's layout.
- [x] 4.4 Register the test in `Tests/CMakeLists.txt` and `Tests/meson.build` with the include path of
  `libosmscout-client-java/src`. Verify: both build systems build and run it.

## 5. Build and regression verification (spec: client-java-render-buffer)

- [x] 5.1 Build the CMake build with the Java client library and the tests, and verify it compiles without
  errors and without warnings from the touched files.
- [x] 5.2 Build the Meson build the same way and verify it compiles without errors.
- [x] 5.3 Run the new test through both build systems and verify every case passes.
- [x] 5.4 Run the Java client suite and the rest of the C++ suite and verify no existing test regresses.
- [x] 5.5 Verify on a device or host surface that a frame rendered through the buffer path shows the same
  colours as the array path (the channel-swap regression). Verify: this is the only level at which the JNI
  part can be checked; there is no host test for it.
  RUN 2026-10-16: done on the host surface the task allows, against the v27 database
  `maps/nordrhein-westfalen` — log `evidence/buffer-color-probe-nrw-2026-10-16.log`, probe source
  `evidence/BufferColorProbe.java`. `renderWithRouteAndPois` (array, 3072 int) and `renderInto` (direct
  ByteBuffer, capacity 12288 bytes) are compared pixel by pixel, both with the same 3-waypoint route
  overlay: `0` of `3072` pixels differ, channel for channel. Still unverified: a real Android/desktop
  surface that consumes the buffer (the ownership and GetDirectBufferAddress plumbing).
- [x] 5.6 Run `openspec validate "client-java-render-into-buffer" --strict` and verify the change validates.

## 6. Documentation and change hygiene

- [x] 6.1 Verify the pixel layouts are documented where a caller reads them (the new entry point's Javadoc)
  and where they are implemented (the header).
- [x] 6.2 Verify the diff touches only the files listed in the proposal's Impact section, and that the
  allocating entry point's behaviour is untouched.
- [x] 6.3 Verify the change builds on `client-java-render-dpi`: the new entry point carries the request's
  projection DPI under the rule of that change.

## Notes

- 5.5 was closed on 2026-10-16 on the host surface (see `verification.md`, "Host verification 2026-10-16
  (map-dependent items)"); the device-only half - a surface that consumes the direct buffer - stays
  unverified.
