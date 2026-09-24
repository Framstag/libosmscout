# Design

## Context

See `proposal.md` - Why. The relevant current state:

- `libosmscout/src/osmscout/navigation/SpeedAgent.cpp` handles `GPSUpdateMessage`s whose
  `horizontalAccuracy` is below 100 m. When the message carries a speed (`currentSpeed >= 0`) the agent
  publishes it converted to km/h; otherwise it appends the distance and duration between the previous and the
  current fix to a list of segments, sums that list, publishes the resulting speed, and pops segments while
  the summed duration exceeds three seconds.
- `SpeedAgent.h` holds that list (`segmentFifo`) and the previous fix (`lastPosition`, updated for every
  accepted fix), plus the last reported maximum speed.
- The published messages are `CurrentSpeedMessage` (a value, or `-1` for unknown) and
  `MaxAllowedSpeedMessage`; they reach clients through the navigation engine, and the Java bridge forwards
  them through `NavigationListener` (`javascout-navigation`).
- There was no test for the agent; `Tests/src/SpeedAgentTest.cpp` is added by this change and drives the
  agent directly with synthetic fixes, so no database or device is involved.

## Goals / Non-Goals

**Goals:**

- Report a standing vehicle as standing, whatever the receiver's fix rate and jitter, without turning the
  decision into a speed floor that deletes walking.
- Leave every other reported speed exactly as it was, so curves, stops and slow movement keep their
  magnitude and clients see one behaviour change only.
- Make the decision and its known limit visible in the code and assertable in tests.

**Non-Goals:**

- Fixing the fallback's own guard, which keeps it from running at fix rates above 1 Hz (recorded in `TODO.md`
  and prepared as the next change).
- Changing the fallback's magnitude computation from a sum of segment distances to a displacement: distance
  travelled and displacement differ on curves, and a displacement-based magnitude would cut corners.
- Filtering or smoothing the position fixes themselves, which is the `PositionAgent`'s domain.
- Changing the published message types, the agent list of the navigation engine, or the Java side.

## Decisions

**D1 - The standing decision compares net displacement over a window of fixes against a floor.**
Alternatives:
- *A distance floor per segment* (the superseded attempt): zeroing segments shorter than about 2 m deletes
  walking - at 1 Hz a walker at 3 to 5 km/h covers 0.8 to 1.4 m per fix - so it is a speed floor in disguise
  and reports a walker as standing.
- *A time floor per segment*: jitter is not a property of a segment's duration but of how many fixes arrive,
  so a longer segment with more jitter inside it passes the floor just the same.
- *A threshold on the reported speed, requiring it to exceed a value for several fixes*: it delays the
  report of a real stop instead of deciding it, and jitter still reads as a small speed.
- *Weighing the displacement by the receiver's reported accuracy*: `horizontalAccuracy` is only used here as
  an acceptance gate below 100 m, which is far too coarse to derive a jitter amplitude from; a per-fix
  accuracy could refine the floor later (see Open Questions) but does not replace the decision.
Chosen because noise does not travel: jitter stays inside its own radius while real movement accumulates, so
accumulated displacement per window separates the two without any assumption about the fix rate.

**D2 - The window is five seconds, the floor is three metres, and the decision needs four seconds of history.**
Alternatives:
- *A shorter window*: it would reduce the history needed but also the displacement a walker accumulates, which
  is what keeps walking above the floor; five seconds is the shortest window in which walking at 3 km/h
  (4.2 m) clearly exceeds jitter of 1 m amplitude (inside 2 m).
- *A longer window*: it reports a stop later and a start later, because the window is also the decision's
  latency.
- *A floor derived from a speed*: the same as a fixed displacement per fixed window, with an extra conversion
  and no gain.
- *Using the whole window from the first fix*: the first fixes of a session have no history, so the plain
  fallback must decide until the window can separate the two cases; the four-second minimum states that
  explicitly and leaves the plain behaviour intact at a start.
The resulting limit - movement that stays below the floor is reported as standing (about 2.7 km/h at 1 Hz) -
is documented at the gate and asserted by a test rather than left implicit.

**D3 - Only the standing case changes; the moving magnitude stays the sum of segment distances.**
Alternatives:
- *Use net displacement for the magnitude as well*: it would remove the jitter inflation, but the displacement
  between two fixes on a curve is shorter than the distance travelled - the speed would be under-reported
  wherever the vehicle turns.
- *Use displacement only below a speed threshold*: the reported value would jump where the two computations
  meet.
