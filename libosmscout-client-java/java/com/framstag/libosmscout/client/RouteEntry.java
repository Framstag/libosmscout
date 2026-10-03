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

    /**
     * Latitude of each description line's manoeuvre, in degrees.
     * <p>
     * Index-aligned with the instruction lines of {@link #descriptions} — the
     * {@code "--- Route ---"} header is not an instruction and is not counted, so
     * entry {@code i} belongs to the {@code i}-th line that is not a header. The
     * array is {@code null} when the native side could not align its positions with
     * the description lines, in which case no per-manoeuvre position is available.
     */
    public double[] instructionLats;

    /**
     * Longitude of each description line's manoeuvre, in degrees.
     * <p>
     * Index-aligned with {@link #instructionLats}; {@code null} under the same
     * condition.
     */
    public double[] instructionLons;

    /** Opaque handle for starting live navigation on this route. Zero if navigation is not available. */
    public long routeHandle;

    /** Default constructor. */
    public RouteEntry() {
    }
}
