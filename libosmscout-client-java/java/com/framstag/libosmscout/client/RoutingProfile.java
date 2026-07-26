package com.framstag.libosmscout.client;

/**
 * Routing profile configuration for route calculation.
 * <p>
 * Specifies the vehicle type and optional restrictions (avoid tolls, ferries, unpaved roads).
 * Passed to {@link OSMScoutClient#calculateRouteAsync(double, double, double, double, RoutingProfile, RouteCallback)}.
 */
public class RoutingProfile {

    /** Vehicle type for routing (default: CAR). */
    public final Vehicle vehicle;

    /** Whether to avoid toll roads. */
    public final boolean avoidTolls;

    /** Whether to avoid ferry routes. */
    public final boolean avoidFerries;

    /** Whether to avoid unpaved roads (relevant for bicycle). */
    public final boolean avoidUnpaved;

    /**
     * Create a default car routing profile with no avoid flags.
     */
    public RoutingProfile() {
        this(Vehicle.CAR, false, false, false);
    }

    /**
     * Create a routing profile with the given vehicle and default avoid flags.
     *
     * @param vehicle vehicle type
     */
    public RoutingProfile(Vehicle vehicle) {
        this(vehicle, false, false, false);
    }

    /**
     * Create a fully specified routing profile.
     *
     * @param vehicle      vehicle type
     * @param avoidTolls   true to avoid toll roads
     * @param avoidFerries true to avoid ferry routes
     * @param avoidUnpaved true to avoid unpaved roads
     */
    public RoutingProfile(Vehicle vehicle,
                          boolean avoidTolls,
                          boolean avoidFerries,
                          boolean avoidUnpaved) {
        this.vehicle = vehicle;
        this.avoidTolls = avoidTolls;
        this.avoidFerries = avoidFerries;
        this.avoidUnpaved = avoidUnpaved;
    }
}
