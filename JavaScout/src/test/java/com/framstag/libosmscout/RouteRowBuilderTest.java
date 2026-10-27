package com.framstag.libosmscout;

import com.framstag.libosmscout.client.RouteEntry;
import com.framstag.libosmscout.client.RouteInstruction;
import com.framstag.libosmscout.client.RouteStep;
import com.framstag.libosmscout.client.TurnType;

import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Unit tests for the row building of the route description list and for the detail line of the
 * next-turn display. Pure value types, so no native library and no map database are needed.
 */
class RouteRowBuilderTest {

    private static RouteStep step(RouteStep.Kind kind, double distanceFromStart,
                                  double legDistance, double legTime) {
        RouteStep step = new RouteStep();

        step.kind = kind;
        step.distanceFromStart = distanceFromStart;
        step.legDistance = legDistance;
        step.legTime = legTime;

        return step;
    }

    private static RouteEntry routeOf(RouteStep... steps) {
        RouteEntry route = new RouteEntry();

        route.steps = steps;

        return route;
    }

    @Test
    void testOneRowPerStepInRouteOrder() {
        RouteStep start = step(RouteStep.Kind.START, 0.0, 0.0, 0.0);

        RouteStep turn = step(RouteStep.Kind.TURN, 3400.0, 3400.0, 120.0);
        turn.turn = TurnType.LEFT;
        turn.name = new RouteStep.Name("Hauptstrasse", "B1");

        RouteStep target = step(RouteStep.Kind.TARGET, 5000.0, 1600.0, 60.0);

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(routeOf(start, turn, target));

        assertEquals(3, rows.size());

        assertEquals("Start", rows.get(0).primary);
        assertEquals("", rows.get(0).secondary);

        assertTrue(rows.get(1).primary.contains("Turn left"),
            "row of the turn: " + rows.get(1).primary);
        assertTrue(rows.get(1).primary.contains("Hauptstrasse"),
            "row of the turn names the street: " + rows.get(1).primary);
        assertTrue(rows.get(1).secondary.contains("3.40 km"),
            "row's distance is the step's leg: " + rows.get(1).secondary);
        assertTrue(rows.get(1).secondary.contains("2 min"),
            "row's time is the step's leg: " + rows.get(1).secondary);

        assertEquals("Arrive", rows.get(2).primary);
    }

    @Test
    void testStepsWithOnlySegmentFactsAreNotRows() {
        RouteStep speed = step(RouteStep.Kind.INFORMATIVE, 100.0, 100.0, 10.0);
        speed.maxSpeed = 100;

        RouteStep turn = step(RouteStep.Kind.TURN, 200.0, 100.0, 10.0);
        turn.turn = TurnType.RIGHT;

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(routeOf(speed, turn));

        assertEquals(1, rows.size(), "only the manoeuvre is a row");
        assertTrue(rows.get(0).primary.contains("Turn right"), rows.get(0).primary);
    }

    @Test
    void testPointOfInterestOnTheRouteIsARow() {
        RouteStep poi = step(RouteStep.Kind.INFORMATIVE, 1200.0, 200.0, 20.0);
        poi.poi = new RouteStep.Poi();
        poi.poi.databaseId = 0;
        poi.poi.objectOffset = 4711;
        poi.poi.objectType = 1;
        poi.poi.name = new RouteStep.Name("Tankstelle", "");
        poi.poi.distance = 1200.0;

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(routeOf(poi));

        assertEquals(1, rows.size());
        assertTrue(rows.get(0).primary.contains("Tankstelle"), rows.get(0).primary);
    }

    @Test
    void testMotorwayRowShowsExitReferenceAndDestination() {
        RouteStep leave = step(RouteStep.Kind.MOTORWAY_LEAVE, 20000.0, 1500.0, 95.0);

        leave.motorwayLeave = new RouteStep.MotorwayLeave();
        leave.motorwayLeave.from = new RouteStep.Name("A2", "");
        leave.motorwayLeave.move = TurnType.RIGHT;
        leave.junction = new RouteStep.Name("Kreuz Kamen", "12");
        leave.destination = "Dortmund;Kamen";

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(routeOf(leave));

        assertEquals(1, rows.size());

        String primary = rows.get(0).primary;

        assertTrue(primary.contains("12"), "row shows the exit reference: " + primary);
        assertTrue(primary.contains("Kreuz Kamen"), "row shows the junction name: " + primary);
        assertTrue(primary.contains("Dortmund;Kamen"), "row shows the destination: " + primary);
    }

