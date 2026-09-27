# Design

## Context

See `proposal.md` for motivation and `specs/` for the behaviour contract.

Facts that shape the approach (verified on `master` @ 791d39743):

- `libosmscout-client/src/osmscoutclient/DBThread.cpp` already carries both mechanisms, ported from `naviveylin-local` by commit `e0695d011` ("Port from naviveylin-local (PR #1773), non-Java part."):
  - the constructor parameter `basemapStyleFilename` (member used in `LoadBasemap()` via `makeStyleConfig(typeConfig, false, basemapStyleFilename)`; `makeStyleConfig` uses the path verbatim, no extension is appended),
  - `DBThread::SetBasemapLookupDirectory(dir)`, an asynchronous `CancelableFuture<bool>` that replaces the directory and reloads through the existing `LoadBasemap()` reopen path.
- Neither is reachable: no caller passes a non-empty `basemapStyleFilename`, and nothing calls `SetBasemapLookupDirectory`.
- `libosmscout-client-java/src/OSMScoutClient.cpp` (single ~4.2k-line translation unit) reads builder fields with `GetFieldID` and constructs `DBThread` with five arguments. `OSMScoutClientBuilder.java` has no basemap stylesheet member, and `OSMScoutClient.java` has no basemap directory setter.
- `Java_..._loadStyleSheet` already implements the whole stylesheet-name policy: rejects empty / `/` / `\` / `.` / `..`, strips a trailing `.oss`, resolves against `Settings::GetStyleSheetDirectory()` (which supplies the `stylesheets` default when the builder left it unset), and checks `std::filesystem::exists`.
- `LoadBasemap()` semantics already match the spec: empty directory unloads, a basemap whose stylesheet is rejected falls to the empty style configuration (basemap layer not drawn) and sets `lastStyleLoadSucceeded=false`, which `wasLastStyleLoadSuccessful()` already exposes to Java.
- Reference implementation on `origin/naviveylin-local` (commits `c3d90a178` / `ea9f684f2` for the stylesheet, `ef143a2c7` for the directory) is not directly portable: it resolves the basemap stylesheet against the raw builder string instead of `Settings`, skips the existence check, and reads a Java field `basemapStyleSheet` that exists in no branch.

## Goals / Non-Goals

**Goals:**

- Make both already-ported core behaviours reachable from the Java client, with the resolution and validation policy shared with the existing stylesheet API instead of duplicated.
- Keep the change additive and small: two existing Java classes gain members, one C++ translation unit gains one resolution helper plus one entry point.

**Non-Goals:**

- No core (`libosmscout-client`) change; the ported APIs are used as they are.
- No runtime switch of the *basemap stylesheet* (only the directory is settable at runtime).
- No new Java source file, so no javac source-list change, no packaging change.
- No new observable API for reading back the configured basemap stylesheet (see Risks).
- Not the other `naviveylin-local` leftovers (route-instruction distances, route/cycle stylesheets).

## Decisions

### D1: Basemap stylesheet is selected on the builder, not at runtime

Chosen: a `basemapStyleSheet` member on `OSMScoutClientBuilder` with `withBasemapStyleSheet(String)`, read in `build()`.

Alternatives:
1. A runtime `OSMScoutClient.setBasemapStyleSheet(String)` next to `loadStyleSheet` — rejected because the core basemap stylesheet is a constructor parameter consumed when the basemap opens; a runtime switch would need a new core setter and a reload path for the basemap style, i.e. more surface for the same goal.
2. Reusing the existing `loadStyleSheet(String)` with a flag such as "apply to basemap" — rejected: it conflates the active map style with the basemap style, and the spec requires the map style to stay unchanged.
3. A stylesheet *file name* instead of a style *name* — rejected: it would bypass the stylesheets directory policy and allow paths outside it.

Reasoning: the basemap stylesheet must be known before `DBThread` is constructed, and `build()` is the point where `Settings` (and therefore the stylesheet directory) already exists.

### D2: Runtime basemap directory is fire-and-forget, observed through existing state

Chosen: `native void setBasemapLookupDirectory(String)`; the JNI entry point calls `DBThread::SetBasemapLookupDirectory` and returns. Whether a basemap is present afterwards is observable from the render and from `wasLastStyleLoadSuccessful()`.

Alternatives:
1. `native boolean` that waits on the `CancelableFuture<bool>` (`StdFuture().get()`) — rejected: the reload opens a database on the worker thread; blocking the caller (a UI thread in JavaScout) for a database open is exactly what the asynchronous core API avoids. `loadStyleSheet` blocks, but a style switch is a user-visible action that must finish before the redraw it triggers.
2. Reporting completion asynchronously through the existing listener/callback machinery — rejected: the client has no general-purpose completion channel, and adding one is out of proportion for this goal.
3. Java-side polling of a basemap-presence getter — rejected: it pushes the waiting problem into every consumer.

Reasoning: matches the sibling setters `setStyleSheetFlag` and `setNativeDataCacheSize`, which are `void` and apply asynchronously on the DB thread.

### D3: Stylesheet name resolution and validation is shared with `loadStyleSheet`

Chosen: extract the existing policy from `Java_..._loadStyleSheet` into one file-local helper in `OSMScoutClient.cpp` — input: stylesheet directory and name; output: absolute stylesheet path, or empty when the name is not a plain style name or no matching file exists — and use it from both `loadStyleSheet` and `build()`.

Alternatives:
1. Duplicate the validation in `build()` (the `naviveylin-local` shape) — rejected: two copies of the same policy drift, and the existing copy already encodes a security-relevant rule (no path traversal).
2. Resolve in Java and pass an absolute path — rejected: it moves the traversal rule and the `stylesheets` default into Java, where the rest of the stylesheet API does not live, and an absolute path from Java would bypass the directory policy entirely.
3. Keep the helper in a header for reuse outside this translation unit — rejected: no other consumer exists.

Reasoning: the spec's scenarios for the basemap stylesheet are the same rules the stylesheet API already applies, so one implementation keeps them honest. The existence check is what makes "name with no matching stylesheet" fall back to the main style rather than producing an unresolvable path.

### D4: A resolved but unloadable basemap stylesheet keeps the existing core behaviour

Chosen: no fallback to the main style at load time. A basemap stylesheet that exists but cannot be parsed leaves the basemap without a style configuration: the basemap layer is not drawn, map rendering continues, and `wasLastStyleLoadSuccessful()` reports the failure.

Alternatives:
1. Fall back to the main map style for the basemap — rejected for this change: it changes core `LoadBasemap()` behaviour that `client-java-style-switching` already specifies ("never uses a style configuration from a failed load") and would widen the change beyond the JNI boundary. Recorded as a possible follow-up.
2. Fail the client creation — rejected: a basemap is optional and must never prevent a usable client.

## Flows

Client creation with a basemap stylesheet:

```
Java                        JNI build()                  DBThread / core
----                        -----------                  ---------------
builder
  .withBasemapLookupDirectory(dir)
  .withBasemapStyleSheet("basemap-render")
  .build()  ----------------->
                            GetFieldID(basemapStyleSheet)
                            GetFieldID(stylesheetDirectory)
                            settings = Settings(...)
                            dir = settings->GetStyleSheetDirectory()
                            ResolveStyleSheetPath(dir, name)
                                 +--> invalid / no file --> ""
                            DBThread(dir, iconDir, settings,
                                     mapManager, customPoiTypes,
                                     basemapStyleFilename) ---->
                                                          LoadBasemap()
                                                            makeStyleConfig(
                                                              basemapTypeConfig,
                                                              false,
                                                              basemapStyleFilename)
                            <---- client object
