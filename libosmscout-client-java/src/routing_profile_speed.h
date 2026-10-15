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
 * Kept dependency-free on purpose (no JNI, no libosmscout includes) so it can be
 * unit-tested on the host — see Tests/src/RoutingProfileSpeedTest.cpp.
 */
#ifndef NAVIVEYLIN_ROUTING_PROFILE_SPEED_H
#define NAVIVEYLIN_ROUTING_PROFILE_SPEED_H

#include <cmath>

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

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTING_PROFILE_SPEED_H
