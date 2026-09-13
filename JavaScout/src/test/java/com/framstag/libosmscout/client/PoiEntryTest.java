package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for the PoiEntry model class.
 */
class PoiEntryTest {

    @Test
    void testDefaultConstructor() {
        PoiEntry entry = new PoiEntry();
        assertNull(entry.label);
        assertNull(entry.operator);
        assertNull(entry.brand);
        assertNull(entry.objectType);
        assertEquals(0.0, entry.lat);
        assertEquals(0.0, entry.lon);
        assertEquals(0.0, entry.distance);
    }

    @Test
    void testPopulatedEntry() {
        PoiEntry entry = new PoiEntry();
        entry.label = "Hotel Central";
        entry.operator = "ACME Hotels";
        entry.brand = "ACME";
        entry.objectType = "tourism_hotel";
        entry.lat = 52.0;
        entry.lon = 8.0;
        entry.distance = 1234.5;

        assertEquals("Hotel Central", entry.label);
        assertEquals("ACME Hotels", entry.operator);
        assertEquals("ACME", entry.brand);
        assertEquals("tourism_hotel", entry.objectType);
        assertEquals(52.0, entry.lat);
        assertEquals(8.0, entry.lon);
        assertEquals(1234.5, entry.distance);
    }

    @Test
    void testOperatorOnlyEntry() {
        PoiEntry entry = new PoiEntry();
        entry.label = "";
        entry.operator = "Sparkasse";
        entry.brand = null;
        entry.objectType = "amenity_atm";

        assertEquals("", entry.label);
        assertEquals("Sparkasse", entry.operator);
        assertNull(entry.brand);
        assertEquals("amenity_atm", entry.objectType);
    }

    @Test
    void testUnnamedEntry() {
        PoiEntry entry = new PoiEntry();
        entry.label = "";
        entry.objectType = "shop";

        assertEquals("", entry.label);
        assertEquals("shop", entry.objectType);
    }
}
