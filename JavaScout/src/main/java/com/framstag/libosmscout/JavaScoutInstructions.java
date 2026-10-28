package com.framstag.libosmscout;

import com.framstag.libosmscout.client.RouteInstruction;
import com.framstag.libosmscout.client.RouteStep;
import com.framstag.libosmscout.client.TurnType;

/**
 * Turns the steps the client library publishes into the {@link RouteInstruction} rows JavaScout's
 * own UI still renders.
 *
 * The library stopped publishing instruction callbacks (its navigation engine now emits
 * {@code RouteStep}, the structured record a calculated route publishes), so a consumer that wants
 * instruction-shaped rows builds them itself. This adapter is deliberately on the <em>consumer</em>
 * side: the library holds no prose and no turn-type mapping, and the wording below is JavaScout's
 * own — a desktop scout's English UI, not a contract of the client library.
 */
final class JavaScoutInstructions {

    private JavaScoutInstructions() {
    }

    /** The instruction rows for a published step list, in route order. */
    static RouteInstruction[] of(RouteStep[] steps) {
        if (steps == null) {
            return new RouteInstruction[0];
        }

        RouteInstruction[] result = new RouteInstruction[steps.length];

        for (int i = 0; i < steps.length; i++) {
            result[i] = of(steps[i]);
        }

        return result;
    }

    /** One step as the instruction row JavaScout's overlay and list render. */
    static RouteInstruction of(RouteStep step) {
        if (step == null) {
            return null;
        }

        double remaining = step.remainingDistance != null ? step.remainingDistance : 0.0;
        String street = step.name != null ? step.name.name : "";
        String wording = wording(step);

        return new RouteInstruction(
            remaining,
            turnType(step),
            street,
            street.isEmpty() ? wording : wording + " into " + street,
            wording);
    }

    /** The instruction row's turn type, for the overlay's arrow. */
    static TurnType turnType(RouteStep step) {
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

    /** The move a turn or a motorway leave carries, straight on when it carries none. */
    private static TurnType move(RouteStep step) {
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

    /** JavaScout's own wording for a manoeuvre. */
    private static String wording(RouteStep step) {
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
                return step.name != null && !step.name.name.isEmpty() ? step.name.name : "";
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
