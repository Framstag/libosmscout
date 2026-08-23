package com.framstag.libosmscout;

import com.framstag.libosmscout.client.TrackPoint;
import javafx.application.Platform;
import javafx.scene.canvas.Canvas;
import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.lang.reflect.Field;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for {@link MapRenderer} overlay state management.
 * <p>
 * These tests do not exercise the native renderer; they verify that
 * track/route data is stored and cleared correctly and that state changes
 * trigger pending render requests.
 */
public class MapRendererStateTest {

    @BeforeAll
    public static void initJavaFx() {
        try {
            Platform.startup(() -> {});
        } catch (IllegalStateException e) {
            // JavaFX runtime may already be started by another test class.
        }
    }

    @AfterEach
    public void shutdownRenderer() {
        // Each test is responsible for shutting down its own renderer.
    }

    @Test
    public void testSetTrackPointsStoresArrays() throws Exception {
        Canvas canvas = new Canvas(100, 100);
        MapRenderer renderer = new MapRenderer(canvas, null);
        try {
            TrackPoint[] points = {
                new TrackPoint(51.514227, 7.465279, "2024-01-01T10:00:00Z"),
                new TrackPoint(51.515, 7.466)
            };

            renderer.setTrackPoints(points);

            double[] lats = (double[]) getField(renderer, "trackLats");
            double[] lons = (double[]) getField(renderer, "trackLons");
            assertNotNull(lats);
            assertNotNull(lons);
            assertEquals(2, lats.length);
            assertEquals(2, lons.length);
            assertEquals(51.514227, lats[0], 0.00001);
            assertEquals(7.465279, lons[0], 0.00001);
            assertEquals(51.515, lats[1], 0.00001);
            assertEquals(7.466, lons[1], 0.00001);

            assertNotNull(getField(renderer, "pendingRender"),
                "setTrackPoints should schedule a render request");
        } finally {
            renderer.shutdown();
        }
    }

    @Test
    public void testSetTrackPointsWithNullClearsArrays() throws Exception {
        Canvas canvas = new Canvas(100, 100);
        MapRenderer renderer = new MapRenderer(canvas, null);
        try {
            renderer.setTrackPoints(new TrackPoint[] {
                new TrackPoint(51.0, 7.0)
            });
            assertNotNull(getField(renderer, "trackLats"));

            renderer.setTrackPoints(null);

            assertNull(getField(renderer, "trackLats"));
            assertNull(getField(renderer, "trackLons"));
        } finally {
            renderer.shutdown();
        }
    }

    @Test
    public void testClearTrackRemovesArraysAndRequestsRender() throws Exception {
        Canvas canvas = new Canvas(100, 100);
        MapRenderer renderer = new MapRenderer(canvas, null);
        try {
            renderer.setTrackPoints(new TrackPoint[] {
                new TrackPoint(51.0, 7.0)
            });

            renderer.clearTrack();

            assertNull(getField(renderer, "trackLats"));
            assertNull(getField(renderer, "trackLons"));
            assertNotNull(getField(renderer, "pendingRender"),
                "clearTrack should schedule a render request");
        } finally {
            renderer.shutdown();
        }
    }

    @Test
    public void testClearRouteOverlayRemovesArraysAndRequestsRender() throws Exception {
        Canvas canvas = new Canvas(100, 100);
        MapRenderer renderer = new MapRenderer(canvas, null);
        try {
            // Simulate an active route by setting the fields directly.
            setField(renderer, "routeLats", new double[] { 51.0, 51.1 });
            setField(renderer, "routeLons", new double[] { 7.0, 7.1 });

            renderer.clearRouteOverlay();

            assertNull(getField(renderer, "routeLats"));
            assertNull(getField(renderer, "routeLons"));
            assertNotNull(getField(renderer, "pendingRender"),
                "clearRouteOverlay should schedule a render request");
        } finally {
            renderer.shutdown();
        }
    }

    @Test
    public void testNotifyStyleChangedBumpsEpochAndRequestsRender() throws Exception {
        Canvas canvas = new Canvas(100, 100);
        MapRenderer renderer = new MapRenderer(canvas, null);
        try {
            java.util.concurrent.atomic.AtomicLong epoch =
                (java.util.concurrent.atomic.AtomicLong) getField(renderer, "epoch");
            long epochBefore = epoch.get();

            renderer.notifyStyleChanged();

            assertEquals(epochBefore + 1, epoch.get(),
                "notifyStyleChanged must bump the epoch to invalidate cached tiles");
            assertNotNull(getField(renderer, "pendingRender"),
                "notifyStyleChanged should schedule a render request");
        } finally {
            renderer.shutdown();
        }
    }

    private static Object getField(Object object, String name) throws Exception {
        Field field = MapRenderer.class.getDeclaredField(name);
        field.setAccessible(true);
        return field.get(object);
    }

    private static void setField(Object object, String name, Object value) throws Exception {
        Field field = MapRenderer.class.getDeclaredField(name);
        field.setAccessible(true);
        field.set(object, value);
    }
}
