package com.framstag.libosmscout.client;

/**
 * A single entry in a structured object description.
 * <p>
 * Each entry belongs to a section (and optionally a subsection),
 * may have an index for repeated subsections, and carries a
 * label/value pair.
 * <p>
 * Returned as part of {@link ObjectDescription} from
 * {@link OSMScoutClient#getDescription(double, double, int)}.
 */
public class DescriptionEntry {

    /** Section name (e.g. "General", "Location", "Contact"). */
    public String sectionKey;

    /** Optional subsection name, empty if none. */
    public String subsectionKey;

    /** Whether this entry has an index (for repeated subsections). */
    public boolean hasIndex;

    /** Index value when {@link #hasIndex} is true. */
    public int index;

    /** Label key (e.g. "Name", "Phone", "Website"). */
    public String labelKey;

    /** Value as a display string. */
    public String value;

    /** Default constructor. */
    public DescriptionEntry() {
    }
}
