package com.framstag.libosmscout.client;

import java.util.HashMap;
import java.util.Map;

/**
 * A single favorite location with a name, geographic coordinate,
 * and an extensible attribute map for future fields.
 */
public class FavoriteLocation {

    /** Display name for this favorite. */
    public String name;

    /** Latitude in degrees. */
    public double lat;

    /** Longitude in degrees. */
    public double lon;

    /** Extensible attribute map for future fields. */
    public Map<String, String> attributes;

    /** Default constructor. */
    public FavoriteLocation() {
        this.attributes = new HashMap<>();
    }

    /**
     * Construct a favorite location with the given name and coordinates.
     *
     * @param name display name
     * @param lat  latitude in degrees
     * @param lon  longitude in degrees
     */
    public FavoriteLocation(String name, double lat, double lon) {
        this.name = name;
        this.lat = lat;
        this.lon = lon;
        this.attributes = new HashMap<>();
    }
}
