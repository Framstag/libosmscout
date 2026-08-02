package com.framstag.libosmscout.client;

/**
 * Represents a single location search result.
 * <p>
 * Returned by {@link OSMScoutClient#searchLocations(String, int)}.
 */
public class LocationEntry {

    /** Display label for the location (e.g. "Am Birkenbaum"). */
    public String label;

    /** Type string: "coordinate" or "object". */
    public String type;

    /** OSM object type (e.g. "place_town", "address", "highway_bus_stop"). */
    public String objectType;

    /** Latitude in degrees. */
    public double lat;

    /** Longitude in degrees. */
    public double lon;

    /** Admin region hierarchy (from most specific to broadest). */
    public String[] region;

    /** Postal area name (e.g. "44339"), or empty string. */
    public String postalArea;

    /** Full admin region hierarchy as a single path string (e.g. "Eving/Dortmund/..."). */
    public String adminRegionHierarchy;

    /** OSM type name of the referenced object (e.g. "building", "highway_residential"). */
    public String objectTypeName;

    /** OSM name of the referenced object (e.g. "Aldi", "Hauptstraße"), or empty if unknown. */
    public String name;

    /** File offset of the referenced object in the database. */
    public long objectFileOffset;

    /** Match quality: "match" or "candidate". */
    public String matchQuality;

    /** Ref type: "node", "way", "area", or null. */
    public String refType;

    /** Default constructor. */
    public LocationEntry() {
    }
}
