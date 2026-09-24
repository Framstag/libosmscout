# Proposal

## Why

The client library now supports two basemap behaviours that applications cannot reach: rendering the basemap with its own stylesheet instead of the main map style, and changing the basemap directory while the application is running. Both were deliberately ported to the core client without their Java-facing part, so the Java client still uses the main style for the basemap and still needs a restart before a basemap that was installed or removed at runtime is picked up.

## What Changes

- Applications SHALL be able to name the stylesheet the basemap renders with, chosen from the configured stylesheets directory, when they create a client. When no basemap stylesheet is named, or the named one cannot be used, the basemap SHALL fall back to the main map style, and the basemap layer SHALL keep rendering.
- Applications SHALL be able to set the basemap directory on a running client. Setting an empty directory SHALL unload the basemap. The change SHALL take effect without restarting the application.
- The Java client SHALL expose both behaviours through its public API; no new Java source file is introduced, existing client and builder classes are extended.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `client-java-style-switching`: the client gains the ability to select the stylesheet used for the basemap, so that a basemap with its own type definitions is presented by its own stylesheet rather than the active map style.
- `basemap-loading`: the basemap directory becomes settable while the application runs, in addition to being supplied at client creation, so an installed or removed basemap takes effect without a restart.

## Impact

Affected modules and files:

- `libosmscout-client-java/src/OSMScoutClient.cpp` — client build path and a new client entry point.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClientBuilder.java` — builder configuration for the basemap.
- `libosmscout-client-java/java/com/framstag/libosmscout/client/OSMScoutClient.java` — client entry point.
- `libosmscout-client/include/osmscoutclient/DBThread.h`, `libosmscout-client/src/osmscoutclient/DBThread.cpp` — consumed, not changed: basemap stylesheet and runtime basemap directory support already exist there.
- `JavaScout/src/test/java/com/framstag/libosmscout/client/` — Java tests exercising the new client API.
- Consumers: JavaScout and other users of the Java client gain the two behaviours; the existing builder and client API stay source compatible (new members only, no signature or behaviour change for existing calls).
- No CMake/Meson javac source-list change is needed, because no new Java source file is added to `libosmscout-client-java`.
- Documentation: `libosmscout-client-java/AGENTS.md` if the client API summary needs updating.
