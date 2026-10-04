# Design

## Context

See proposal.md — Why. The current state that shapes the approach:

- `libosmscout-map-opengl/src/osmscoutmapopengl/MapPainterOpenGL.cpp:925` (`ProcessNode`) calls
  `osmscout::Triangulate::TriangulatePolygon` on the node path with no guard at all, while the area path
  at the same file already wraps its call (`MapPainterOpenGL.cpp:348`) in
  `catch (const std::runtime_error&)` and continues with
  `log.Warn() << "Skip area " << area->GetFileOffset() << ", triangulation failed: " << e.what()`.
  The crash TODO §97 reports is the node path's version of exactly that failure.
- `libosmscout-map-opengl/src/osmscoutmapopengl/Triangulate.cpp:28` and its three sibling overloads build
  `p2t::Point` objects straight from the input and call `p2t::CDT::Triangulate()`; nothing validates or
  catches.
- The vendored triangulator is `libosmscout-map-opengl/{include,src}/poly2tri`. It **throws**
  `std::runtime_error` for the input it detects — `sweep/sweep.cc:66` and `:72` "Provided polygon is not
  simple", `:130`/`:146` "EdgeEvent - collinear points not supported", `:766` "[Unsupported] Opposing
  point on constrained edge" — and it **asserts** where it does not throw: `sweep/advancing_front.cc:85`
  (`assert(0)`) and `common/shapes.h:139` (`// Repeat points; assert(false)` in `p2t::Edge::Edge`, i.e. a
  zero-length edge from a repeated point). A `Release` build compiles both out, which is what turns the
  same input into the null dereference in `p2t::Sweep::NewFrontTriangle` that `gdb` reports in TODO §97
  instead of a throw.
- What is *not* a rejection class: a self-crossing ring. Measured against the vendored library, a bow tie
  is triangulated (five triangles) rather than detected, so the contract cannot claim that a
  non-simple ring is rejected without the O(n²) simplicity check this design declines to add (decision 2,
  alternative C). The spec's rejection scenarios name the classes the library genuinely cannot sweep.
- Polygons reach the triangulation from map data, i.e. from input no code of this project validated:
  rings with a repeated closing point, rings with a duplicated vertex, rings with fewer than three
  distinct points, and self-intersecting rings all occur.
- Test precedent: `Tests/CMakeLists.txt:568-569` (`OpenGLAreaVisibilityTest`) links `OSMScout::MapOpenGL`,
  so a unit target for the triangulation is established practice; nothing calls `Triangulate` in `Tests/`
  today. The triangulation itself makes no GL call, so it runs without a GL context.
- The integration trigger is in TODO §97: `Tests/PerformanceTest --driver opengl` with `maps/Dortmund`
  and `stylesheets/standard.oss`, `cycle.oss` or `winter-sports.oss` SIGSEGVs at every zoom level, while
  `public-transport.oss` survives.

## Goals / Non-Goals

**Goals:**

- A frame never terminates because a polygon cannot be triangulated, on any caller of the triangulation.
- Input that repeats points is triangulated instead of rejected, without changing which area is covered.
- The triangles of an accepted polygon are byte-identical to today's, so nothing that renders now renders
  differently.
- The OpenGL backend draws the shipped area styles on the Dortmund database, and names what it skipped.

**Non-Goals (design-level):**

- Replacing or upgrading the vendored triangulator, and making its assert-based invariants release-safe.
- The other §57/§98/§99 findings in the same OpenGL code (reprocessing cost, border-only rings, the
  tolerance of the first border style).
- Changing which geometry is handed to the triangulation (that is the `AreaVisibility`/culling path).

## Decisions

### 1. The guard lives in `osmscout::Triangulate`, not at the call sites

**Chosen (A):** normalization, rejection and the report are implemented once in
`Triangulate.cpp`, behind the existing signatures, so every current and future caller is safe without
repeating the guard. The callers' own `catch` stays where it exists (the area path) as a second line of
defence, and the node path gains the same skip/report around its call so the *object* that was skipped is
named.

Alternatives:

- **(B)** Guard at each call site in `MapPainterOpenGL` only. Rejected: there are two call sites today
  (node `:925`, area `:348`) and the area path's existing guard is precisely what the node path lacks -
  copying it reproduces the asymmetry that caused the defect, and a third caller would repeat it again.
- **(C)** Patch the vendored library so it never dereferences a null node and always throws. Rejected as
  the primary fix: it edits third-party code that the project does not maintain, and it still leaves the
  caller with an exception to handle; it is recorded as a follow-up, because a case the validation misses
  will otherwise still crash.

