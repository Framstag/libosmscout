package com.framstag.libosmscout;

import com.framstag.libosmscout.client.RouteEntry;
import com.framstag.libosmscout.client.RouteStep;
import com.framstag.libosmscout.client.TurnType;

import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Builds the rows of the route description list and the detail line of the next-turn display.
 * <p>
 * Every row of the list comes from a published route step: the manoeuvres of the route and the
 * POIs on it, so the list shows every manoeuvre the route description holds and takes each row's
 * distance and time from the step's leg values. A node that carries only segment facts (a maximum
 * speed, the lanes, a via section) is published as a step but is not a row: the list keeps the
 * manoeuvres it has always shown instead of becoming a list of speed limits. No
 * routing fact — turn type, road name, road type, exit number, destination, distance or time — is
 * recovered from the characters of a description line, and a header is never recognised by
 * comparing its text to a literal.
 * <p>
 * A result carries its steps or it carries nothing: the library stopped publishing description
 * lines, so there is no prose fallback to fall back to (change `retire-instruction-prose`).
 */
public final class RouteRowBuilder {

    /** One row of the route description list. */
    public static final class Row {
        /** The manoeuvre, as text the app composes from the step's values. */
        public final String primary;
        /** The leg's distance and time, empty when the step carries none. */
        public final String secondary;

        /**
         * Construct a row.
         *
         * @param primary   the manoeuvre text
         * @param secondary the leg text
         */
        public Row(String primary, String secondary) {
            this.primary = primary != null ? primary : "";
            this.secondary = secondary != null ? secondary : "";
        }
    }

    private RouteRowBuilder() {
    }

    /**
     * Build the rows of a route result.
     *
     * @param route the route result
     * @return one row per published step, in route order; empty when it carries none
     */
    public static List<Row> buildRows(RouteEntry route) {
        List<Row> rows = new ArrayList<>();

        if (route == null) {
            return rows;
        }

        if (route.steps != null) {
            for (RouteStep step : route.steps) {
                if (step != null && isRendered(step)) {
                    rows.add(rowOf(step));
                }
            }
        }

        return rows;
    }

    /**
     * Whether a step produces a row of the list.
     * <p>
     * The list keeps the manoeuvres it has always shown (start, arrival, turns, roundabouts,
     * motorway manoeuvres, name changes) and adds the POIs on the route. A node that carries only
     * segment facts - a maximum speed, the lanes, a via section - is not a row: the model publishes
     * it so a client can read those facts, but the route list does not become a list of speed
     * limits.
     */
    private static boolean isRendered(RouteStep step) {
        if (step.poi != null) {
            return true;
        }

        switch (step.kind) {
            case START:
            case TARGET:
            case TURN:
            case ROUNDABOUT_ENTER:
            case ROUNDABOUT_LEAVE:
            case MOTORWAY_ENTER:
            case MOTORWAY_CHANGE:
            case MOTORWAY_LEAVE:
            case NAME_CHANGE:
                return true;
            case INFORMATIVE:
            case UNKNOWN:
            default:
                return false;
        }
    }

    /**
     * The row of one step: the manoeuvre and its leg's distance and time.
     *
     * @param step the step
     * @return its row
     */
    static Row rowOf(RouteStep step) {
        StringBuilder primary = new StringBuilder(manoeuvreText(step));

        // The exit reference, the junction name and the destination signs belong to the row of the
        // manoeuvre that carries them (spec: route-description - every manoeuvre of a motorway
        // route appears).
        if (step.junction != null) {
            if (!step.junction.ref.isEmpty()) {
                primary.append(" exit ").append(step.junction.ref);
            }

            if (!step.junction.name.isEmpty()) {
                primary.append(" ").append(step.junction.name);
            }
        }

        if (step.destination != null && !step.destination.isEmpty()) {
            primary.append(" \u2192 ").append(step.destination);
        }

        return new Row(primary.toString(), legText(step));
    }

