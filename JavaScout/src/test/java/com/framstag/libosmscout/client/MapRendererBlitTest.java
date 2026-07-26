package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

/**
 * Tests the sub-region blit math.
 * Verifies pixel offset computation and viewport bounds check.
 */
public class MapRendererBlitTest {

    private static final int SCREEN_W = 800;
    private static final int SCREEN_H = 577;
    private static final double OVERRUN = 1.5;
    private static final int FB_W = (int) (SCREEN_W * OVERRUN);
    private static final int FB_H = (int) (SCREEN_H * OVERRUN);
    private static final int MAG = 5;

    /** Compute pixel offset from lat/lon change (matches MapRenderer formula). */
    private static double computeDx(double oldLon, double newLon) {
        double degPerPx = 360.0 / Math.pow(2, MAG) / SCREEN_W;
        return (oldLon - newLon) / degPerPx;
    }

    private static double computeDy(double oldLat, double newLat) {
        double degPerPx = 360.0 / Math.pow(2, MAG) / SCREEN_W;
        return (newLat - oldLat) / degPerPx;
    }

    /** Check if panned viewport fits within rendered area. */
    private static boolean viewportFits(double dx, double dy) {
        double viewLeft = (FB_W / 2.0) - (SCREEN_W / 2.0) + dx;
        double viewTop = (FB_H / 2.0) - (SCREEN_H / 2.0) + dy;
        return viewLeft >= 0 && viewTop >= 0 &&
               viewLeft + SCREEN_W <= FB_W &&
               viewTop + SCREEN_H <= FB_H;
    }

    @Test
    public void testPixelOffsetMatchesMouseDrag() {
        // User drags right by 100px → interaction handler computes newLon = oldLon - 100 * degPerPx
        double degPerPx = 360.0 / Math.pow(2, MAG) / SCREEN_W;
        double oldLon = 7.46;
        double newLon = oldLon - 100 * degPerPx;

        double dx = computeDx(oldLon, newLon);
        assertEquals(100.0, dx, 0.001,
            "100px rightward drag should give dx = +100 (source shifts right)");
    }

    @Test
    public void testViewportFitsWithinOverrun() {
        assertTrue(viewportFits(50, 0), "50px right should fit");
        assertTrue(viewportFits(-50, 0), "50px left should fit");
        assertTrue(viewportFits(0, 50), "50px down should fit");
        assertTrue(viewportFits(0, -50), "50px up should fit");
        assertFalse(viewportFits(250, 0), "250px right should exceed");
        assertFalse(viewportFits(-250, 0), "250px left should exceed");
    }

    @Test
    public void testInteractionHandlerConsistency() {
        double degPerPx = 360.0 / Math.pow(2, MAG) / SCREEN_W;
        double oldLat = 51.5;
        double oldLon = 7.46;

        // Drag right 100px → lon decreases → dx positive (source shifts right)
        double newLon = oldLon - 100 * degPerPx;
        double dx = computeDx(oldLon, newLon);
        assertEquals(100.0, dx, 0.001);

        // Drag down 50px → lat increases → dy positive (source shifts down)
        double newLat = oldLat + 50 * degPerPx;
        double dy = computeDy(oldLat, newLat);
        assertEquals(50.0, dy, 0.001);
    }

    @Test
    public void testSourceRectangleComputedCorrectly() {
        double dx = 50; // pan right by 50px
        double dy = 0;

        double srcX = (FB_W / 2.0) - (SCREEN_W / 2.0) + dx;
        double srcY = (FB_H / 2.0) - (SCREEN_H / 2.0) + dy;

        assertEquals(250.0, srcX, 0.001, "srcX = 200 + 50 = 250");
        assertEquals(144.0, srcY, 0.001, "srcY should be centered vertically");
        assertTrue(srcX >= 0, "srcX must be >= 0");
        assertTrue(srcX + SCREEN_W <= FB_W, "srcX + screenW must be <= fbW");
    }
}
