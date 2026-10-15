package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for {@link RoutingProfile}, in particular the vehicle's usual maximum
 * speed the constructors name or default.
 */
class RoutingProfileTest {

    @Test
    void testDefaultConstructorRequestsTheBuiltInSpeed() {
        RoutingProfile profile = new RoutingProfile();

        assertEquals(Vehicle.CAR, profile.vehicle);
        assertEquals(0.0, profile.maxSpeedKmh, 0.00001);
        assertFalse(profile.avoidTolls);
        assertFalse(profile.avoidFerries);
        assertFalse(profile.avoidUnpaved);
    }

    @Test
    void testVehicleOnlyConstructorRequestsTheBuiltInSpeed() {
        RoutingProfile profile = new RoutingProfile(Vehicle.BICYCLE);

        assertEquals(Vehicle.BICYCLE, profile.vehicle);
        assertEquals(0.0, profile.maxSpeedKmh, 0.00001);
        assertFalse(profile.avoidTolls);
        assertFalse(profile.avoidFerries);
        assertFalse(profile.avoidUnpaved);
    }

    @Test
    void testVehicleAndSpeedConstructorCarriesTheSpeed() {
        RoutingProfile profile = new RoutingProfile(Vehicle.BICYCLE, 20.0);

        assertEquals(Vehicle.BICYCLE, profile.vehicle);
        assertEquals(20.0, profile.maxSpeedKmh, 0.00001);
        assertFalse(profile.avoidTolls);
        assertFalse(profile.avoidFerries);
        assertFalse(profile.avoidUnpaved);
    }

    @Test
    void testFullConstructorCarriesTheSpeedAndTheFlags() {
        RoutingProfile profile = new RoutingProfile(Vehicle.PEDESTRIAN, true, false, true, 4.0);

        assertEquals(Vehicle.PEDESTRIAN, profile.vehicle);
        assertEquals(4.0, profile.maxSpeedKmh, 0.00001);
        assertTrue(profile.avoidTolls);
        assertFalse(profile.avoidFerries);
        assertTrue(profile.avoidUnpaved);
    }

    @Test
    void testAvoidFlagConstructorWithoutASpeedKeepsTheBuiltInSpeed() {
        RoutingProfile profile = new RoutingProfile(Vehicle.CAR, true, true, true);

        assertEquals(Vehicle.CAR, profile.vehicle);
        assertEquals(0.0, profile.maxSpeedKmh, 0.00001);
        assertTrue(profile.avoidTolls);
        assertTrue(profile.avoidFerries);
        assertTrue(profile.avoidUnpaved);
    }
}
