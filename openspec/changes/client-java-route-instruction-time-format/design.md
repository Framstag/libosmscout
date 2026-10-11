# Design

## Context

See `proposal.md` — Why. The relevant current state:

- `libosmscout-client-java/src/OSMScoutClient.cpp`: the description text builder of both route-building paths
  formats a step's time by converting the difference to minutes and printing it, with hours when the minutes
  reach 60. A difference below a minute therefore prints as "0 min".
- The distance part of the same line already distinguishes sub-metre cases (it prints metres when the
  distance is small), so the line has no uniform unit rule today.
- The per-step numeric values are published separately on the route and the instruction
  (`client-java-route-instruction-metrics`); the text is a display of them, not their source.

## Goals / Non-Goals

**Goals:**

- Give a short step a duration a user can read.
- Keep the line's existing shape, columns and separators.
- Keep both description paths identical.

**Non-Goals:**

- Changing the per-step numeric values or their sources.
- Changing the distance column or adding a new column.
- Introducing a locale-dependent time format or a unit setting.
- Formatting the route's total duration differently (it keeps the existing rule).

## Decisions

**D1 — Seconds below a minute, minutes below an hour, hours and minutes above.**
Alternatives:
- *Keep "0 min"*: the defect; a city route's step list carries no duration.
- *Round a sub-minute step up to "1 min"*: shows a duration that was not estimated, and a list of such steps
  would sum to more than the route's duration.
- *Always print seconds*: a two-hour step would read as "7200 s", which is unreadable and inconsistent with
  the route's total.
Chosen because the unit follows the magnitude, which is what the distance column already does informally.

**D2 — A time below a second prints no time.**
Alternatives:
- *Print "0 s"*: a zero-valued time on a step that does exist reads as missing data.
- *Print the fractional seconds*: the value is truncated to whole seconds elsewhere, and a fractional second
  is below the precision a user can act on.
Chosen because the line already prints conditionally (distance and time are separated by a comma only when
both are present), so omitting a sub-second time fits the existing shape.

**D3 — One rule, applied in both paths.**
The formatting is changed in the builder both paths use, or in both builders with the same rule.
Alternatives:
- *Change only the calculate path*: a recalculated route would still print "0 min", and the two paths are
  meant to describe one route identically.
- *Factor the whole line builder out first*: a bigger refactor than the defect needs, and it would move the
  distance formatting too.
Chosen because the defect is one condition in the time part, and the requirement pins that the paths agree.

## Sequence diagram

```
route node time            description text builder                 Java description line
      |                            |                                        |
      | dt = time - previous time->| dt >= 1 h ? h + remaining min          |
      |                            | dt >= 1 min ? whole minutes            |
      |                            | dt >= 1 s ? whole seconds              |
      |                            | dt < 1 s  ? no time part               |
      |                            | append distance and the time part ---->| "… [1.2 km, 45 s]"
```

## Risks / Trade-offs

- *A consumer parses the time column and expects "min"* → the column already varied its unit (hours and
  minutes, minutes), so it was never a fixed-unit column; the requirement states the rule.
- *A step below a second prints no time, so a column can be empty* → the same line can already print without
  a distance part, and a separator is only added when both parts are present.
- *Two builders must stay in step* → the requirement has a scenario that compares the two paths' output for
  one description.
- *The rule is display-only, so it is hard to unit-test without a route* → the time rule is factored into the
  dependency-free header `route_step_time.h`, which both description paths call, and the host test
  `Tests/src/RouteStepTimeTest.cpp` exercises it. The shipped map databases carry format version 26 while the
  library expects 27, so a route probe through the Java bridge is not available on this machine either.

## Migration Plan

Display-only: no value, signature or data changes. Rollback is a revert of the commits, which restores the
minute-only formatting.

## Open Questions

- Whether the route's total duration should follow the same magnitude rule: deferrable, and a separate
  display decision.
- Whether the description paths should share one builder instead of two: deferrable, a refactor rather than a
  fix.
