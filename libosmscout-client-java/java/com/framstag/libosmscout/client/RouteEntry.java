package com.framstag.libosmscout.client;

/**
 * Represents a computed route result.
 * <p>
 * Returned by {@link OSMScoutClient#calculateRouteAsync(double, double, double, double, RouteCallback)}.
 */
public class RouteEntry {

    /** Route waypoint latitudes in degrees, ordered from start to destination. */
    public double[] latitudes;

    /** Route waypoint longitudes in degrees, ordered from start to destination. */
    public double[] longitudes;

    /** Total route distance in meters. */
    public double distance;

    /** Estimated travel duration in seconds. */
    public double duration;

    /**
     * The route's manoeuvres as structured steps, in route order.
     * <p>
     * One entry per described route node. A step carries its node's manoeuvre data as published by
     * the client library's route step model ({@link RouteStep}), so a client reads no description
     * line to obtain it: the library publishes values, never prose (spec:
     * `turn-instruction-localization`). {@code null} only for a result that carries no
     * description at all.
     */
    public RouteStep[] steps;

    /**
     * Mapping from a step's database identity to the map directory it belongs to.
     * <p>
     * Lets a client resolve the database of each step when the route spans more than one map
     * region. Empty (never {@code null}) when the description carries no mapping.
     */
    public java.util.Map<Long, String> databaseMapping = new java.util.HashMap<>();

    /** Opaque handle for starting live navigation on this route. Zero if navigation is not available. */
    public long routeHandle;

    /** Default constructor. */
    public RouteEntry() {
    }
}
