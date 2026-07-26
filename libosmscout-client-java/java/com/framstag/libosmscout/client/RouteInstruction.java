package com.framstag.libosmscout.client;

/**
 * A turn-by-turn instruction produced by the navigation engine.
 * <p>
 * Bridged from C++ {@code JavaRouteInstruction} via JNI. Instances are
 * created by the native {@code DispatchMessage} handler and delivered
 * through {@link NavigationListener#onNextRouteInstruction(RouteInstruction)}
 * (live) or {@link NavigationListener#onRouteInstructions(RouteInstruction[])}
 * (full list on route change).
 */
public class RouteInstruction {

    /** Distance to the next manoeuvre in meters. */
    public final double distanceTo;

    /** Type of turn at the next manoeuvre. */
    public final TurnType turnType;

    /** Street name to turn into, or empty if unknown. */
    public final String streetName;

    /** Human-readable description, e.g. "Turn left into Hauptstrasse". */
    public final String description;

    /** Short description, e.g. "Turn left". */
    public final String shortDescription;

    // -- Optional "next next" hint fields --

    /** Distance to the following manoeuvre (0 if none or too far). */
    public final double nextNextDistanceTo;

    /** Turn type of the following manoeuvre. */
    public final TurnType nextNextTurnType;

    /** Description of the following manoeuvre. */
    public final String nextNextDescription;

    /** Short description of the following manoeuvre. */
    public final String nextNextShortDescription;

    /**
     * Construct a route instruction with the required fields.
     *
     * @param distanceTo       distance to next manoeuvre in meters
     * @param turnType         type of turn
     * @param streetName       street to turn into (may be empty)
     * @param description      human-readable description
     * @param shortDescription short description
     */
    public RouteInstruction(double distanceTo,
                            TurnType turnType,
                            String streetName,
                            String description,
                            String shortDescription) {
        this(distanceTo, turnType, streetName, description, shortDescription,
             0.0, TurnType.STRAIGHT_ON, "", "");
    }

    /**
     * Full constructor including optional "next next" hint.
     *
     * @param distanceTo              distance to next manoeuvre in meters
     * @param turnType                type of turn
     * @param streetName              street to turn into (may be empty)
     * @param description             human-readable description
     * @param shortDescription        short description
     * @param nextNextDistanceTo      distance to following manoeuvre (0 if none)
     * @param nextNextTurnType        turn type of following manoeuvre
     * @param nextNextDescription     description of following manoeuvre
     * @param nextNextShortDescription short description of following manoeuvre
     */
    public RouteInstruction(double distanceTo,
                            TurnType turnType,
                            String streetName,
                            String description,
                            String shortDescription,
                            double nextNextDistanceTo,
                            TurnType nextNextTurnType,
                            String nextNextDescription,
                            String nextNextShortDescription) {
        this.distanceTo = distanceTo;
        this.turnType = turnType;
        this.streetName = streetName != null ? streetName : "";
        this.description = description != null ? description : "";
        this.shortDescription = shortDescription != null ? shortDescription : "";
        this.nextNextDistanceTo = nextNextDistanceTo;
        this.nextNextTurnType = nextNextTurnType != null ? nextNextTurnType : TurnType.STRAIGHT_ON;
        this.nextNextDescription = nextNextDescription != null ? nextNextDescription : "";
        this.nextNextShortDescription = nextNextShortDescription != null ? nextNextShortDescription : "";
    }

    /**
     * Whether a "next next" hint is present.
     *
     * @return true if a following manoeuvre hint is available
     */
    public boolean hasNextNext() {
        return nextNextDistanceTo > 0 && !nextNextDescription.isEmpty();
    }

    @Override
    public String toString() {
        return "RouteInstruction{"
            + "distanceTo=" + distanceTo
            + ", turnType=" + turnType
            + ", streetName='" + streetName + '\''
            + ", description='" + description + '\''
            + ", shortDescription='" + shortDescription + '\''
            + (hasNextNext()
               ? ", nextNext=" + nextNextDistanceTo + "m " + nextNextDescription
               : "")
            + '}';
    }
}
