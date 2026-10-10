/*
 * route_step_time.h — formatting of the per-step time column of a route
 * description line (openspec change
 * "client-java-route-instruction-time-format").
 *
 * A description line carries the distance and the time of one step between a
 * pair of brackets. The time used to be printed in minutes only, so a step
 * shorter than a minute showed "0 min" and every step of a city route looked
 * like it took no time, although the router had estimated one. The time is
 * therefore printed in the unit that fits its magnitude: hours and remaining
 * minutes from an hour, whole minutes from a minute, whole seconds from a
 * second, and no time part at all below a second (a "0 s" would claim a value
 * the line does not have).
 *
 * The rule is display-only and lives here as a pure function so both
 * description paths (calculate and async calculate / reroute) share it and
 * cannot drift, and so a host test can exercise it without a routing engine.
 * Kept dependency-free on purpose (no libosmscout includes, no JNI).
 *
 * The per-step numeric values published on the route and the instruction are a
 * different concern and are not derived from this text.
 */
#ifndef NAVIVEYLIN_ROUTE_STEP_TIME_H
#define NAVIVEYLIN_ROUTE_STEP_TIME_H

#include <chrono>
#include <optional>
#include <string>

namespace naviveylin {

/**
 * Time part of a description line for a step of `stepTime`, or nothing when
 * the step carries no time of its own.
 *
 * Returns nullopt below one whole second — including a zero and a negative
 * difference — and the formatted time otherwise. The result is a value the
 * caller appends after the distance, separated from it by a comma only when
 * both parts are present.
 */
inline std::optional<std::string> FormatRouteStepTime(std::chrono::seconds stepTime)
{
  if (stepTime.count() < 1) {
    return std::nullopt;
  }

  auto dtM = std::chrono::duration_cast<std::chrono::minutes>(stepTime);

  if (dtM.count() >= 60) {
    auto dtH = std::chrono::duration_cast<std::chrono::hours>(stepTime);
    auto dtRem = std::chrono::duration_cast<std::chrono::minutes>(stepTime - dtH);

    return std::to_string(dtH.count()) + " h " + std::to_string(dtRem.count()) + " min";
  }

  if (dtM.count() >= 1) {
    return std::to_string(dtM.count()) + " min";
  }

  // A step shorter than a minute used to print "0 min" (owner finding,
  // 2026-10-03). Seconds keep the per-step time meaningful below one minute;
  // do not reduce this branch back to minutes.
  return std::to_string(stepTime.count()) + " s";
}

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTE_STEP_TIME_H
