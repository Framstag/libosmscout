package com.framstag.libosmscout;

/**
 * Parsed route instruction ready for web-like card rendering.
 * <p>
 * This is an extension point for optical navigation hints:
 * turn direction, road type, lane guidance, exit numbers, etc.
 */
public class RouteInstruction {

    /** Primary instruction text (e.g. "Left onto Main Street"). */
    public final String primary;

    /** Secondary text (e.g. distance and time). */
    public final String secondary;

    /** Icon glyph/emoji for the turn direction. */
    public final String icon;

    /** Optional road name extracted from the instruction. */
    public final String roadName;

    /** Optional road type (motorway, residential, ...). */
    public final String roadType;

    public RouteInstruction(String primary, String secondary, String icon,
                            String roadName, String roadType) {
        this.primary = primary;
        this.secondary = secondary;
        this.icon = icon;
        this.roadName = roadName;
        this.roadType = roadType;
    }

    /**
     * Parse a description line emitted by the JNI route generator.
     * Expected format: "Turn onto road name  [distance, time]"
     */
    public static RouteInstruction parse(String rawLine) {
        if (rawLine == null || rawLine.isEmpty()) {
            return new RouteInstruction("", "", "•", "", "");
        }

        String rest = rawLine;
        String secondary = "";
        int bracket = rawLine.indexOf(" [");
        if (bracket >= 0) {
            rest = rawLine.substring(0, bracket).trim();
            secondary = rawLine.substring(bracket + 2, rawLine.length() - 1).trim();
        }

        // Strip trailing ")" if bracket was not closed properly
        if (rest.endsWith("]")) {
            rest = rest.substring(0, rest.length() - 1).trim();
        }

        String icon = "•";
        String primary = rest;
        String roadType = "";
        String roadName = "";

        // Detect turn keywords and assign icons
        String lower = rest.toLowerCase();
        if (lower.startsWith("start")) {
            icon = "▶";
        } else if (lower.startsWith("destination")) {
            icon = "🏁";
        } else if (lower.startsWith("sharp left")) {
            icon = "↰";
            primary = stripPrefix(primary, "Sharp left");
        } else if (lower.startsWith("left")) {
            icon = "←";
            primary = stripPrefix(primary, "Left");
        } else if (lower.startsWith("slight left")) {
            icon = "↲";
            primary = stripPrefix(primary, "Slight left");
        } else if (lower.startsWith("sharp right")) {
            icon = "↱";
            primary = stripPrefix(primary, "Sharp right");
        } else if (lower.startsWith("right")) {
            icon = "→";
            primary = stripPrefix(primary, "Right");
        } else if (lower.startsWith("slight right")) {
            icon = "↳";
            primary = stripPrefix(primary, "Slight right");
        } else if (lower.startsWith("straight")) {
            icon = "↑";
            primary = stripPrefix(primary, "Straight");
        } else if (lower.startsWith("turn")) {
            icon = "↻";
            primary = stripPrefix(primary, "Turn");
        }

        // Try to extract road type/name like "onto highway_primary Name"
        if (primary.toLowerCase().startsWith("onto ")) {
            primary = primary.substring(5).trim();
        }
        int space = primary.indexOf(' ');
        if (space > 0) {
            String firstToken = primary.substring(0, space);
            if (firstToken.startsWith("highway_")) {
                roadType = firstToken;
                roadName = primary.substring(space + 1).trim();
                primary = roadName.isEmpty() ? roadType : roadName;
            }
        }

        if (primary.isEmpty()) {
            primary = rest;
        }

        return new RouteInstruction(primary, secondary, icon, roadName, roadType);
    }

    private static String stripPrefix(String s, String prefix) {
        if (s.length() >= prefix.length()
            && s.substring(0, prefix.length()).equalsIgnoreCase(prefix)) {
            String rest = s.substring(prefix.length()).trim();
            if (rest.startsWith(" ")) {
                rest = rest.substring(1);
            }
            return rest;
        }
        return s;
    }
}
