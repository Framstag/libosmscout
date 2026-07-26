package com.framstag.libosmscout.client;

import java.util.Collections;
import java.util.List;

/**
 * Structured description of an OSM map object (Node, Way, or Area).
 * <p>
 * Returned by {@link OSMScoutClient#getDescription(double, double)}.
 * Contains a list of {@link DescriptionEntry} instances that together
 * form a structured view of the object's properties, organised by
 * section/subsection.
 * <p>
 * The structure mirrors the C++ {@code osmscout::ObjectDescription} class.
 */
public class ObjectDescription {

    /** List of description entries, ordered as returned by native DescriptionService. */
    private final List<DescriptionEntry> entries;

    /**
     * Construct an ObjectDescription with the given entries.
     *
     * @param entries list of description entries, may be empty
     */
    public ObjectDescription(List<DescriptionEntry> entries) {
        this.entries = entries != null ? entries : List.of();
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
}
