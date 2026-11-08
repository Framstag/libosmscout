# Design — client-java-avoid-unpaved

## Context

`osmscout::FastestPathRoutingProfile` already supports per-grade speeds: `AddType(const TypeInfoRef&,
SpeedVariant)` takes five speeds, one per grade, and both cost paths read them — `GetCosts` indexes
`speeds[type][variant.grade]` for routing-graph objects, `GetTime2` indexes the same table for a `Way` or
`Area` feature value. The routing index stores the grade per object (`ObjectVariantData{type, maxSpeed,
grade}`), the importer fills it from `surface`/`tracktype`, and the cost function masks nothing because
nothing else uses the byte.

What the bridge does instead is `AddType(type, speed)`, whose `SpeedVariant::Fill(speed)` puts the same speed
in all five slots. That single call is the whole defect.

## Decision 1 — where the table is applied

**Chosen: parametrize the vehicle normally, then re-price the types.**

`FastestPathRoutingProfile::ParametrizeForCar` (and the bicycle/foot counterparts) does more than add types:
it enables the junction penalty and doubles the cost limit and factor. `AbstractRoutingProfile::AddType`
does not. A profile whose type list is built by hand therefore loses both, and degrading a road raises the
route's cost while the limit that bounds the search stays where the grade-blind profile put it — a route
that needs a high-grade way then fails with "no route" instead of being detoured.

**Rejected: build the type list from the speed map directly.** Cheaper to read, but it duplicates the
library's profile-level setup, and every future addition to `ParametrizeForCar` would silently miss it.

**Rejected: a new `RoutingProfile` method in the library** (`SetGradeSpeeds(...)`). It would be the cleaner
home, but `AddType` already carries the capability, and this change is a bridge fix — the library needs no
new API to deliver it.

## Decision 2 — the table

**Chosen: 1.0 / 0.85 / 0.6 / 0.3 / 0.1 of the type's base speed, floored at 3 km/h (grade 1 → 5).**

The flag's intent is "avoid unpaved", so the table is aggressive towards the soft grades: a grade 4 road
costs three and a third times a solid one, a grade 5 road ten times, while grade 1 keeps the type's speed
unchanged. The floor keeps a degraded road routable at all, so the aggressive table cannot turn "avoid" into
"cannot route" for a way that has no alternative. The source branch compared several candidate tables on a
Bremen database through its probe; that probe needs a routable database, which this checkout does not have
(`maps/` is an import-version mismatch), so the table's arithmetic is pinned by the host test instead and
the runtime comparison is not repeated here.

**Rejected: a grade-blind cost multiplier.** Multiplying a whole route's cost does not express "avoid this
way"; the grade is per object, not per route.

**Rejected: speed 0 for grade 5.** It makes the type unusable, which is a different statement from "avoid",
and it shrinks the profile's slowest type to zero — the shape behind the `NoRouteFound` above.

## Decision 3 — a host-testable helper

`routing_profile_speed.h` is already the bridge's dependency-free helper (the "usable speed or built-in
default" decision), pinned by `RoutingProfileSpeedTest`. The grade arithmetic is factored into the same
header as `AvoidUnpavedGradeSpeedsKmh(baseSpeedKmh)` together with the table and the floor, so the ordering
and the floor are asserted on the host instead of only observed through a built map. `ApplyAvoidUnpavedGradeSpeeds`
in the bridge is now the thin part that walks the type configuration, decides routability and base speed,
and hands the base speed to the helper. This does not change what the bridge computes: the helper applies
the same `max(floor, base * factor)` the reference applies inline.

## Consequence worth recording

The bridge's car speed map has no `highway_track` entry, so `ParametrizeForCar` logs
`No speed for type 'highway_track'` and every track type keeps a zero speed: tracks are not car-routable at
all, with or without the flag. For a car, `avoidUnpaved` therefore acts on roads whose `surface`/`tracktype`
tags give them a grade. Making tracks car-routable is a separate decision (it changes which roads the car
profile uses) and is not part of this change.

## Verification

`RoutingProfileSpeedTest` pins the table (1.0/0.85/0.6/0.3/0.1), the strict fall of the grade speeds, the
3 km/h floor and the grade-1 identity. The bridge itself compiles into `osmscout_client_java`. The
end-to-end route cannot be exercised in this checkout: `maps/` is v26 against a v27 importer and there is no
routable database, so no route is calculated and no database is imported for it.
