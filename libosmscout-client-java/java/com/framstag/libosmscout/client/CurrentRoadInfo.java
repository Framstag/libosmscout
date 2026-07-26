package com.framstag.libosmscout.client;

/**
 * Information about the road the vehicle is currently on during navigation.
 * <p>
 * Bridged from C++ {@code RouteDescription::NameDescription} and
 * {@code RouteDescription::TypeNameDescription} via JNI. Each field is
 * only set if the data is available from the OSM way.
 */
public class CurrentRoadInfo {

    /** Road reference, e.g. "A40", "B1", or empty if unknown. */
    public final String ref;

    /** Road type name, e.g. "motorway", "primary", "residential", or empty if unknown. */
    public final String typeName;

    /** Road name, e.g. "Ruhrschnellweg", "Hauptstrasse", or empty if unknown. */
    public final String name;

    /**
     * Create a current road info object.
     *
     * @param ref      road reference, or empty
     * @param typeName road type name, or empty
     * @param name     road name, or empty
     */
    public CurrentRoadInfo(String ref, String typeName, String name) {
        this.ref = ref != null ? ref : "";
        this.typeName = typeName != null ? typeName : "";
        this.name = name != null ? name : "";
    }

    /**
     * Whether any road information is available.
     *
     * @return true if at least one field is non-empty
     */
    public boolean hasInfo() {
        return !ref.isEmpty() || !typeName.isEmpty() || !name.isEmpty();
    }

    /**
     * Return a human-readable one-line summary.
     * Format: "[ref] [typeName] [name]" with only the available parts.
     */
    public String toDisplayString() {
        StringBuilder sb = new StringBuilder();
        if (!ref.isEmpty()) {
            sb.append(ref);
        }
        if (!typeName.isEmpty()) {
            if (sb.length() > 0) sb.append(" ");
            sb.append(typeName);
        }
        if (!name.isEmpty()) {
            if (sb.length() > 0) sb.append(" ");
            sb.append(name);
        }
        return sb.toString();
    }

    @Override
    public String toString() {
        return "CurrentRoadInfo{"
            + "ref='" + ref + '\''
            + ", typeName='" + typeName + '\''
            + ", name='" + name + '\''
            + '}';
    }
}
