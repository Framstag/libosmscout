package com.framstag.libosmscout.client;

/**
 * Lane turn direction indicators, matching C++ {@code osmscout::LaneTurn}.
 * <p>
 * Values are emitted by the navigation engine's {@code LaneAgent} and
 * delivered via {@link NavigationListener#onLaneUpdate}.
 * <p>
 * Must be kept in sync with {@code osmscout::LaneTurn} in
 * {@code libosmscout/include/osmscout/util/LaneTurn.h}.
 */
public enum LaneTurn {
    /** No lane data. */
    NULL(0),
    /** No turn required. */
    NONE(1),
    /** Turn left. */
    LEFT(2),
    /** Merge to left lane. */
    MERGE_TO_LEFT(3),
    /** Slight left turn. */
    SLIGHTLY_LEFT(4),
    /** Sharp left turn. */
    SHARP_LEFT(5),
    /** Left turn or straight on. */
    LEFT_AND_STRAIGHT(6),
    /** Straight on or slight left. */
    STRAIGHT_AND_SLIGHTLY_LEFT(7),
    /** Straight on or sharp left. */
    STRAIGHT_AND_SHARP_LEFT(8),
    /** Continue straight. */
    STRAIGHT_ON(9),
    /** Straight on or right. */
    STRAIGHT_AND_RIGHT(10),
    /** Straight on or slight right. */
    STRAIGHT_AND_SLIGHTLY_RIGHT(11),
    /** Straight on or sharp right. */
    STRAIGHT_AND_SHARP_RIGHT(12),
    /** Turn right. */
    RIGHT(13),
    /** Merge to right lane. */
    MERGE_TO_RIGHT(14),
    /** Slight right turn. */
    SLIGHTLY_RIGHT(15),
    /** Sharp right turn. */
    SHARP_RIGHT(16),
    /** Unknown turn direction. */
    UNKNOWN(17),
    /** Not present in C++ yet; reserved for future use. */
    LEFT_AND_RIGHT(18);

    /** Numeric id matching C++ osmscout::LaneTurn. */
    private final int id;

    /**
     * Constructor.
     *
     * @param id numeric id matching C++ enum value
     */
    LaneTurn(int id) {
        this.id = id;
    }

    /**
     * Returns the numeric id matching the C++ enum value.
     *
     * @return numeric id
     */
    public int getId() {
        return id;
    }

    /**
     * Look up a LaneTurn by its numeric id.
     *
     * @param id the numeric value from the C++ enum
     * @return the matching LaneTurn, or {@code UNKNOWN} if not found
     */
    public static LaneTurn fromId(int id) {
        for (LaneTurn t : values()) {
            if (t.id == id) {
                return t;
            }
        }
        return UNKNOWN;
    }
}