```

Runtime basemap directory change:

```
Java                          JNI                            DBThread
----                          ---                            --------
client.setBasemapLookupDirectory(dir)
    ------------------------->
                              getClientData() null check
                              dbThread->SetBasemapLookupDirectory(dir)
                                                                |
                                                                v
                                                          Async job:
                                                            basemapLookupDirectory = dir
                                                            LoadBasemap()
                                                              empty dir -> unload
                                                              else open + load style
                                                              lastStyleLoadSucceeded
                                                                + basemapLoadedSignal
    <--- returns immediately
```

## Risks / Trade-offs

- [JNI/Java declaration mismatch is not caught at compile time: the JNI implementation is plain `extern "C"` and does not include the generated headers, while the CMake/Meson build generates JNI headers from the Java sources] → Mitigation: add the Java members and the C++ entry point in the same change; the JNI symbol name and signature must match the generated header exactly; a Java test that builds a client with a basemap stylesheet and calls the setter turns a mismatch into a visible failure.
- [`GetFieldID` on a member that does not exist raises `NoSuchFieldError` and leaves a pending exception, which can make the whole `build()` fail] → Mitigation: the builder member is added before the C++ reads it; the reference implementation's field name is reused deliberately so the ported JNI code matches; the same Java test covers the creation path.
- [The JNI translation unit is a single ~4.2k-line file shared with other in-flight JNI work (`naviveylin-local` and its successor branches), so conflicts are likely] → Mitigation: branch off current `master`, keep the diff to one helper plus one entry point plus one constructor argument; land before any further JNI port (the remaining route-instruction work touches the same file).
- [Two basemap stylesheet entry points can drift again: the resolution helper is shared, but the "named vs not named" decision lives in `build()` and the "fail vs fall back" decision in `loadStyleSheet`] → Mitigation: the spec scenarios pin both behaviours, and tests cover the invalid-name, unknown-name and unparseable cases separately.
- [A resolved-but-unparseable basemap stylesheet silently drops the basemap layer, and with the fire-and-forget setter a runtime directory change has no direct success signal] → Mitigation: the failure is reported through `wasLastStyleLoadSuccessful()`; a boolean/awaiting variant is documented as D2 alternative 1 and can be added later without a spec change.
- [The stylesheet directory default (`stylesheets`, relative to the process working directory) now silently affects the basemap stylesheet when the builder does not set it explicitly] → Trade-off accepted: resolving through `Settings` keeps the basemap and map stylesheets on the same directory; a builder-set directory behaves exactly as before.
- [`libosmscout-client-java/AGENTS.md` states "Meson (only — no CMake support)", while `master` builds the same targets from `libosmscout-client-java/CMakeLists.txt` and `meson.build`] → Mitigation: correct the note while touching this module, or record it as a separate documentation fix.

## Migration Plan

- Branch off `master` (`origin/naviveylin-local` is 12 commits behind and its JNI variants are not portable as-is, so this is a fresh port rather than a cherry-pick).
- Order: shared resolution helper → builder member → JNI `build()` wiring → Java client setter → JNI entry point → Java tests.
- No data or database format change, no API removal: existing builder and client calls behave identically, so rollback is a revert of the change.
- `naviveylin-local` should fast-forward onto `master` and drop the superseded JNI hunks afterwards, otherwise the same code paths stay in two shapes.
