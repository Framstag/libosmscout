# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: `renderWithRouteAndPois` builds the render parameters,
  renders through `MapPainterCairo` into a Cairo image surface, then walks the surface and builds a
  `jintArray` of `0xAARRGGBB` words for Java. The frame therefore exists at least twice at the end of a
  render (Cairo surface plus the Java array), and a caller that wants the pixels elsewhere copies them
  again.
- Cairo's `CAIRO_FORMAT_RGB24` carries its own byte order (B, G, R, X), so the conversion to the array's
  layout already happens in the bridge; there is no shared place that states a layout.
- The JNI environment can hand out the address of a direct `java.nio.ByteBuffer`, so the bridge can write
  the caller's storage itself.
- The Java client's render path is shared by a desktop surface (array form) and an Android surface (bitmap),
  and the Android surface showed a red/blue channel swap once, because the byte buffer path wrote the
  array's layout into the bitmap's storage.

## Goals / Non-Goals

**Goals:**

- Let a caller render without the bridge allocating a frame-sized array, so the peak memory of a render does
  not grow by one full frame.
- Make the pixel layout of each destination a stated property of that destination, written in one place.
- Keep the existing entry point's signature, behaviour and result exactly as they are.

**Non-Goals:**

- Removing the allocating entry point or changing its signature.
- Rendering into a file, a Java array other than the existing one, or a memory-mapped region.
- Changing the Cairo surface format, the viewport rules or the overlay drawing.
- Adding a Java-level buffer pool or buffer lifecycle management in the library.

## Decisions

**D1 — One shared render body; the destinations differ only in the destination.**
A single function renders the frame into a `FrameDestination` (pointer plus layout); the allocating entry
point passes an array-backed destination, the buffer entry point passes the caller's storage.
Alternatives:
- *Duplicate the render body*: two renderings could drift apart, and the change's own contract says the two
  destinations produce the same frame.
- *Render into the Cairo surface and convert per destination afterwards*: keeps a full intermediate copy,
  which is exactly the cost the change removes, and it is where the channel swap came from.
Chosen because one body makes "same request, same frame" structural rather than a promise.

**D2 — The layout is stated per destination and written in one place.**
A dependency-free header defines the two layouts and a destination that clears and writes a frame in its own
layout; the rest of the bridge computes colour components only.
Alternatives:
- *Keep the conversion in the pixel loops and document the layouts*: documentation did not prevent the
  channel swap; the loop is where the mistake is made.
- *Convert the array destination's frame into the buffer destination's layout*: an extra pass and another
  place that could swap channels.
- *Make the buffer path reuse the existing array*: that is the allocation the change removes.
Chosen because a single write point per layout makes a swap impossible rather than unlikely, and it is
host-testable without JNI.

**D3 — The buffer entry point requires storage it can write directly and of the right size.**
A missing, non-writable or too-small buffer is a rejected request, reported as false.
Alternatives:
- *Copy through an intermediate array for a non-direct buffer*: reintroduces the allocation and the copy,
  and silently changes the performance contract.
- *Grow or allocate storage on the caller's behalf*: contradicts "caller-owned" and hides misuse.
Chosen because the caller that wants the buffer path also owns the storage and its size; a clear false is
better than a hidden allocation.

**D4 — Success is a value; a failure never faults.**
The buffer entry point returns a boolean, and the JNI side checks every precondition before it writes.
Alternatives:
- *Throw a Java exception*: the repository reports failures as values, and the buffer entry point's request
  is invalid-parameter driven, not exceptional.
- *Return the pixel data as well*: collides with the "no allocation" property of the path.
Chosen because the caller needs one bit — was a frame written — and everything else is its own misuse.

**D5 — The caller owns the storage; the bridge writes it only during the call.**
The destination holds a pointer for the duration of the call, and the bridge keeps no reference.
Alternatives:
- *Retain and reuse the buffer*: the library would suddenly own the caller's memory and its lifetime.
- *Guard the buffer with a lock*: the caller's storage is the caller's to serialise; the bridge cannot see
  two calls that pass the same storage from different threads.
Chosen because ownership must stay with the caller, and the Javadoc states that one buffer must not serve
two renders at once.

## Sequence diagram

```
Java caller                    OSMScoutClient (JNI)                  shared render body
    |                                 |                                     |
    | renderInto(w,h,..., dpi, buf) ->| check buffer: direct? size >= w*h*4?|
    |                                 | check request: client, db thread,   |
    |                                 |   viewport, magnification           |
    |                                 |--- FrameDestination{addr, RgbaBytes}|
    |                                 |                                     | clear (opaque black
    |                                 |                                     |   in this layout)
    |                                 |                                     | render via Cairo
    |                                 |                                     | write each pixel once,
    |                                 |                                     |   in this layout
    |<-- true (no allocation) --------|                                     |
    |                                 |                                     |
    | render(w,h,...) --------------->| allocate int[]; destination {arr, ArgbInt}
    |                                 |------------------------------------>| same body
    |<-- int[] ARGB ------------------|                                     |
```

## Risks / Trade-offs

- *The channel swap recurs in a future destination* → the layout is written in one place, the layouts are
  host-tested, and a test compares the same request across both destinations.
- *A caller passes the same buffer to two renders at once* → the bridge cannot guard the caller's storage;
  the Javadoc states it, and a concurrent write is a caller-side misuse.
- *A caller passes a non-direct buffer and expects a copy* → the Javadoc states the requirement and the
  result is false, so no silent allocation happens.
- *The buffer path needs a viewport-sized buffer, so a mismatch is a rejected frame* → the required size is
  stated in the declaration and the check is before any write.
- *The render body now has two callers with two lifetime models* → both pass a value destination that only
  lives for the call; no ownership escapes the function.
- *The buffer entry point's JNI part is not host-testable* → the layouts and the destination are host-tested,
  and the JNI part is argument marshalling plus one precondition check; the pixel contract is covered by the
  comparison test.

## Migration Plan

Pure addition: the allocating entry point keeps its signature, its result and its layout, and no data is
persisted. A caller adopts the buffer path by allocating its storage and calling the new entry point;
rollback is a revert of the commits, which removes the new entry point and leaves the allocating path.

## Open Questions

- Whether the library should also accept a caller-supplied array (not only a direct buffer): deferrable, no
  current caller needs it.
- Whether the two destinations should be compared in a test that needs no database: deferrable, it would
  need a fake renderer for the shared body; the layout comparison is the failing part today.
