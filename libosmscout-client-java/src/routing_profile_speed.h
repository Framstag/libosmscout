/*
 * routing_profile_speed.h — pure helper for the speed a routing profile is
 * parametrized with (change "client-java-routing-profile-max-speed").
 *
 * The JNI bridge used to hardcode the vehicle's speed (car 120, bicycle 30,
 * pedestrian 10 km/h), so a caller that knows the vehicle it actually routes
 * for had no way to say so, and the speed decides both the chosen route and the
 * reported duration. A RoutingProfile now carries the vehicle's usual maximum
 * speed in km/h, with 0 meaning "use the routing engine's built-in profile".
 *
 * A requested value is only usable as a speed when it is a finite number in
 * 1-300 km/h. 0 is the documented "built-in profile" request, a negative or
 * non-finite value is not a speed, and a value above 300 km/h is above every
 * road vehicle: the engine's parametrization is a speed map of way types, and
 * an unusable ceiling there changes nothing meaningful while an unusable flat
 * speed would make the vehicle arbitrarily fast. The decision is therefore
 * "usable value, else the vehicle's built-in default", never a failure: the
 * bridge has no error channel for it and a wrong speed is better than a refused
 * calculation.
 *
 * The header also holds the per-grade speed table a profile is re-priced with when
 * its `avoidUnpaved` flag is set (change "client-java-avoid-unpaved"): a helper
 * that turns a type's base speed into its five grade speeds, so the table and its
 * floor can be pinned on the host rather than only observed through a built map.
 *
 * Kept dependency-free on purpose (no JNI, no libosmscout includes) so it can be
 * unit-tested on the host — see Tests/src/RoutingProfileSpeedTest.cpp.
 */
#ifndef NAVIVEYLIN_ROUTING_PROFILE_SPEED_H
#define NAVIVEYLIN_ROUTING_PROFILE_SPEED_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace naviveylin {

// The speed a routing profile is parametrized with, in km/h. Returns
// requestedKmh when it is a usable speed (a finite value in 1..300), else
// builtInKmh, the vehicle's built-in default.
inline double ResolveProfileMaxSpeedKmh(double requestedKmh, double builtInKmh)
{
  if (!std::isfinite(requestedKmh) ||
      requestedKmh < 1.0 ||
      requestedKmh > 300.0) {
    return builtInKmh;
  }

  return requestedKmh;
}

// Speed factor applied to a type's base speed per grade (index 0..4 meaning grade 1..5) when
// `avoidUnpaved` is requested. Solid and gravel keep most of their type's speed; unpaved and worse
// lose most of it.
//
// A degraded road must stay routable: the parametrization prices a profile's cost limit from its
// slowest road type, so a table close to zero makes the limit unreachable and the calculation
// reports "no route" instead of a detour. See Tests/src/RoutingProfileSpeedTest.cpp.
inline constexpr double kAvoidUnpavedGradeFactor[5] = {1.0, 0.85, 0.6, 0.3, 0.1};

// Floor for a degraded road, so avoiding an unpaved way never means being unable to route.
inline constexpr double kAvoidUnpavedMinSpeedKmh = 3.0;

// The five per-grade speeds a routable type with [baseSpeedKmh] is priced with when `avoidUnpaved`
// is requested: grade i (1..5) keeps kAvoidUnpavedGradeFactor[i-1] of the base speed, floored at
// kAvoidUnpavedMinSpeedKmh.
inline std::array<double,5> AvoidUnpavedGradeSpeedsKmh(double baseSpeedKmh)
{
  std::array<double,5> speeds{};

  for (size_t i=0; i<5; i++) {
    speeds[i]=std::max(kAvoidUnpavedMinSpeedKmh, baseSpeedKmh*kAvoidUnpavedGradeFactor[i]);
  }

  return speeds;
}

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTING_PROFILE_SPEED_H
