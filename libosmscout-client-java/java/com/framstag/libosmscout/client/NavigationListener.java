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
     * Called when the full list of route instructions changes (new route or
     * route recalculation).
     *
     * @param instructions all route instructions for the current route
     */
    default void onRouteInstructions(RouteInstruction[] instructions) {
    }

    /**
     * Called on every position update with the next manoeuvre instruction.
     * <p>
     * This is the primary callback for live turn-by-turn guidance. The UI
     * should update the next-turn display with the instruction's distance,
     * turn type, and street name.
     *
     * @param instruction the next route instruction
     */
    default void onNextRouteInstruction(RouteInstruction instruction) {
    }
}
