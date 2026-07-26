package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for {@link TrackPoint}.
 */
public class TrackPointTest {

    @Test
    public void testDefaultConstructor() {
        TrackPoint point = new TrackPoint();
        assertEquals(0.0, point.lat, 0.00001);
        assertEquals(0.0, point.lon, 0.00001);
        assertNull(point.timestamp);
    }

    @Test
    public void testConstructorWithCoordinates() {
        TrackPoint point = new TrackPoint(51.514227, 7.465279);
        assertEquals(51.514227, point.lat, 0.00001);
        assertEquals(7.465279, point.lon, 0.00001);
        assertNull(point.timestamp);
    }

    @Test
    public void testConstructorWithTimestamp() {
        TrackPoint point = new TrackPoint(51.514227, 7.465279, "2024-01-01T10:00:00Z");
        assertEquals(51.514227, point.lat, 0.00001);
        assertEquals(7.465279, point.lon, 0.00001);
        assertEquals("2024-01-01T10:00:00Z", point.timestamp);
    }
}
