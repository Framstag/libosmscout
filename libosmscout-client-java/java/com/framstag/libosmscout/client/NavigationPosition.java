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

    /** Name of the resolved way the vehicle is on (route way on-route, nearest routable way off-route), or empty if unknown. */
    public final String wayName;

    /** Ref tag of the resolved way the vehicle is on (e.g. "B 1"), or empty if unknown. */
    public final String wayRef;

    /** OSM type name of the resolved way (e.g. "highway_residential"), or empty if unknown. */
    public final String wayType;

    /**
     * Create a navigation position estimate.
     *
     * @param state     navigation state
     * @param lat       estimated latitude in degrees
     * @param lon       estimated longitude in degrees
     * @param bearing   estimated bearing in degrees, or NaN
     * @param accuracy  horizontal accuracy in meters, or negative
     * @param wayName   name of the resolved way, or empty
     * @param wayRef    ref tag of the resolved way, or empty
     * @param wayType   OSM type name of the resolved way, or empty
     */
    public NavigationPosition(NavigationState state,
                              double lat,
                              double lon,
                              double bearing,
                              double accuracy,
                              String wayName,
                              String wayRef,
                              String wayType) {
        this.state = state;
        this.lat = lat;
        this.lon = lon;
        this.bearing = bearing;
        this.accuracy = accuracy;
        this.wayName = wayName != null ? wayName : "";
        this.wayRef = wayRef != null ? wayRef : "";
        this.wayType = wayType != null ? wayType : "";
    }

    /**
     * Create a navigation position estimate without way info.
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
        this(state, lat, lon, bearing, accuracy, "", "", "");
    }
}
