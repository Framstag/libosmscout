package com.framstag.libosmscout.client;

/**
 * Road resolved at a position by the bearing-aware lookup
 * ({@link OSMScoutClient#getRoadAt(double, double, double)}).
 * <p>
 * Carries the name, ref, type, and maximum allowed speed of the road the
 * vehicle is actually driving on, so the street label and the speed-limit
 * sign always describe the same road.
 */
public class RoadInfo {

    /** Street name, or empty when the road has no name tag. */
    public final String name;

    /** Ref tag (e.g. "B 1", "A 44"), or empty when the road has none. */
    public final String ref;

    /** OSM type name of the road (e.g. "highway_residential"). */
    public final String typeName;

    /** Maximum allowed speed in km/h, or NaN when undefined. */
    public final double maxSpeedKmH;

    /**
     * Create a road info.
     *
     * @param name       street name, or empty
     * @param ref        ref tag, or empty
     * @param typeName   OSM type name
     * @param maxSpeedKmH maximum allowed speed in km/h, or NaN
     */
    public RoadInfo(String name, String ref, String typeName, double maxSpeedKmH) {
        this.name = name != null ? name : "";
        this.ref = ref != null ? ref : "";
        this.typeName = typeName != null ? typeName : "";
        this.maxSpeedKmH = maxSpeedKmH;
    }

    /** Whether any displayable identity (name or ref) is present. */
    public boolean hasInfo() {
        return !name.isEmpty() || !ref.isEmpty();
    }
}
