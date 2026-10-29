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
does not. A profile whose type list is built by hand — the first shape the probe tried — therefore loses
both, and the probe failed with `NoRouteFound` on exactly the pairs whose route needs a high-grade way:
degrading a road raises the route's cost while the limit that bounds the search stays where the
grade-blind profile put it.

**Rejected: build the type list from the speed map directly.** Cheaper to read, but it duplicates the
library's profile-level setup, and every future addition to `ParametrizeForCar` would silently miss it.

**Rejected: a new `RoutingProfile` method in the library** (`SetGradeSpeeds(...)`). It would be the cleaner
home, but `AddType` already carries the capability, and this change is a bridge fix — the library needs no
new API to deliver it.

## Decision 2 — the table

**Chosen: 1.0 / 0.85 / 0.6 / 0.3 / 0.1 of the type's base speed, floored at 3 km/h (grade 1 → 5).**

Measured with the change's probe on a Bremen database imported from `bremen-latest.osm.pbf`, one kilometre
of `highway_unclassified` (base 50 km/h), seconds:

| profile | grade 1 | 2 | 3 | 4 | 5 |
|---|---|---|---|---|---|
| without the flag (today) | 72.0 | 72.0 | 72.0 | 72.0 | 72.0 |
| 1 / .85 / .6 / .3 / .1 | 72.0 | 84.7 | 120.0 | 240.0 | 720.0 |
| 1 / .9 / .75 / .5 / .3 | 72.0 | 80.0 | 96.0 | 144.0 | 240.0 |
| 1 / 1 / .85 / .6 / .4 | 72.0 | 72.0 | 84.7 | 120.0 | 180.0 |
| 1 / 1 / .9 / .75 / .5 | 72.0 | 72.0 | 80.0 | 96.0 | 144.0 |

The flag's intent is "avoid unpaved", so the first row is chosen: a grade 4 road costs three and a third
times a solid one, a grade 5 road ten times. No route was lost across the 32 routable pairs of the probe's
grid, so the aggressive table does not turn "avoid" into "cannot route" on real data. The floor keeps a
degraded road routable at all.

**Rejected: a grade-blind cost multiplier.** Multiplying a whole route's cost does not express "avoid this
way"; the grade is per object, not per route.

**Rejected: speed 0 for grade 5.** It makes the type unusable, which is a different statement from "avoid",
and it shrinks the profile's slowest type to zero — the shape that produced `NoRouteFound` above.

## Consequence worth recording

The bridge's car speed map has no `highway_track` entry, so `ParametrizeForCar` logs
`No speed for type 'highway_track'` and every track type keeps a zero speed: tracks are not car-routable at
all, with or without the flag. For a car, `avoidUnpaved` therefore acts on roads whose `surface`/`tracktype`
tags give them a grade — the measurement in the spec uses `highway_unclassified` for that reason. Making
tracks car-routable is a separate decision (it changes which roads the car profile uses) and is not part of
this change.

## Verification

`evidence/AvoidUnpavedProbe.cc` measures the object-level cost per grade through
`RoutingProfile::GetTime(Way, Distance)` and sweeps a coordinate grid for a route that changes. The date
stamped log beside it is the run of the tables above. The probe is built against the submodule's host build
(`hostbuild/libosmscout/libosmscout.so`) and runs without a device.

The spec scenario asserts the cost ordering, not a particular route: across 32 routable pairs of that grid
none of the four tables changed a route, which is a fact about dense city data, not about the flag.
