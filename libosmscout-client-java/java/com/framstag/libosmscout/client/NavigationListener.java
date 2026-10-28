package com.framstag.libosmscout.client;

/**
 * Callback interface for navigation events.
 * <p>
 * All methods are invoked from the native navigation thread. Implementations
 * MUST marshal to the UI thread before touching JavaFX widgets.
 */
public interface NavigationListener {

    /**
     * Called when the navigation engine produces a new position estimate.
     *
     * @param position estimated vehicle position
     */
    default void onPositionEstimate(NavigationPosition position) {
    }

    /**
     * Called when the vehicle leaves the planned route and a reroute is needed.
     *
     * @param lat   current latitude
     * @param lon   current longitude
     * @param bearing current bearing in degrees, or {@code Double.NaN}
     * @param destLat destination latitude
     * @param destLon destination longitude
     */
    default void onRerouteRequest(double lat,
                                  double lon,
                                  double bearing,
                                  double destLat,
                                  double destLon) {
    }

    /**
     * Called when the destination is approached or reached.
     *
     * @param bearing bearing from current position to destination, or {@code Double.NaN}
     * @param distance remaining distance to destination in meters
     */
    default void onTargetReached(double bearing, double distance) {
    }

    /**
     * Called when the estimated arrival time changes.
     *
     * @param arrivalEstimate epoch milliseconds of estimated arrival
     * @param remainingDistance remaining route distance in meters
     */
    default void onArrivalEstimate(long arrivalEstimate, double remainingDistance) {
    }

    /**
     * Called with the current vehicle speed computed from GPS updates.
     *
     * @param speedKmH speed in km/h, or negative if unknown
     */
    default void onCurrentSpeed(double speedKmH) {
    }

    /**
     * Called when the maximum allowed speed on the current road changes.
     *
     * @param maxSpeedKmH maximum allowed speed in km/h, or negative if unknown
     */
    default void onMaxAllowedSpeed(double maxSpeedKmH) {
    }

    /**
     * Called when lane guidance for the next manoeuvre changes.
     *
     * @param oneway      whether the road is oneway
     * @param count       number of lanes
     * @param suggested   whether suggested lanes are known
     * @param suggestedFrom first suggested lane, inclusive (0 when not suggested)
     * @param suggestedTo   last suggested lane, inclusive (0 when not suggested)
     * @param turn          turn to take as a string, or empty
     * @param turns         per-lane turn indications, one element per lane; empty if unknown
     */
    default void onLaneUpdate(boolean oneway,
                              int count,
                              boolean suggested,
                              int suggestedFrom,
                              int suggestedTo,
                              String turn,
                              LaneTurn[] turns) {
    }

    /**
     * Called when a voice instruction should be played.
     *
     * @param samples list of voice sample identifiers
     */
    default void onVoiceInstruction(int[] samples) {
    }

    /**
     * Called when an error occurs in the navigation thread.
     *
     * @param message human-readable error description
     */
    default void onError(String message) {
    }

    /**
     * Called when the full list of route steps changes (new route or
     * route recalculation).
     * <p>
     * The steps are the same structured record a calculated route publishes
     * ({@link RouteEntry#steps}), in route order. They carry values only: no
     * description, no short description and no turn type name. A surface words
     * every manoeuvre itself, in its own language, from the step's kind, move,
     * exit ordinal, street name and way type.
     * <p>
     * The list never contains a {@code null} element: when the bridge cannot
     * build a step it delivers no list at all rather than a partial one.
     *
     * @param steps all route steps for the current route
     */
    default void onRouteSteps(RouteStep[] steps) {
    }

    /**
     * Called on every position update with the step of the next manoeuvre.
     * <p>
     * This is the primary callback for live turn-by-turn guidance. The step
     * carries its manoeuvre records plus the distance and time remaining to the
     * manoeuvre, and - when the following manoeuvre is close enough to be
     * announced - that manoeuvre as a nested {@link RouteStep} in
     * {@link RouteStep#nextNext}.
     *
     * @param step the step ahead
     */
    default void onNextRouteStep(RouteStep step) {
    }
}