Chosen because distance travelled and displacement answer different questions, and only the second one can
decide whether the vehicle moves; a client that wants the smoothed magnitude is unaffected by this change.

**D4 - While the decision holds, the agent publishes a standstill and discards the segment history.**
Alternatives:
- *Publish nothing*: a client keeps displaying the speed it last saw, which is the symptom being fixed.
- *Publish unknown (`-1`)*: a client cannot distinguish "standing" from "no signal", and the agent already
  uses `-1` for a lost signal in a tunnel.
Chosen because standing is a measurement the agent can make, and discarding the segments is what keeps jitter
that arrived before the decision from being summed into a speed afterwards.

**D5 - The decision lives in the speed agent, not in the position agent or a client.**
Alternatives:
- *In `PositionAgent`*: that agent's job is to snap the position onto the route and to estimate progress
  along it; a speed decision there would mix two responsibilities and make the snapped position depend on
  it.
- *In each client (Java, Qt, others)*: every client would re-implement the same heuristic and they would
  drift apart, and a client without the fix history could not do it at all.
Chosen because the agent already keeps the fix history the decision needs, and the speed contract is what it
publishes.

**D6 - The gate keeps its own list of recent fixes instead of reusing the segment history.**
Alternatives:
- *Reuse `segmentFifo`*: it stores summed segment distances, not positions, so the oldest and newest fix of
  the window would have to be reconstructed from them, and the list is trimmed by three seconds of duration
  rather than by the window the decision needs.
Chosen because the decision needs two positions and their times, which a small separate list provides
directly; the segment history keeps its own purpose and trimming.

## Sequence diagram

```
GPS update (no receiver speed)
        |
        v
+---------------------------------------------------------------+
| SpeedAgent::Process                                            |
|                                                               |
|  gap > 10 s ? -------------------------> clear fifo + fixes    |
|                                                               |
|  recentFixes.push(current fix)                                 |
|  trim window to <= 5 s                                         |
|                                                               |
|  history = now - oldest.time                                   |
|  net     = distance(oldest.coord, current.coord)               |
|                                                               |
|  history >= 4 s && net < 3 m ?                                 |
|        | yes: clear segmentFifo                                |
|        |      publish CurrentSpeedMessage(0 km/h)               |
|        |                                                       |
|        + no : sum segmentFifo (as before)                      |
|               publish CurrentSpeedMessage(summed km/h)          |
|               pop while summed duration > 3 s                  |
+---------------------------------------------------------------+
        |
        v
  NavigationEngine -> client (Java listener, Qt, ...)
```

## Risks / Trade-offs

- *The floor is a constant, so a receiver whose jitter exceeds it reports standing while moving slowly* →
  the limit is documented where the decision is taken, asserted by a test, and the receiver's own speed is
  preferred whenever it is available; an accuracy-scaled floor is recorded as an open question.
- *The decision needs four seconds of history, so a stop is reported as standing only after that* → during
  those fixes the plain fallback reports the jitter-derived speed, exactly as before; the test asserts that
  the first published speeds of a session come from the plain fallback, so the behaviour is explicit.
- *A real slow walker is now reported as standing* (movement below the floor) → the alternative (per-segment
  floor) reported a walker as standing in every case, so this is strictly narrower; the boundary is tested.
- *The fallback still does not run at fix rates above 1 Hz* → the gate is inert on such a stream, and the
  agent publishes no speed at all there when the receiver reports none; recorded in `TODO.md` and prepared as
  the next change, and the test helper documents why it reads "the last published speed".
- *The window is trimmed by timestamps, not by a fix count* → a receiver with irregular intervals gets a
  time-based window, which is what the decision wants; a burst of fixes inside one second simply does not
  move the oldest fix out of the window earlier.
- *Behaviour visible to users*: a parked vehicle now reports 0 km/h instead of 1 to 4 km/h, and a very slow
  walker reports 0 - intended, and stated in the capability spec.
- *Divergence from the downstream branch*: the superseded two-metre-floor variant is not carried, and the
  extracted implementation is kept as reviewed.

## Migration Plan

No API, message or configuration change: only the value the agent publishes for a standing vehicle. A client
needs no change; a client that displays the speed will show 0 instead of a phantom value while parked.
Rollback is a revert of the commits, after which the phantom speed returns.

## Open Questions

- Whether the floor should scale with the receiver's reported accuracy per fix (a receiver that reports a
  10 m accuracy is likely to jitter more than one that reports 1 m): deferrable, it would refine a constant
  and not change the contract.
- Whether a client should be able to configure the floor or the window: deferrable, and it would be a
  settings change in the client library rather than in the agent.
