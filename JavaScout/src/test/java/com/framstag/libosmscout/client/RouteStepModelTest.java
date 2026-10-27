package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import java.util.HashMap;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Unit tests for the route step data class and for the manoeuvre facts of the route instruction.
 * <p>
 * Both are plain value types, so they are constructed directly here: no native library and no map
 * database are needed, and every value of the JNI projection has a Java-side counterpart that a
 * test can name.
 */
class RouteStepModelTest {

    @Test
    void testStepCarriesEverySubRecord() {
        RouteStep step = new RouteStep();

        step.kind = RouteStep.Kind.MOTORWAY_LEAVE;
        step.databaseId = 7L;
        step.currentNodeIndex = 3L;
        step.pathObjectOffset = 42L;
        step.pathObjectType = 3;
        step.targetNodeIndex = 4L;
        step.lat = 51.5;
        step.lon = 7.4;
        step.distanceFromStart = 20_000.0;
        step.timeFromStart = 720.0;
        step.objectOffsets = new long[]{11L, 12L};
        step.objectTypes = new int[]{3, 1};
        step.legDistance = 1_500.0;
        step.legTime = 95.0;

        step.name = new RouteStep.Name("Hauptstrasse", "B1");
        step.nameChanged = new RouteStep.NameChanged();
        step.nameChanged.from = new RouteStep.Name("Alt", "L1");
        step.nameChanged.to = new RouteStep.Name("Neu", "L2");
        step.typeName = "highway_motorway_link";

        step.crossingWays = new RouteStep.CrossingWays();
        step.crossingWays.exitCount = 2;
        step.crossingWays.origin = new RouteStep.Name("Von", "");
        step.crossingWays.target = new RouteStep.Name("Nach", "");
        step.crossingWays.others = new RouteStep.Name[]{new RouteStep.Name("Kreuzung", "")};

        step.direction = new RouteStep.Direction();
        step.direction.turnAngle = 45.0;
        step.direction.curveAngle = 30.0;
        step.direction.turn = TurnType.SLIGHTLY_RIGHT;
        step.direction.curve = TurnType.RIGHT;

        step.turn = TurnType.LEFT;

        step.roundaboutEnter = new RouteStep.RoundaboutEnter();
        step.roundaboutEnter.clockwise = true;

        step.roundaboutLeave = new RouteStep.RoundaboutLeave();
        step.roundaboutLeave.exitCountTaken = 2;
        step.roundaboutLeave.clockwise = false;

        step.motorwayEnter = new RouteStep.MotorwayEnter();
        step.motorwayEnter.to = new RouteStep.Name("A2", "");

        step.motorwayChange = new RouteStep.MotorwayChange();
        step.motorwayChange.from = new RouteStep.Name("A2", "");
        step.motorwayChange.to = new RouteStep.Name("A45", "");
        step.motorwayChange.move = TurnType.RIGHT;

        step.motorwayLeave = new RouteStep.MotorwayLeave();
        step.motorwayLeave.from = new RouteStep.Name("A2", "");
        step.motorwayLeave.move = TurnType.RIGHT;

        step.junction = new RouteStep.Name("Kreuz Kamen", "12");
        step.destination = "Dortmund;Kamen";
        step.maxSpeed = 80;

        step.lanes = new RouteStep.Lane();
        step.lanes.oneway = true;
        step.lanes.count = 2;
        step.lanes.turns = new LaneTurn[]{LaneTurn.LEFT, LaneTurn.STRAIGHT_ON};

        step.suggestedLanes = new RouteStep.SuggestedLane();
        step.suggestedLanes.from = 0;
        step.suggestedLanes.to = 1;
        step.suggestedLanes.turn = LaneTurn.SLIGHTLY_LEFT;

        step.poi = new RouteStep.Poi();
        step.poi.databaseId = 7L;
        step.poi.objectOffset = 99L;
        step.poi.objectType = 1;
        step.poi.name = new RouteStep.Name("Tankstelle", "");
        step.poi.distance = 12_300.0;

        step.via = new RouteStep.Via();
        step.via.sectionNumber = 2;
        step.via.nodeCount = 14;

        step.start = "Home";
        step.target = "Work";

        assertEquals(RouteStep.Kind.MOTORWAY_LEAVE, step.kind);
        assertEquals(7L, step.databaseId);
        assertEquals(3L, step.currentNodeIndex);
        assertEquals(42L, step.pathObjectOffset);
        assertEquals(3, step.pathObjectType);
        assertEquals(4L, step.targetNodeIndex);
        assertEquals(51.5, step.lat);
        assertEquals(7.4, step.lon);
        assertEquals(20_000.0, step.distanceFromStart);
        assertEquals(720.0, step.timeFromStart);
        assertEquals(2, step.objectOffsets.length);
        assertEquals(3, step.objectTypes[0]);
        assertEquals(1_500.0, step.legDistance);
        assertEquals(95.0, step.legTime);
        assertEquals("Hauptstrasse", step.name.name);
        assertEquals("B1", step.name.ref);
        assertEquals("Alt", step.nameChanged.from.name);
        assertEquals("L2", step.nameChanged.to.ref);
        assertEquals("highway_motorway_link", step.typeName);
        assertEquals(2, step.crossingWays.exitCount);
        assertEquals("Von", step.crossingWays.origin.name);
        assertEquals("Nach", step.crossingWays.target.name);
        assertEquals("Kreuzung", step.crossingWays.others[0].name);
        assertEquals(45.0, step.direction.turnAngle);
        assertEquals(30.0, step.direction.curveAngle);
        assertEquals(TurnType.SLIGHTLY_RIGHT, step.direction.turn);
        assertEquals(TurnType.RIGHT, step.direction.curve);
        assertEquals(TurnType.LEFT, step.turn);
        assertTrue(step.roundaboutEnter.clockwise);
        assertEquals(2, step.roundaboutLeave.exitCountTaken);
        assertEquals("A2", step.motorwayEnter.to.name);
        assertEquals("A45", step.motorwayChange.to.name);
        assertEquals(TurnType.RIGHT, step.motorwayChange.move);
        assertEquals("A2", step.motorwayLeave.from.name);
        assertEquals("12", step.junction.ref);
        assertEquals("Kreuz Kamen", step.junction.name);
        assertEquals("Dortmund;Kamen", step.destination);
        assertEquals(80, step.maxSpeed);
        assertTrue(step.lanes.oneway);
        assertEquals(2, step.lanes.count);
        assertEquals(LaneTurn.LEFT, step.lanes.turns[0]);
        assertEquals(1, step.suggestedLanes.to);
        assertEquals(LaneTurn.SLIGHTLY_LEFT, step.suggestedLanes.turn);
        assertEquals("Tankstelle", step.poi.name.name);
        assertEquals(12_300.0, step.poi.distance);
        assertEquals(2, step.via.sectionNumber);
        assertEquals(14, step.via.nodeCount);
        assertEquals("Home", step.start);
        assertEquals("Work", step.target);
    }

