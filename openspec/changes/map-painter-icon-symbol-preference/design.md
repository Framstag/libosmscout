# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-map/src/osmscoutmap/MapPainter.cpp`, point label stage: for a style entry with an icon
  style the code checks `!iconStyle->GetIconName().empty() && HasIcon(...)` first and pushes an
  `Icon` label data; only when that is false does it test `iconStyle->GetSymbol()` and push a `Symbol`
  label data. The raster icon therefore always wins when it is both named and servable, and the symbol is
  a pure fallback.
- `HasIcon` is a backend capability: it is what decides whether the icon image can be served, and it is
  the only reason a symbol is currently reached for an entry that names an icon.
- The render parameters are already the per-render configuration carrier (`MapParameter`: icon size,
  icon pixel size, icon padding, pattern mode, label parameters, …), so a rendering preference belongs
  there and is copied per render.
- The Java client builds a `MapParameter` for every frame from values stored on its client handle, so a
  preference stored there reaches the next frame without touching the stylesheet.

## Goals / Non-Goals

**Goals:**

- Give a caller a per-render choice between the two renderings of a style entry, defaulting to today's
  behaviour.
- Keep the choice in the shared frame preparation, so every backend behaves identically and no backend
  has to learn a new parameter.
- Keep the entry's fallback behaviour intact when only one rendering is available.

**Non-Goals:**

- Changing the stylesheets or the style syntax, or adding a per-type or per-entry preference.
- Changing `HasIcon`/`DrawIcon` or the raster icon lookup.
- Changing which entry is chosen by the style; this is about which of that entry's two renderings is
  drawn.
- Changing the symbol renderers.

## Decisions

**D1 — The preference is a render parameter, not stylesheet state.**
`MapParameter` gains the preference, defaulting to the raster icon; the label stage reads it.
Alternatives:
- *A setting in the stylesheet (`.oss`)*: it is per style and cannot be changed without a reload, and it
  mixes a rendering policy into the style data; the change explicitly wants the next frame to follow it.
- *A global or static flag*: not per render, not testable in isolation, and process-wide state in a
  library that is used by several clients at once.
- *A new backend capability query*: moves the decision into every backend and duplicates it.
Chosen because `MapParameter` is already the per-render configuration carrier and the decision is
backend-independent.

**D2 — The decision is taken in the shared frame preparation.**
The point label stage decides between the two label-data kinds and pushes exactly one; the backends keep
drawing whatever label data they are given.
Alternatives:
- *Decide inside each backend's icon drawing*: the backend that would have to skip its raster icon would
  also need the symbol fallback path, so the reordering would be repeated per backend and could drift.
- *Filter the style before rendering*: it would discard one of the two renderings for the whole render,
  which is what the preference expresses, but it also loses the per-entry fallback for an unservable icon
  and makes the stylesheet data look changed to other consumers.
Chosen because there is exactly one place where a style entry becomes a label, and the fallback for a
single rendering stays where it is.

**D3 — Additive and default-off.**
The parameter defaults to the raster icon; the two existing branches are reordered, not removed, and the
"prefer symbol" branch is the new first test.
Alternatives:
- *Default to the symbol*: changes rendering for every existing client, including the demos and the Qt
  client, without a stylesheet change to justify it.
- *Introduce a tri-state ("auto", icon, symbol)*: "auto" and "icon" would behave identically today, so it
  is an unobservable extra state until a third rendering exists.
Chosen because the change must not alter any existing render, and the preference is a consumer decision.

## Sequence diagram

```
caller (Java or C++)        MapParameter              MapPainter (frame preparation)        backend
      |                          |                                |                             |
      | set preference --------->|                                |                             |
      | render ----------------->| copy into render parameters --->|                             |
      |                          |                                | for each style entry:       |
      |                          |                                |   both renderings?          |
      |                          |                                |     prefer symbol? -> Symbol|
      |                          |                                |     else servable icon? -> Icon
      |                          |                                |     else symbol? -> Symbol  |
      |                          |                                | push one label data         |
      |                          |                                |---------------------------->| draw
```

## Risks / Trade-offs

- *A client that sets the preference and expects an icon* → the parameter is documented as "prefer the
  vector symbol of an entry that carries both"; an entry with only an icon still draws the icon.
- *An entry preferred as a symbol but without a symbol* → falls through to the raster icon branch, so the
  preference can never make a render emptier than the raster icon path already was. Covered by a case.
- *The label data kind is observable in tests through the no-op painter* → the test asserts the kind and
  the icon style, not pixels, so it is backend-independent; a pixel test would need an image file and a
  backend.
- *Two clients of one process with different preferences* → the preference travels in the render
  parameters of each frame, so they cannot affect each other.

## Migration Plan

Additive: the default reproduces the previous behaviour exactly, no signature is removed, and no data is
persisted. A consumer adopts the preference by setting it once; rollback is a revert of the commits, which
only affects consumers that set it.

## Open Questions

- Whether a stylesheet-level default (a style declaring "this type prefers its symbol") should exist:
  deferrable, it would be a separate style-syntax change and is not needed by any current consumer.
