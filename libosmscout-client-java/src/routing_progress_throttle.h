/*
 * routing_progress_throttle.h — pure decision for the rate-limited routing
 * progress reports the JNI bridge hands to Java (openspec change
 * "client-java-routing-progress-rate-limit").
 *
 * The router calls RoutingProgress::Progress once per successfully relaxed
 * edge — thousands to millions of times for a long route — and every call would
 * cross JNI from the routing worker thread into a Java callback. A report is
 * worth handing over only when the percentage has *changed*, and at most once
 * per rate-limit interval, so the callback means "the progress moved" instead
 * of "a node was visited".
 *
 * The decision is a pure function of the reported percentage and the clock, so
 * it lives here and is host-tested; the distance-to-percentage conversion is
 * here as well because the cap below completion belongs to the reported value.
 * The JNI attach and the Java call stay in OSMScoutClient.cpp and happen only
 * for an accepted report, so a dropped report costs no JNI crossing.
 *
 * Kept dependency-free on purpose (no libosmscout includes, no JNI) so it can
 * be unit-tested on the host. The caller injects the time, which is what makes
 * the rule testable without a routing engine or a real clock.
 */
#ifndef NAVIVEYLIN_ROUTING_PROGRESS_THROTTLE_H
#define NAVIVEYLIN_ROUTING_PROGRESS_THROTTLE_H

#include <chrono>

namespace naviveylin {

/** Minimum interval between two progress reports handed to Java. */
inline constexpr std::chrono::milliseconds kMinProgressReportInterval{100};

/**
 * Highest percentage a progress report may carry. Completion is not progress:
 * it is delivered as the calculation's result, so 100 % stays reserved for the
 * route that actually arrived.
 */
inline constexpr int kMaxProgressPercent = 99;

/**
 * Percentage of the route the router has covered, capped below completion.
 * Returns 0 while the overall distance is unknown (0) or negative, so the
 * conversion cannot divide by zero or report a negative percentage.
 */
inline int ProgressPercent(double currentMaxDistanceMeters,
                           double overallDistanceMeters)
{
  if (!(overallDistanceMeters > 0.0)) {
    return 0;
  }

  int percent = static_cast<int>(
      currentMaxDistanceMeters / overallDistanceMeters * 100.0);

  if (percent > kMaxProgressPercent) {
    return kMaxProgressPercent;
  }

  return percent;
}

/**
 * Decides which of the percentages the router reports are handed to Java.
 *
 * State is the last handed-over percentage and when it was handed over. The
 * first report of a calculation is accepted without waiting for the interval;
 * afterwards a report is accepted only when the percentage is greater than the
 * last handed-over one and at least one interval has passed since that
 * handover. A dropped report changes nothing, so the next accepted report still
 * compares against the last handed-over value and the interval is measured from
 * the last handover, not from the last call.
 *
 * The accepted sequence is therefore non-decreasing, and one calculation hands
 * over at most one report per interval. Intermediate percentages are dropped,
 * not queued: progress is a sampled indicator, not a log.
 */
class RoutingProgressThrottle
{
private:
  bool reported{false};                             //!< Whether a report was handed over already
  int lastPercent{0};                               //!< Last handed-over percentage
  std::chrono::steady_clock::time_point lastReport; //!< When that percentage was handed over

public:
  /**
   * Returns true when the report of `percent` at `now` is handed over, and
   * records it, or false when it is dropped and nothing changes.
   */
  bool ShouldReport(int percent, std::chrono::steady_clock::time_point now)
  {
    if (reported && percent <= lastPercent) {
      return false;
    }

    if (reported && (now - lastReport) < kMinProgressReportInterval) {
      return false;
    }

    reported = true;
    lastPercent = percent;
    lastReport = now;

    return true;
  }
};

} // namespace naviveylin

#endif // NAVIVEYLIN_ROUTING_PROGRESS_THROTTLE_H
