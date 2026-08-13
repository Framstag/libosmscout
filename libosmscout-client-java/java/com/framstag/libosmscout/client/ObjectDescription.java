package com.framstag.libosmscout.client;

import java.util.Collections;
import java.util.List;

/**
 * Structured description of an OSM map object (Node, Way, or Area).
 * <p>
 * Returned by {@link OSMScoutClient#getDescription(double, double, int)}.
 * Contains a list of {@link DescriptionEntry} instances that together
 * form a structured view of the object's properties, organised by
 * section/subsection, plus the geographic coordinates of the object.
 * <p>
 * The structure mirrors the C++ {@code osmscout::ObjectDescription} class.
 */
public class ObjectDescription {

    /** List of description entries, ordered as returned by native DescriptionService. */
    private final List<DescriptionEntry> entries;

    /** Latitude of the described object, or NaN if unknown. */
    private final double objectLat;

    /** Longitude of the described object, or NaN if unknown. */
    private final double objectLon;

    /**
     * Construct an ObjectDescription with the given entries and no object location.
     *
     * @param entries list of description entries, may be empty
     */
    public ObjectDescription(List<DescriptionEntry> entries) {
        this(entries, Double.NaN, Double.NaN);
    }

    /**
     * Construct an ObjectDescription with the given entries and object location.
     *
     * @param entries   list of description entries, may be empty
     * @param objectLat latitude of the described object
     * @param objectLon longitude of the described object
     */
    public ObjectDescription(List<DescriptionEntry> entries, double objectLat, double objectLon) {
        this.entries = entries != null ? entries : List.of();
        this.objectLat = objectLat;
        this.objectLon = objectLon;
    }

    /**
     * Return the list of description entries.
     * <p>
     * Entries are ordered as returned by the native {@code DescriptionService}.
     *
     * @return unmodifiable list of entries, never null
     */
    public List<DescriptionEntry> getEntries() {
        return Collections.unmodifiableList(entries);
    }

    /**
     * Return the latitude of the described object.
     *
     * @return latitude in degrees, or NaN if unknown
     */
    public double getObjectLat() {
        return objectLat;
    }

    /**
     * Return the longitude of the described object.
     *
     * @return longitude in degrees, or NaN if unknown
     */
    public double getObjectLon() {
        return objectLon;
    }
}
