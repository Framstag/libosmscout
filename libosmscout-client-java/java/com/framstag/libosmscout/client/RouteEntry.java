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

    /** Turn-by-turn route description lines. */
    public String[] descriptions;

    /** Opaque handle for starting live navigation on this route. Zero if navigation is not available. */
    public long routeHandle;

    /** Default constructor. */
    public RouteEntry() {
    }
}