    @Test
    void testRoundaboutExitRowShowsTheOrdinalTaken() {
        RouteStep leave = step(RouteStep.Kind.ROUNDABOUT_LEAVE, 800.0, 300.0, 40.0);

        leave.roundaboutLeave = new RouteStep.RoundaboutLeave();
        leave.roundaboutLeave.exitCountTaken = 2;

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(routeOf(leave));

        assertEquals(1, rows.size());
        assertTrue(rows.get(0).primary.contains("2"),
            "row shows the exit taken: " + rows.get(0).primary);
    }

    @Test
    void testRouteWithoutStepsFallsBackToDescriptionLines() {
        RouteEntry route = new RouteEntry();

        route.steps = null;
        route.descriptions = new String[]{
            "--- Route ---",
            "Start: Home  [0 m]",
            "Left onto Ringstrasse  [3.4 km, 2 min]"
        };
        // The per-instruction positions are index-aligned with the instruction lines only, so the
        // description list is one entry longer than them: the header is dropped structurally.
        route.instructionLats = new double[]{52.0, 52.02};

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(route);

        assertEquals(2, rows.size());
        assertEquals("Start: Home  [0 m]", rows.get(0).primary);
        assertEquals("Left onto Ringstrasse  [3.4 km, 2 min]", rows.get(1).primary);
    }

    @Test
    void testHeaderIsNotRecognisedByItsText() {
        RouteEntry route = new RouteEntry();

        route.steps = null;
        // A description line whose text is a manoeuvre, in a list that carries a header: the entry
        // the alignment excludes is dropped because of its position, never because of its text.
        route.descriptions = new String[]{"Turn left into X", "Right onto Y"};
        route.instructionLats = new double[]{52.0};

        List<RouteRowBuilder.Row> rows = RouteRowBuilder.buildRows(route);

        assertEquals(1, rows.size());
        assertEquals("Right onto Y", rows.get(0).primary);

        // Without alignment information nothing is dropped: a line that happens to read like a
        // header is a row, because its text is not inspected.
        RouteEntry unaligned = new RouteEntry();

        unaligned.steps = null;
        unaligned.descriptions = new String[]{"--- Route ---"};
        unaligned.instructionLats = null;

        List<RouteRowBuilder.Row> unalignedRows = RouteRowBuilder.buildRows(unaligned);

        assertEquals(1, unalignedRows.size());
        assertEquals("--- Route ---", unalignedRows.get(0).primary);
    }

    @Test
    void testEmptyResultProducesNoRow() {
        assertTrue(RouteRowBuilder.buildRows(null).isEmpty());
        assertTrue(RouteRowBuilder.buildRows(new RouteEntry()).isEmpty());
    }

    @Test
    void testNextTurnDetailCarriesTheFactsAsData() {
        RouteInstruction withFacts = new RouteInstruction(
            200.0, 12.0, 200.0, TurnType.RIGHT, "B233",
            "Turn right into B233", "Turn right",
            0.0, TurnType.STRAIGHT_ON, "", "", 0, true, 0,
            "12", "Kreuz Kamen", "Dortmund;Kamen", "highway_motorway_link", 80);

        String detail = RouteRowBuilder.nextTurnDetail(withFacts);

        assertTrue(detail.contains("12"), "detail shows the exit reference: " + detail);
        assertTrue(detail.contains("Kreuz Kamen"), "detail shows the junction name: " + detail);
        assertTrue(detail.contains("Dortmund;Kamen"), "detail shows the destination: " + detail);

        // A turn instruction without those facts contributes nothing, so the display renders as
        // before.
        RouteInstruction plain = new RouteInstruction(
            200.0, TurnType.LEFT, "Hauptstrasse", "Turn left", "Turn left");

        assertTrue(RouteRowBuilder.nextTurnDetail(plain).isEmpty());
        assertFalse(RouteRowBuilder.nextTurnDetail(plain).contains("Exit"));
    }
}
