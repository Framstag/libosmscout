package com.framstag.libosmscout.client;

/**
 * Callback interface for asynchronous route calculation.
 * <p>
 * Passed to {@link OSMScoutClient#calculateRouteAsync(double, double, double, double, RouteCallback)}.
 * Methods are invoked from the JNI native thread — implementations MUST
 * marshal to the UI thread (e.g. using Platform.runLater).
 */
public interface RouteCallback {

    /**
     * The routing layer could not name the cause of the failure.
     */
    int KIND_UNKNOWN = 0;

    /**
     * No routing data could be used for the calculation: no database is loaded, or the routing
     * service could not open the loaded ones.
     */
    int KIND_ROUTING_DATA_UNAVAILABLE = 1;

    /**
     * The start position could not be attached to the road network.
     */
    int KIND_START_UNROUTABLE = 2;

    /**
     * The target position could not be attached to the road network.
     */
    int KIND_TARGET_UNROUTABLE = 3;

    /**
     * The two positions are not connected in the loaded routing data.
     * <p>
     * There is deliberately no "route too long" kind. The router reports one error for a search that
     * exhausted its alternatives, and a caller cannot tell from it whether the connection is longer
     * than the profile's cost limit allows or the endpoints' parts of the network do not join: measured
     * 2026-10-13, a 416-point sweep of Dortmund's old town produced 24 such failures that had all hit
     * the cost limit, on trips of at most 1.2 km. Both cases are reported as this kind.
     */
    int KIND_NO_ROUTE_FOUND = 4;

    /**
     * The routing data could not be read (a route node, area or way failed to load).
     */
    int KIND_ROUTING_DATA_FAILURE = 5;

    /**
     * The native routing work failed outside the router itself.
     */
    int KIND_ENGINE_FAILURE = 6;

    /**
     * Called periodically during route calculation.
     *
     * @param percent progress percentage (0-100)
     */
    void onProgress(int percent);

    /**
     * Called when route calculation completes successfully.
     *
     * @param route the computed route with geometry and metadata
     */
    void onSuccess(RouteEntry route);

    /**
     * Called when route calculation fails.
     *
     * @param message human-readable error description
     */
    void onError(String message);

    /**
     * Called when route calculation fails, with the kind of failure the routing layer determined.
     * <p>
     * The kind is one of the {@code KIND_*} constants of this interface; it names the cause, and a
     * caller that shows a message is expected to select the wording for the kind from its own
     * resources in the user's language — {@code detail} is an English description for a log, not
     * display text. A kind this interface does not define (a newer native library, or one that knows
     * a cause this version does not) MUST be treated as {@link #KIND_UNKNOWN}.
     * <p>
     * The default implementation ignores the kind and delegates to {@link #onError(String)}, so an
     * implementation written before this method existed keeps working unchanged.
     *
     * @param kind one of the {@code KIND_*} constants
     * @param detail human-readable description of the failure, for a log
     * @param nearestRoadDistanceMeters how far the failing end of the route is from the nearest road
     *     the router could use, in meters, or {@link Double#NaN} when the calculation did not fail
     *     at one end of the route or the router found no such road. A caller may state it in the
     *     wording for {@link #KIND_START_UNROUTABLE} and {@link #KIND_TARGET_UNROUTABLE}; a
     *     non-finite or non-positive value means the distance is unknown.
     */
    default void onErrorDetail(int kind, String detail, double nearestRoadDistanceMeters) {
        onError(detail);
    }

    /**
     * Called when route calculation is cancelled by the user.
     */
    void onCancel();
}