    @Test
    void testAbsentSubRecordsAreNull() {
        RouteStep step = new RouteStep();

        assertNull(step.name);
        assertNull(step.nameChanged);
        assertNull(step.typeName);
        assertNull(step.crossingWays);
        assertNull(step.direction);
        assertNull(step.turn);
        assertNull(step.roundaboutEnter);
        assertNull(step.roundaboutLeave);
        assertNull(step.motorwayEnter);
        assertNull(step.motorwayChange);
        assertNull(step.motorwayLeave);
        assertNull(step.junction);
        assertNull(step.destination);
        assertNull(step.maxSpeed);
        assertNull(step.lanes);
        assertNull(step.suggestedLanes);
        assertNull(step.poi);
        assertNull(step.via);
        assertNull(step.start);
        assertNull(step.target);
        assertNull(step.remainingDistance);
        assertNull(step.remainingTime);
        assertNull(step.nextNext);
    }

    @Test
    void testRouteEntryPublishesStepsAndMapping() {
        RouteEntry entry = new RouteEntry();

        entry.steps = new RouteStep[]{new RouteStep()};
        entry.databaseMapping = new HashMap<>();
        entry.databaseMapping.put(7L, "/maps/dortmund");

        Map<Long, String> mapping = entry.databaseMapping;

        assertNotNull(entry.steps);
        assertEquals(1, entry.steps.length);
        assertEquals("/maps/dortmund", mapping.get(7L));

        // A fresh result carries an empty mapping, never null, so a client can resolve without a
        // null check.
        RouteEntry fresh = new RouteEntry();

        assertNotNull(fresh.databaseMapping);
        assertTrue(fresh.databaseMapping.isEmpty());
    }

    @Test
    void testExistingInstructionConstructorsDefaultTheFactsToAbsent() {
        RouteInstruction five = new RouteInstruction(
            100.0, TurnType.LEFT, "Hauptstrasse", "Turn left into Hauptstrasse", "Turn left");

        RouteInstruction twelve = new RouteInstruction(
            100.0, 20.0, 100.0, TurnType.LEFT, "Hauptstrasse",
            "Turn left into Hauptstrasse", "Turn left",
            0.0, TurnType.STRAIGHT_ON, "", "");

        RouteInstruction thirteen = new RouteInstruction(
            100.0, 20.0, 100.0, TurnType.LEFT, "Hauptstrasse",
            "Turn left into Hauptstrasse", "Turn left",
            0.0, TurnType.STRAIGHT_ON, "", "", 0, true);

        RouteInstruction fourteen = new RouteInstruction(
            100.0, 20.0, 100.0, TurnType.LEFT, "Hauptstrasse",
            "Turn left into Hauptstrasse", "Turn left",
            0.0, TurnType.STRAIGHT_ON, "", "", 0, true, 0);

        for (RouteInstruction instruction : new RouteInstruction[]{five, twelve, thirteen, fourteen}) {
            assertNull(instruction.junctionRef);
            assertNull(instruction.junctionName);
            assertNull(instruction.destinations);
            assertNull(instruction.wayType);
            assertNull(instruction.maxSpeed);
            assertEquals("Turn left into Hauptstrasse", instruction.description);
            assertEquals("Turn left", instruction.shortDescription);
        }
    }

    @Test
    void testInstructionCarriesTheFactsOfItsNode() {
        RouteInstruction instruction = new RouteInstruction(
            200.0, 12.0, 200.0, TurnType.RIGHT, "B233",
            "Turn right into B233", "Turn right",
            0.0, TurnType.STRAIGHT_ON, "", "", 0, true, 0,
            "12", "Kreuz Kamen", "Dortmund;Kamen", "highway_motorway_link", 80);

        assertEquals("12", instruction.junctionRef);
        assertEquals("Kreuz Kamen", instruction.junctionName);
        assertEquals("Dortmund;Kamen", instruction.destinations);
        assertEquals("highway_motorway_link", instruction.wayType);
        assertEquals(80, instruction.maxSpeed);

        // The text fields are unchanged by the added facts.
        assertEquals("Turn right into B233", instruction.description);
        assertEquals("Turn right", instruction.shortDescription);
    }
}
