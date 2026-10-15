package com.framstag.libosmscout.client;

/**
 * Routing profile configuration for route calculation.
 * <p>
 * Specifies the vehicle type, its usual maximum speed and optional restrictions (avoid tolls,
 * ferries, unpaved roads).
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
     * The vehicle's usual maximum speed in km/h, or 0 to use the routing engine's built-in profile
     * for the vehicle (car 120, bicycle 30, pedestrian 10 km/h).
     * <p>
     * The value is interpreted by the vehicle: for a car it is a ceiling applied on top of the
     * per-road-type speed table, for bicycle and pedestrian it is the flat speed used for every
     * routable way type.
     */
    public final double maxSpeedKmh;

    /**
     * Create a default car routing profile with no avoid flags and the built-in maximum speed.
     */
    public RoutingProfile() {
        this(Vehicle.CAR, false, false, false);
    }

    /**
     * Create a routing profile with the given vehicle, default avoid flags and the built-in
     * maximum speed.
     *
     * @param vehicle vehicle type
     */
    public RoutingProfile(Vehicle vehicle) {
        this(vehicle, false, false, false);
    }

    /**
     * Create a routing profile with the given vehicle and maximum speed, without avoid flags.
     *
     * @param vehicle     vehicle type
     * @param maxSpeedKmh the vehicle's usual maximum speed in km/h, 0 for the built-in profile
     */
    public RoutingProfile(Vehicle vehicle,
                          double maxSpeedKmh) {
        this(vehicle, false, false, false, maxSpeedKmh);
    }

    /**
     * Create a fully specified routing profile without a maximum speed.
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
        this(vehicle, avoidTolls, avoidFerries, avoidUnpaved, 0.0);
    }

    /**
     * Create a fully specified routing profile.
     *
     * @param vehicle      vehicle type
     * @param avoidTolls   true to avoid toll roads
     * @param avoidFerries true to avoid ferry routes
     * @param avoidUnpaved true to avoid unpaved roads
     * @param maxSpeedKmh  the vehicle's usual maximum speed in km/h, 0 for the built-in profile
     */
    public RoutingProfile(Vehicle vehicle,
                          boolean avoidTolls,
                          boolean avoidFerries,
                          boolean avoidUnpaved,
                          double maxSpeedKmh) {
        this.vehicle = vehicle;
        this.avoidTolls = avoidTolls;
        this.avoidFerries = avoidFerries;
        this.avoidUnpaved = avoidUnpaved;
        this.maxSpeedKmh = maxSpeedKmh;
    }
}
