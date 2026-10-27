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

    /**
     * Distance in meters of the leg that <em>ends</em> at each description line's manoeuvre.
     * <p>
     * Index-aligned with {@link #instructionLats} and with the instruction lines of
     * {@link #descriptions}. The value is the route distance from the previous instruction's
     * manoeuvre to this one, so the entries add up to the route's own total distance (the start
     * line owns a zero-length leg). It is never a distance between two route nodes that carry no
     * instruction, and the array is {@code null} under the same condition as
     * {@link #instructionLats}.
     */
    public double[] instructionDistances;

    /**
     * Estimated travel time in seconds of that same leg, index-aligned with
     * {@link #instructionDistances}, so the entries add up to the route's own total duration.
     * <p>
     * Zero for a leg whose travel time is below a second, and {@code null} under the same
     * condition as {@link #instructionLats}.
     */
    public double[] instructionTimes;

    /**
     * The route's manoeuvres as structured steps, in route order.
     * <p>
     * One entry per described route node. A step carries its node's manoeuvre data as published by
     * the client library's route step model ({@link RouteStep}), so a client reads no description
     * line to obtain it. The count of steps does not depend on the count of
     * {@link #descriptions}: a mismatch in one never discards the other. {@code null} only for a
     * result that carries no description at all.
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
