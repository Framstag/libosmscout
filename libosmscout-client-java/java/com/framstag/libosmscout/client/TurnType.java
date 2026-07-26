package com.framstag.libosmscout.client;

import java.util.HashMap;
import java.util.Locale;
import java.util.Map;

/**
 * Turn direction for a route instruction.
 * <p>
 * Mirrors {@code RouteDescription::DirectionDescription::Move} from the C++
 * navigation engine. Values are serialised as camelCase strings across the
 * JNI bridge and resolved case-insensitively via {@link #fromString(String)}.
 */
public enum TurnType {

    /** Sharp left turn (greater than about 135 degrees). */
    SHARP_LEFT,
    /** Normal left turn (about 90 degrees). */
    LEFT,
    /** Slight left turn (less than about 45 degrees). */
    SLIGHTLY_LEFT,
    /** Continue straight, no turn. */
    STRAIGHT_ON,
    /** Slight right turn (less than about 45 degrees). */
    SLIGHTLY_RIGHT,
    /** Normal right turn (about 90 degrees). */
    RIGHT,
    /** Sharp right turn (greater than about 135 degrees). */
    SHARP_RIGHT,
    /** Route start point. */
    START,
    /** Destination reached. */
    TARGET_REACHED,
    /** Entering a roundabout. */
    ROUNDABOUT_ENTER,
    /** Leaving a roundabout. */
    ROUNDABOUT_LEAVE,
    /** Entering a motorway. */
    MOTORWAY_ENTER;

    /** Lookup map for case-insensitive turn type resolution. */
    private static final Map<String, TurnType> LOOKUP = new HashMap<>();

    static {
        for (TurnType t : values()) {
            // Register both UPPER_CASE and camelCase
            LOOKUP.put(t.name(), t);
            LOOKUP.put(t.name().toLowerCase(Locale.ROOT), t);
            // Also register camelCase variants like "sharpLeft"
            String[] parts = t.name().toLowerCase(Locale.ROOT).split("_");
            StringBuilder camel = new StringBuilder(parts[0]);
            for (int i = 1; i < parts.length; i++) {
                camel.append(Character.toUpperCase(parts[i].charAt(0)))
                     .append(parts[i].substring(1));
            }
            LOOKUP.put(camel.toString(), t);
        }
    }

    /**
     * Look up a TurnType by its camelCase or UPPER_CASE name.
     *
     * @param name the turn type string from the C++ engine
     * @return the matching TurnType, or {@code STRAIGHT_ON} if unknown
     */
    public static TurnType fromString(String name) {
        if (name == null) return STRAIGHT_ON;
        TurnType t = LOOKUP.get(name);
        return t != null ? t : STRAIGHT_ON;
    }
}
