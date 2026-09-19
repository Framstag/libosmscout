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

    /**
     * Match quality of the admin region component: "match", "candidate" or "none".
     * <p>
     * Note that the search scope (default admin region) is reported as
     * "match" for every entry inside it, even when the query never named that
     * region — callers must not treat this value as evidence that the query
     * named the region.
     */
    public String adminRegionMatchQuality;

    /** Match quality of the postal area component: "match", "candidate" or "none". */
    public String postalAreaMatchQuality;

    /** Match quality of the location (street/place name) component: "match", "candidate" or "none". */
    public String locationMatchQuality;

    /** Match quality of the house number component: "match", "candidate" or "none". */
    public String addressMatchQuality;

    /** Match quality of the POI component: "match", "candidate" or "none". */
    public String poiMatchQuality;

    /** True when the entry carries a house number. */
    public boolean hasHouseNumber;

    /**
     * Name of the component that supplied {@link #label}: for a house-level
     * entry the street name (the label itself is street + house number), for a
     * location/POI/region entry the entry's own name. Null when unknown.
     */
    public String matchedName;

    /**
     * Which component supplied {@link #matchedName} and therefore which of the
     * per-attribute quality fields carries its match quality: one of
     * "adminRegion", "location", "poi", "address". Null when unknown.
     * <p>
     * Needed because the label and {@link #objectType} are derived from
     * different component precedences, so the quality field to consult cannot
     * be inferred from either.
     */
    public String matchedComponent;

    /** Default constructor. */
    public LocationEntry() {
    }
}
