package com.framstag.libosmscout;

import com.framstag.libosmscout.client.RouteStep;
import com.framstag.libosmscout.client.TurnType;

/**
 * JavaScout's own wording and glyph choice for a published route step.
 *
 * The client library publishes values only — a step's kind, its turn type, the road name, the exit
 * ordinal — and never text, so every consumer words the manoeuvre itself (spec:
 * `turn-instruction-localization`). This class is that wording for this desktop scout's English UI,
 * deliberately on the consumer side: it is not a contract of the client library, and the library
 * holds no prose for it to fall back to.
 */
final class NextTurnText {

    private NextTurnText() {
    }

    /** The turn type a step's arrow is drawn from. */
    static TurnType turnType(RouteStep step) {
        if (step == null) {
            return TurnType.STRAIGHT_ON;
        }

        switch (step.kind) {
            case START:
                return TurnType.START;
            case TARGET:
                return TurnType.TARGET_REACHED;
            case ROUNDABOUT_ENTER:
                return TurnType.ROUNDABOUT_ENTER;
            case ROUNDABOUT_LEAVE:
                return TurnType.ROUNDABOUT_LEAVE;
            case MOTORWAY_ENTER:
            case MOTORWAY_CHANGE:
                return TurnType.MOTORWAY_ENTER;
            default:
                return move(step);
        }
    }

    /** The move a turn or a motorway manoeuvre carries, straight on when it carries none. */
    static TurnType move(RouteStep step) {
        if (step.turn != null) {
            return step.turn;
        }
        if (step.motorwayLeave != null) {
            return step.motorwayLeave.move;
        }
        if (step.motorwayChange != null) {
            return step.motorwayChange.move;
        }
        return TurnType.STRAIGHT_ON;
    }

    /** The road the step leads onto, empty when it names none. */
    static String street(RouteStep step) {
        if (step == null || step.name == null || step.name.name == null) {
            return "";
        }

        return step.name.name;
    }

    /** The step's remaining distance, zero when it carries none (a route list entry has no progress). */
    static double distanceTo(RouteStep step) {
        if (step == null || step.remainingDistance == null) {
            return 0.0;
        }

        return step.remainingDistance;
    }

    /** The manoeuvre as one line: the wording, and the road it leads onto when it has one. */
    static String line(RouteStep step) {
        String wording = wording(step);
        String street = street(step);

        if (wording.isEmpty()) {
            return street;
        }
        if (street.isEmpty() || wording.contains(street)) {
            return wording;
        }

        return wording + " into " + street;
    }

    /** The manoeuvre alone: "Turn left", "Exit 2", "Arrive". */
    static String wording(RouteStep step) {
        if (step == null) {
            return "";
        }

        switch (step.kind) {
            case START:
                return "Start";
            case TARGET:
                return "Arrive";
            case ROUNDABOUT_ENTER:
                return "Roundabout";
            case ROUNDABOUT_LEAVE:
                return step.roundaboutLeave != null && step.roundaboutLeave.exitCountTaken > 0
                    ? "Exit " + step.roundaboutLeave.exitCountTaken
                    : "Leave roundabout";
            case MOTORWAY_ENTER:
                return "Enter motorway";
            case MOTORWAY_CHANGE:
                return "Keep " + directionWord(move(step));
            case MOTORWAY_LEAVE:
                return "Leave motorway";
            case NAME_CHANGE:
                return "Continue";
            case INFORMATIVE:
            case UNKNOWN:
                return street(step);
            default:
                return directionWord(move(step));
        }
    }

    private static String directionWord(TurnType move) {
        switch (move) {
            case SHARP_LEFT:
                return "sharp left";
            case LEFT:
                return "left";
            case SLIGHTLY_LEFT:
                return "slightly left";
            case SLIGHTLY_RIGHT:
                return "slightly right";
            case RIGHT:
                return "right";
            case SHARP_RIGHT:
                return "sharp right";
            default:
                return "straight on";
        }
    }
}
