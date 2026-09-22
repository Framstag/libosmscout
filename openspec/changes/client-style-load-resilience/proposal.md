# Proposal

## Why

A stylesheet that cannot be parsed currently leaves the affected database without a usable style
configuration on every path except an explicit runtime switch, and the render pipeline then proceeds
without one: an application using the client died with `Fatal signal 11 (SIGSEGV)` inside the style
lookup called from the map painter, preceded only by a warning that the stylesheet failed to load. A
malformed bundled stylesheet is therefore a process crash for every user of the application, with no
way for the application to react.

The client already detects the failure and already owns a runtime switching contract that keeps the
previous style on a failed switch; what is missing is that the same guarantee holds on every load
path, and that a total failure degrades instead of reaching the painter.

## What Changes

- A stylesheet that failed to load SHALL NOT become the active style. Whenever a style was active
  before the attempt, it SHALL remain in effect, on every load path: the explicit runtime switch, the
  style applied when a session starts, a style-flag change, the basemap's own stylesheet, and the
  stylesheet refresh.
- Every database SHALL always have a usable style configuration. When no stylesheet has been loaded
  successfully for a database yet, the client SHALL use a safe configuration that draws no content for
  that database and cannot fault, instead of leaving the database without a configuration.
- Rendering SHALL NEVER use a configuration produced by a failed load. The client SHALL NOT present a
  rejected or absent style configuration to the map painter, and a render performed while a database
  has no successfully loaded stylesheet SHALL complete without a native fault, drawing no content for
  that database while other databases keep rendering.
- The failure SHALL be reported to the caller on every load path (not only the explicit switch),
  including which style is active after the attempt, so an application can inform the user.
- The existing runtime switching contract (enumerate styles, switch, flags survive a switch, redraw
  notification) is extended, not replaced: a successful load behaves exactly as before.

Not **BREAKING**: no API is removed or renamed, and no successful path changes behaviour; only the
failure path stops faulting and starts being reported.

## Capabilities

### New Capabilities

None — the existing runtime style switching capability already owns the failure contract; it is
generalized rather than duplicated.

### Modified Capabilities

- `client-java-style-switching`: the "Switching to an unloadable stylesheet fails" requirement is
  generalized from an explicit switch to every stylesheet load path (session start, style-flag change,
  basemap stylesheet, stylesheet refresh), and two requirements are added: a database always has a
  usable style configuration (safe configuration when nothing loaded successfully yet) and rendering
  never uses a configuration from a failed load. The failure is reported to the caller on every path,
  together with the style that is active after the attempt.

## Impact

**Modules / files:**

- `libosmscout-client/include/osmscoutclient/DBInstance.h` and `DBThread.h` — the per-database style
  configuration lifecycle and the load outcome of a database.
- `libosmscout-client/src/osmscoutclient/DBInstance.cpp` and `DBThread.cpp` — which configuration is
  installed after a successful or failed load, on every load path.
- `libosmscout-client-java/src/OSMScoutClient.cpp` — the Java-facing style load and render entry
  points, so the load outcome and the active style are reported to the application and the render path
  never paints a database without a configuration.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — the Java API
  surface for the load outcome and the active style.
- `Tests/src/StyleLoadResilienceTest.cpp` (new) plus the `Tests/CMakeLists.txt` and `Tests/meson.build`
  registrations — the Catch2 client tests gain the failure paths (first-load failure, failure after a
  success, recovery, batch painting with a database on the safe configuration).
- No build-system, dependency, platform- or backend-specific change; the behaviour is in the
  platform-independent client, so every backend and both build systems are covered.

**Origin:**

- Extracted from the NaviVeylin fork of this repository (`naviveylin-local`), where the behaviour was
  first implemented; the change contains nothing NaviVeylin-specific and is intended to be
  independently upstreamable.

**Consumers:**

- A consuming Android application uses this client through its JNI bridge and gains the guarantee; the
  application-side reporting and message are a separate change in that repository, which depends on
  this one being committed and its client revision updated.