    /**
     * The manoeuvre of a step as text, composed from the step's own values.
     */
    private static String manoeuvreText(RouteStep step) {
        switch (step.kind) {
            case START:
                return "Start";
            case TARGET:
                return "Arrive";
            case ROUNDABOUT_ENTER:
                return "Enter roundabout";
            case ROUNDABOUT_LEAVE:
                if (step.roundaboutLeave != null && step.roundaboutLeave.exitCountTaken > 0) {
                    return "Take exit " + step.roundaboutLeave.exitCountTaken;
                }
                return "Leave roundabout";
            case MOTORWAY_ENTER:
                return "Enter " + nameText(step.motorwayEnter != null ? step.motorwayEnter.to : null);
            case MOTORWAY_LEAVE:
                return "Leave " + nameText(step.motorwayLeave != null ? step.motorwayLeave.from : null);
            case MOTORWAY_CHANGE:
                return "Change motorway";
            case NAME_CHANGE:
                return "Continue as " + nameText(step.nameChanged != null ? step.nameChanged.to : null);
            case TURN:
                return turnText(step);
            case INFORMATIVE:
            case UNKNOWN:
            default:
                if (step.poi != null) {
                    String poiName = nameText(step.poi.name);

                    return poiName.isEmpty() ? "Point of interest" : "Point of interest " + poiName;
                }
                if (step.typeName != null && !step.typeName.isEmpty()) {
                    return "Continue on " + step.typeName;
                }
                if (step.name != null) {
                    return "Continue on " + nameText(step.name);
                }
                return "Continue";
        }
    }

    private static String turnText(RouteStep step) {
        TurnType turn = step.turn;

        if (turn == null && step.direction != null) {
            turn = step.direction.curve;
        }

        String action = turnWord(turn);
        String street = nameText(step.name);

        if (street.isEmpty()) {
            return action;
        }

        return action + " into " + street;
    }

    private static String turnWord(TurnType turn) {
        if (turn == null) {
            return "Continue";
        }

        switch (turn) {
            case SHARP_LEFT:
                return "Turn sharp left";
            case LEFT:
                return "Turn left";
            case SLIGHTLY_LEFT:
                return "Turn slightly left";
            case STRAIGHT_ON:
                return "Continue straight";
            case SLIGHTLY_RIGHT:
                return "Turn slightly right";
            case RIGHT:
                return "Turn right";
            case SHARP_RIGHT:
                return "Turn sharp right";
            default:
                return "Continue";
        }
    }

    private static String nameText(RouteStep.Name name) {
        if (name == null) {
            return "";
        }

        if (!name.name.isEmpty()) {
            return name.name;
        }

        return name.ref;
    }

    private static String legText(RouteStep step) {
        StringBuilder text = new StringBuilder();

        if (step.legDistance > 0.0) {
            text.append(distanceText(step.legDistance));
        }

        if (step.legTime > 0.0) {
            if (text.length() > 0) {
                text.append(" \u00b7 ");
            }

            text.append(timeText(step.legTime));
        }

        return text.toString();
    }

    private static String distanceText(double meters) {
        if (meters < 1000.0) {
            return String.format(Locale.ROOT, "%.0f m", meters);
        }

        return String.format(Locale.ROOT, "%.2f km", meters / 1000.0);
    }

    private static String timeText(double seconds) {
        long totalSeconds = Math.round(seconds);

        if (totalSeconds < 60) {
            return totalSeconds + " s";
        }

        long minutes = totalSeconds / 60;

        if (minutes < 60) {
            return minutes + " min";
        }

        return (minutes / 60) + " h " + (minutes % 60) + " min";
    }

    /**
     * The detail line of the next-turn display: the exit reference, the junction name and the
     * destination signs, when the step carries them.
     *
     * @param step the step ahead
     * @return the detail text, empty when the step carries none of those facts
     */
    public static String nextTurnDetail(RouteStep step) {
        if (step == null) {
            return "";
        }

        StringBuilder detail = new StringBuilder();

        if (step.junction != null && step.junction.ref != null && !step.junction.ref.isEmpty()) {
            detail.append("Exit ").append(step.junction.ref);
        }

        if (step.junction != null && step.junction.name != null && !step.junction.name.isEmpty()) {
            if (detail.length() > 0) {
                detail.append(" ");
            }

            detail.append(step.junction.name);
        }

        if (step.destination != null && !step.destination.isEmpty()) {
            if (detail.length() > 0) {
                detail.append(" \u2192 ");
            }

            detail.append(step.destination);
        }

        return detail.toString();
    }
}
