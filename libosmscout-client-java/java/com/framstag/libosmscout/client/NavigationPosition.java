package com.framstag.libosmscout.client;

/**
 * Estimated vehicle position produced by the navigation engine.
 */
public class NavigationPosition {

    /** Navigation state at the time of the estimate. */
    public final NavigationState state;

    /** Estimated latitude in degrees. */
    public final double lat;

    /** Estimated longitude in degrees. */
    public final double lon;

    /** Estimated bearing in degrees, or {@code Double.NaN} if unknown. */
    public final double bearing;

    /** Horizontal accuracy in meters, or negative if unknown. */
    public final double accuracy;

    /**
     * Create a navigation position estimate.
     *
     * @param state     navigation state
     * @param lat       estimated latitude in degrees
     * @param lon       estimated longitude in degrees
     * @param bearing   estimated bearing in degrees, or NaN
     * @param accuracy  horizontal accuracy in meters, or negative
     */
    public NavigationPosition(NavigationState state,
                              double lat,
                              double lon,
                              double bearing,
                              double accuracy) {
        this.state = state;
        this.lat = lat;
        this.lon = lon;
        this.bearing = bearing;
        this.accuracy = accuracy;
    }
}