Risk of (A): the triangulation now swallows failures that a caller might want to distinguish. Mitigation:
the reason is logged where the rejection happens, the painter logs the object it skipped, and the returned
value is the empty triangle list the callers already handle.

### 2. A rejection is produced by validating the input *and* catching the library's throw

**Chosen (A):** normalize the polygon first (decision 3) and additionally wrap the `p2t` call in a
`catch (const std::runtime_error&)` that logs and returns no triangles, because the library's invariant
checks are compiled out under `NDEBUG` and the observed failure is a dereference rather than a throw.

Alternatives:

- **(B)** Catch only. Rejected: it handles the shapes the library detects and not the shapes that reach
  its compiled-out asserts - the SIGSEGV §97 reports is the second class, so catching alone would leave
  the crash in place for the same input.
- **(C)** Validate only, with a complete simplicity test (a full O(n²) or exact-predicate check that the
  ring is simple and non-collinear). Rejected for this change: it pays a quadratic cost per ring on a hot
  path to replace a guard that is already cheap, the library's own detector is the authority on what it
  can sweep, and — measured while implementing — the class it would buy is self-crossing input, which the
  library triangulates rather than rejects (five triangles for a bow tie). It is recorded as a possible
  later hardening if a case survives validation.

Risk of (A): a case that neither the validation nor the library's detector notices still dereferences.
Mitigation: the diagnosis (task 1.1) captured the input that reaches the compiled-out assert as the
regression fixture, the Debug and sanitizer configurations keep the vendored asserts active, and the one
dereference that survived both — the sweep losing its triangle, decision 6 — is guarded inside the
vendored copy.

### 3. Normalization removes a repeated point and a redundant collinear point

**Chosen (A):** before triangulating, drop a point equal to its predecessor, drop a closing point equal to
the ring's first point, drop a point whose two neighbours and itself are collinear (it contributes no
corner and its removal does not change the covered area), and reject the ring when fewer than three
distinct points remain.

Alternatives:

- **(B)** Drop repeated points only. Rejected: "EdgeEvent - collinear points not supported"
  (`sweep/sweep.cc:130`) is a documented rejection of input map data produces, and the removal is the
  cheapest way to make it triangulable; it changes no covered area.
- **(C)** Reorder or re-orient the ring (sort by angle, enforce winding). Rejected: orientation and order
  are the caller's, the library handles either winding, and reordering can turn a simple ring into a
  self-intersecting one.

Risk of (A): a ring that is a legitimate thin spike loses its tip when its neighbours are collinear - the
covered area of a zero-area spike is unchanged, and the golden cases (task 2.3) pin that no accepted
polygon's triangles change. The removal of a repeated point is also what keeps the zero-length edge of
`common/shapes.h:139` out of the library, so normalization and rejection together cover both of the crash
mechanisms the diagnosis found.

### 4. The rejection is reported through the log, with no API change

**Chosen (A):** the triangulation logs the reason at `Warn` (the shape the area path uses) and returns an
empty list; the painter logs the object it skipped, naming the file offset, next to its existing area-path
line.

Alternatives:

- **(B)** Return a status (a pair of triangles and a reason, or an out-parameter). Rejected: it changes a
  public signature used by the OpenGL backend for no behavioural gain — the caller's action on a rejection
  is always "draw nothing and continue".
- **(C)** Let the exception escape and require every caller to catch it. Rejected: that is today's state
  on the node path, and it makes safety a property of the caller, not of the triangulation.

Risk of (A): a log-only report is easy to ignore. Mitigation: the integration run's log is part of the
verification (the skipped-object line is read), and the report names the object rather than only the
reason.

### 5. Verification is a unit target on the triangulation plus the integration run

**Chosen (A):** a new Catch2 target (`Tests/src/PolygonTriangulationTest.cpp`, linking
`OSMScout::MapOpenGL` as `OpenGLAreaVisibilityTest` does) covers the degenerate inputs, the normalization
and the unchanged triangles; the documented integration trigger is run once for the frame-level
requirement.

Alternatives:

- **(B)** Integration only (`PerformanceTest --driver opengl`). Rejected: it needs the Dortmund database, a
  font and shaders, takes minutes, and cannot pin *which* input is rejected.
- **(C)** A painter-level test only. Rejected: the OpenGL painter needs a GL context and a device, which
  the triangulation itself does not, so the cheap and precise cases would be paid for with the expensive
  harness.

