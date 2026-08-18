package com.framstag.libosmscout.client;

/**
 * Represents a single POI search result.
 * <p>
 * Returned by {@link OSMScoutClient#searchPOIs(String, double, double, double, int)}.
 */
public class PoiEntry {

    /** Display label for the POI (e.g. "Hotel Central"), or empty if unnamed. */
    public String label;

    /** OSM object type (e.g. "tourism_hotel", "amenity_restaurant", "shop"). */
    public String objectType;

    /** Latitude in degrees. */
    public double lat;

    /** Longitude in degrees. */
    public double lon;

    /** Distance from the search center in meters. */
    public double distance;

    /** Default constructor. */
    public PoiEntry() {
    }
}
