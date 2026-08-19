## Context

Current long-press flow (see proposal.md — Why):

```
MapInteractionHandler (long-press timer, JavaScout/.../MapInteractionHandler.java)
  → MainController.onLongPress(lat, lon)            (MainController.java L1801)
  → OSMScoutClient.getDescription(lat, lon)         (JavaScout, native call)
  → Java_com_framstag_libosmscout_client_OSMScoutClient_getDescription
      (libosmscout-client-java/src/OSMScoutClient.cpp L2613)
      — already collects AND ranks candidates (L2660-2703): areas/ways/nodes
        in 50m radius, ranked by type, containment, distance
      — currently keeps only the best one and builds one ObjectDescription
  → MainController.showDescriptionOverlay(desc)     (L1836)
  → DescriptionOverlay (JavaScout/.../DescriptionOverlay.java)
```

Search results are rendered by `SearchOverlay` (JavaScout/.../SearchOverlay.java L192-229) as a 3-line cell: line 1 name/address, line 2 admin hierarchy, line 3 object type + offset. That format is the "search result description" to mirror for candidates.

## Goals / Non-Goals

**Goals:**
- Long-press returns all reasonable candidates, ranked as today
- Candidate list rendered in the search-result cell format, user picks one
- Single candidate → direct description dialog; zero → "No description available"
- Zero behavior change to existing `getDescription(lat, lon)` callers

**Non-Goals:**
- Touch-based long-press (JavaFX desktop scope only)
- Changes to core libosmscout description services (proposal Impact)
- Changing search ranking or `SearchOverlay` behavior

## Decisions

### D1: New native method `getDescriptionCandidates(lat, lon)` returning `List<ObjectDescription>`

Refactor the existing candidate collection/ranking block in `OSMScoutClient.cpp` (L2660-2703) into a shared helper that returns the ordered candidate list; build one `ObjectDescription` per candidate. New JNI entry point `Java_..._getDescriptionCandidates` returns a Java `List<ObjectDescription>`. `getDescription` stays and keeps its exact current semantics (top-ranked single object) — implemented as "first element of the candidate list".

When several databases cover the coordinate (e.g. a world basemap plus a regional map), candidates are collected from all databases and candidates from a coarser database whose bounding box contains a finer database's box are dropped — the basemap must not shadow the regional map's candidates. The lookup also takes the current magnification so candidates too small to be visible on screen (areas under ~1px) rank below visible ones.

- Alternative: reuse `getDescription` and add a separate "list objects" call returning lightweight refs. Rejected: two round-trips and duplicate marshalling; the picker needs the description sections anyway for search-result-like rendering and for the details dialog after selection.
- Alternative: single method with a `maxResults` parameter. Rejected: changes the existing public `getDescription` signature; a separate method keeps backward compatibility.

### D2: `ObjectDescription` carries the object reference (type + file offset)

Add fields to `ObjectDescription` (Java): object type name/type and file offset, filled by the JNI layer from the `FeatureValueBuffer`/object ref. The picker displays them in the search-result line-3 format; selection later needs the ref for nothing else (details come from the already-fetched `ObjectDescription`), but it makes entries unambiguous when two candidates have similar names.

- Alternative: return `List<ObjectDescription>` plus a parallel `List<ObjectRef>` and zip them in Java. Rejected: ordering coupling across two lists is fragile and hides the relation from the API surface.
- Alternative: define a new `CandidateDescription` wrapper class. Rejected: adds a type for one extra field; extending `ObjectDescription` keeps the API flat.

### D3: New `CandidatePickerOverlay` class, search-result cell layout extracted

Create `JavaScout/.../CandidatePickerOverlay.java` following the `DescriptionOverlay` pattern (StackPane, fade/slide animation, click-outside + Escape to close, fullscreen <600px, centered ≥600px). Extract the 3-line cell rendering from `SearchOverlay.updateItem` into a shared static builder (e.g. `SearchResultCell.create(uiScale, lines...)` or a small helper class) so candidate entries use literally the same format. Candidate name comes from the `ObjectDescription` entries (name label) with the object type/offset on line 3.

- Alternative: add a "candidate mode" to `SearchOverlay`. Rejected: `SearchOverlay` already carries search + favorites modes; a third mode raises coupling and regressions in the search UI.
- Alternative: duplicate the cell code in the new overlay. Rejected: drift between search results and candidate entries — the exact requirement ("same description as in search results") would rot.

### D4: Selection flow stays in `MainController`

`onLongPress` becomes: call `getDescriptionCandidates` on a background task; on success — 0 candidates → existing "No description available" dialog; 1 candidate → `showDescriptionOverlay` directly; N candidates → show `CandidatePickerOverlay`; user click → `showDescriptionOverlay(selected)`.

- Alternative: move the branching into a new presenter class. Rejected: MainController already owns overlay lifecycle; an extra indirection layer has no payoff at this size.

### Sequence (long-press, multi-candidate)

```
User ──hold 500ms──► MapInteractionHandler.fireLongPress()
  MainController.onLongPress(lat, lon)
  MainController ──Task──► OSMScoutClient.getDescriptionCandidates(lat, lon)
    OSMScoutClient.cpp: collect areas/ways/nodes in radius
      → rank (type, containment, distance) → cap at N
      → build List<ObjectDescription> (each with object ref)
  MainController (FX thread): size==1 → DescriptionOverlay(desc)
                               size>1  → CandidatePickerOverlay(candidates)
                               size==0 → "No description available"
  User clicks entry → CandidatePickerOverlay closes → DescriptionOverlay(selected)
```

## Risks / Trade-offs

- **JNI marshalling cost** — N full `ObjectDescription` objects per long-press instead of one. → Mitigation: cap the returned list at a small N (e.g. 10) after ranking; typical dense-map case is well below that.
- **Cell format drift** — if search-result cells change, candidate picker must follow. → Mitigation: D3 shared cell builder; single place to edit.
- **`ObjectDescription` API change** — new fields ripple through Java class + JNI fill code. → Mitigation: additive fields + overloaded constructor; existing `getDescription` fills them too, no behavior change.
- **`getDescription`/`getDescriptionCandidates` divergence** — two native paths to maintain. → Mitigation: D1 shared candidate helper; `getDescription` delegates.

## Migration Plan

1. Native: refactor candidate helper, add `getDescriptionCandidates` JNI, add object ref to `ObjectDescription` construction; keep `getDescription` delegating.
2. Java client: declare `getDescriptionCandidates` native method, extend `ObjectDescription` with ref fields.
3. JavaScout: extract cell builder, add `CandidatePickerOverlay`, rewire `onLongPress`.
4. Manual verification: long-press on overlapping objects (building + street + POI) shows picker; sparse area shows single dialog; empty area shows "No description available".
5. Rollback: revert `onLongPress` wiring to `getDescription`; all other additions stay inert.

## Open Questions

- Exact cap N for candidate list (10 suggested) — tune during verification, does not change specs or design.
- Candidate display name source: first `DescriptionEntry` with a name-like label vs. dedicated field. Resolved at implementation from `DescriptionEntry` keys; does not affect contract.
