package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for the RouteInstruction model class, in particular the per-step
 * time the native side reports for a segment.
 */
class RouteInstructionTest {

    @Test
    void testFullConstructorCarriesThePerStepTime() {
        RouteInstruction instruction = new RouteInstruction(
            1500.0, 95.0, TurnType.LEFT, "Hauptstrasse", "Turn left into Hauptstrasse", "Turn left",
            0.0, TurnType.STRAIGHT_ON, "", "");

        assertEquals(1500.0, instruction.distanceTo);
        assertEquals(95.0, instruction.timeTo);
        assertEquals(TurnType.LEFT, instruction.turnType);
        assertEquals("Hauptstrasse", instruction.streetName);
        assertEquals("Turn left into Hauptstrasse", instruction.description);
        assertEquals("Turn left", instruction.shortDescription);
    }

    @Test
    void testFullConstructorCarriesTheNextNextHintAlongsideTheTime() {
        RouteInstruction instruction = new RouteInstruction(
            500.0, 30.0, TurnType.RIGHT, "Ringstrasse", "Turn right into Ringstrasse", "Turn right",
            400.0, TurnType.LEFT, "Turn left into Markt", "Turn left");

        assertEquals(30.0, instruction.timeTo);
        assertTrue(instruction.hasNextNext());
        assertEquals(400.0, instruction.nextNextDistanceTo);
        assertEquals(TurnType.LEFT, instruction.nextNextTurnType);
        assertEquals("Turn left into Markt", instruction.nextNextDescription);
    }

    @Test
    void testShortConstructorReportsNoTime() {
        RouteInstruction instruction = new RouteInstruction(
            250.0, TurnType.SLIGHTLY_RIGHT, "B1", "Keep right", "Keep right");

        assertEquals(250.0, instruction.distanceTo);
        assertEquals(0.0, instruction.timeTo);
        assertFalse(instruction.hasNextNext());
    }

    @Test
    void testTimeIsRenderedInToString() {
        RouteInstruction instruction = new RouteInstruction(
            1200.0, 60.0, TurnType.STRAIGHT_ON, "B1", "Continue", "Continue",
            0.0, TurnType.STRAIGHT_ON, "", "");

        String text = instruction.toString();

        assertTrue(text.contains("timeTo=60.0"), text);
        assertTrue(text.contains("distanceTo=1200.0"), text);
    }

    @Test
    void testNullTextFieldsFallBackToEmpty() {
        RouteInstruction instruction = new RouteInstruction(
            100.0, 10.0, TurnType.STRAIGHT_ON, null, null, null,
            0.0, null, null, null);

        assertEquals("", instruction.streetName);
        assertEquals("", instruction.description);
        assertEquals("", instruction.shortDescription);
        assertEquals(TurnType.STRAIGHT_ON, instruction.nextNextTurnType);
        assertEquals("", instruction.nextNextDescription);
    }
}