Risk of (A): the unit target links a GL library without initialising GL. Mitigation: the triangulation
calls no GL function (only `GLfloat` in its return type), and the target links exactly what
`OpenGLAreaVisibilityTest` links, so no context is created.

### 6. The one place the vendored sweep dereferences a lost triangle throws instead

**Chosen (A):** `Sweep::EdgeEvent` gains a guard that throws when it is handed no triangle, marked as a
local deviation from upstream. That is the site the second diagnosis reached: the sweep loses the triangle
of its edge when the neighbour it rotates to (`NeighborAcross` at `sweep.cc:127`/`:143`, `NeighborCW`/`NeighborCCW`
at `:155`/`:157`) does not exist, and the next recursion dereferences the null (gdb: `#0 Triangle::EdgeIndex`
- the first member read of a null `this` - `<- #1 Sweep::EdgeEvent`). Throwing here is what the library
already does in five other places to report input it cannot sweep, so the triangulation's existing catch
turns it into a rejection with a log line, and the requirement holds for this class and for every other
input that loses the sweep its triangle.

Alternatives:

- **(B)** Guard the caller instead: skip symbol polygons whose mapped size is below a device pixel. Rejected
as the fix: it avoids the one input the diagnosis found, not the class - another shape that loses the sweep
its triangle still crashes - and it silently changes which symbol polygons are drawn. It stays available as
an optimisation of the node path, which unlike the area path does not cull.
- **(C)** Leave the vendored copy untouched and record the patch as a follow-up. Rejected: the requirement
is that a rejected polygon does not terminate the render, and this input would still terminate it, so the
change would not close TODO §97.

Risk of (A): the guard is lost when the vendored copy is updated to a newer upstream. Mitigation: the guard
carries the marker comment naming this change and TODO §97, and the two fixtures of the class (the captured
thin rectangle and the degenerate rings) fail loudly if it disappears.

### Sequence: a node whose ring the triangulation rejects

```
  frame (ProcessNodes)
      |
      v
  ProcessNode(node)
      |
      v
  Triangulate::TriangulatePolygon(points)
      |
      v
  normalize: drop repeated point / closing repeat / redundant collinear
      |
      +-- fewer than 3 distinct points --> reject (log reason) ------------+
      |                                                                    |
      v                                                                    |
  p2t::CDT::Triangulate()                                                  |
      |                                                                    |
      +-- throws (not simple / collinear / unsupported) --> reject (log) --+
      |                                                                    |
      +-- returns triangles --> return them                               |
                                                                           v
                                                        ProcessNode: skip this object,
                                                        log "Skip node <offset>",
                                                        continue with the next object
                                                                           |
                                                                           v
                                                        frame completes (no SIGSEGV)
```

## Risks / Trade-offs

- [A case survives both the validation and the library's detector and still dereferences] → the Debug and
  sanitizer configurations keep the vendored asserts active and fire there; the one dereference that
  survived both is guarded in the vendored copy (decision 6), and its fixture fails if that guard goes away.
- [Normalization changes which triangles an accepted polygon yields] → collinear and repeated points add
  no corner and no area; the golden comparison of task 2.3 runs before/after on the same input.
- [The catch hides a defect that should be fixed instead] → the exception text is logged with the reason,
  so the class of rejected input is visible in the run's log; the change's verification records what
  appeared during the integration run.
- [Cost on the hot path] → normalization is a single O(n) pass per ring, unlike the existing quadratic
  duplicate removal in the area path (`MapPainterOpenGL.cpp:325-334`, TODO §57); the OpenGL performance
  driver measures the step before and after.
- [The integration trigger is heavy] → the unit target carries the contract; the integration run is the
  frame-level check and is run once, with the recipe from TODO §97 (database, shaders, font).
- [The unit target links GL without a context] → no GL call is made by the triangulation; if the link
  proves to need more, the target mirrors `OpenGLAreaVisibilityTest` exactly.

## Migration Plan

Additive and internal: the public signatures of `Triangulate` are unchanged, no format or database
contract is touched, and the OpenGL output for accepted polygons is identical. Rollback is a revert.

## Open Questions

- Whether the vendored triangulator should be updated to a revision whose invariants do not depend on
  `assert`, or keep the local guards. Deferrable: it does not change the requirements or the tasks, and
  the two crash classes this change's diagnosis captured are covered by the normalization, the rejection
  and decision 6's guard.
