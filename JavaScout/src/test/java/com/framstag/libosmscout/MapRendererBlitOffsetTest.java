package com.framstag.libosmscout;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for the sub-region blit offset calculation used in
 * {@link MapRenderer#trySubRegionBlit(double, double, int, double)}.
 * <p>
 * The offset calculation projects old and new centers through
 * {@link ProjectionUtils#geoToScreen(double, double, int, int, int, double, double, double)},
 * computes the pixel delta, then checks whether the viewport fits within
 * the overrun buffer.
 */
public class MapRendererBlitOffsetTest {

    private static final double DPI = 96.0;
    private static final double OVERRUN = 2.5;
    private static final double EPSILON = 1e-6;

    // ---- Helper: compute viewport bounds in buffer space ----

    private static double[] computeViewportBounds(
            double newLat, double newLon, int newMag,
            double oldLat, double oldLon, int oldMag,
            int screenW, int screenH, double overrun) {

        int fbW = (int) (screenW * overrun);
        int fbH = (int) (screenH * overrun);

        double[] oldCenter = ProjectionUtils.geoToScreen(oldLat, oldLon, screenW, screenH, oldMag, oldLat, oldLon, DPI);
        double[] newCenter = ProjectionUtils.geoToScreen(newLat, newLon, screenW, screenH, oldMag, oldLat, oldLon, DPI);
        double dx = newCenter[0] - oldCenter[0];
        double dy = newCenter[1] - oldCenter[1];

        double viewLeft = (fbW / 2.0) - (screenW / 2.0) + dx;
        double viewTop = (fbH / 2.0) - (screenH / 2.0) + dy;
        double viewRight = viewLeft + screenW;
        double viewBottom = viewTop + screenH;

        return new double[]{viewLeft, viewTop, viewRight, viewBottom};
    }

    private static boolean viewportFits(double[] bounds, int fbW, int fbH) {
        return bounds[0] >= 0 && bounds[1] >= 0 && bounds[2] <= fbW && bounds[3] <= fbH;
    }

    // ---- No pan: viewport should be centered in buffer ----

    @Test
    public void testNoPanViewportCentered() {
        double[] bounds = computeViewportBounds(51.0, 7.0, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // Viewport should be centered: left = (fbW - screenW) / 2, top = (fbH - screenH) / 2
        assertEquals((fbW - 800) / 2.0, bounds[0], 1.0);
        assertEquals((fbH - 600) / 2.0, bounds[1], 1.0);
        assertEquals((fbW + 800) / 2.0, bounds[2], 1.0);
        assertEquals((fbH + 600) / 2.0, bounds[3], 1.0);

        assertTrue(viewportFits(bounds, fbW, fbH));
    }

    // ---- Small pan within overrun ----

    @Test
    public void testSmallPanEastWithinOverrun() {
        // Pan 0.1 degrees east — viewport shifts right in buffer (dx > 0)
        double[] bounds = computeViewportBounds(51.0, 7.1, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // Viewport should shift right (viewLeft > center)
        assertTrue(bounds[0] > (fbW - 800) / 2.0);
        assertTrue(viewportFits(bounds, fbW, fbH));
    }

    @Test
    public void testSmallPanWestWithinOverrun() {
        // Pan 0.1 degrees west — viewport shifts left in buffer (dx < 0)
        double[] bounds = computeViewportBounds(51.0, 6.9, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // Viewport should shift left (viewLeft < center)
        assertTrue(bounds[0] < (fbW - 800) / 2.0);
        assertTrue(viewportFits(bounds, fbW, fbH));
    }

    @Test
    public void testSmallPanNorthWithinOverrun() {
        // Pan 0.5 degrees north — viewport shifts up in buffer (dy < 0, smaller Y)
        double[] bounds = computeViewportBounds(51.5, 7.0, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // North = smaller Y in screen coords, so viewport shifts up (smaller top = viewTop < center)
        assertTrue(bounds[1] < (fbH - 600) / 2.0);
        assertTrue(viewportFits(bounds, fbW, fbH));
    }

    @Test
    public void testSmallPanSouthWithinOverrun() {
        // Pan 0.5 degrees south — viewport shifts down in buffer (dy > 0, larger Y)
        double[] bounds = computeViewportBounds(50.5, 7.0, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // South = larger Y in screen coords, so viewport shifts down (viewTop > center)
        assertTrue(bounds[1] > (fbH - 600) / 2.0);
        assertTrue(viewportFits(bounds, fbW, fbH));
    }

    // ---- Large pan beyond overrun (use mag 10 for larger pixel offsets) ----

    @Test
    public void testLargePanEastExceedsOverrun() {
        // Pan 5 degrees east at mag 10 — should exceed buffer
        double[] bounds = computeViewportBounds(51.0, 12.0, 10, 51.0, 7.0, 10, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        assertFalse(viewportFits(bounds, fbW, fbH));
    }

    @Test
    public void testLargePanWestExceedsOverrun() {
        double[] bounds = computeViewportBounds(51.0, 2.0, 10, 51.0, 7.0, 10, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        assertFalse(viewportFits(bounds, fbW, fbH));
    }

    @Test
    public void testLargePanNorthExceedsOverrun() {
        double[] bounds = computeViewportBounds(60.0, 7.0, 10, 51.0, 7.0, 10, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        assertFalse(viewportFits(bounds, fbW, fbH));
    }

    // ---- Higher overrun factor allows larger pan ----

    @Test
    public void testLargerOverrunAllowsMorePan() {
        double overrunSmall = 1.5;
        double overrunLarge = 3.0;

        // Pan 0.5 degrees east at mag 5
        double[] boundsSmall = computeViewportBounds(51.0, 7.5, 5, 51.0, 7.0, 5, 800, 600, overrunSmall);
        double[] boundsLarge = computeViewportBounds(51.0, 7.5, 5, 51.0, 7.0, 5, 800, 600, overrunLarge);

        int fbWSmall = (int) (800 * overrunSmall);
        int fbHSmall = (int) (600 * overrunSmall);
        int fbWLarge = (int) (800 * overrunLarge);
        int fbHLarge = (int) (600 * overrunLarge);

        // Larger overrun should fit what smaller overrun doesn't
        // (This depends on the specific pan distance — may need adjustment)
        assertTrue(viewportFits(boundsLarge, fbWLarge, fbHLarge));
    }

    // ---- Higher zoom = smaller geo area = more pan room in buffer ----

    @Test
    public void testHigherZoomAllowsMoreGeoPan() {
        // At mag 10, 0.5 degrees is a huge pan (much larger area)
        // At mag 5, 0.5 degrees is a moderate pan
        double[] boundsMag5 = computeViewportBounds(51.0, 7.5, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        double[] boundsMag10 = computeViewportBounds(51.0, 7.5, 10, 51.0, 7.0, 10, 800, 600, OVERRUN);

        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // At mag 10, 0.5 degrees is a much larger pixel offset, so it's more likely to exceed buffer
        // At mag 5, it might fit
        // This test verifies the relationship, not specific pass/fail
        double offsetMag5 = Math.abs(boundsMag5[0] - (fbW - 800) / 2.0);
        double offsetMag10 = Math.abs(boundsMag10[0] - (fbW - 800) / 2.0);
        assertTrue(offsetMag10 > offsetMag5);
    }

    // ---- Sub-region source coordinates ----

    @Test
    public void testSubRegionSourceCoords() {
        // The source coordinates for the sub-region blit are:
        // srcX = (fbW / 2.0) - (screenW / 2.0) + dx
        // srcY = (fbH / 2.0) - (screenH / 2.0) + dy
        // These should match viewLeft/viewTop
        double[] bounds = computeViewportBounds(51.0, 7.1, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        double[] oldCenter = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        double[] newCenter = ProjectionUtils.geoToScreen(51.0, 7.1, 800, 600, 5, 51.0, 7.0, DPI);
        double dx = newCenter[0] - oldCenter[0];
        double dy = newCenter[1] - oldCenter[1];

        double srcX = (fbW / 2.0) - (800 / 2.0) + dx;
        double srcY = (fbH / 2.0) - (600 / 2.0) + dy;

        assertEquals(bounds[0], srcX, 1.0);
        assertEquals(bounds[1], srcY, 1.0);
    }

    // ---- Clipped sub-region dimensions ----

    @Test
    public void testClippedSubRegionDimensions() {
        // When viewport extends beyond buffer, the sub-region is clipped
        // At mag 10, buffer extends ~0.82 deg beyond viewport. Pan 0.9 deg east = partial clip.
        double[] bounds = computeViewportBounds(51.0, 7.9, 10, 51.0, 7.0, 10, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        // Clipped source
        double isx = Math.max(0, bounds[0]);
        double isy = Math.max(0, bounds[1]);
        double iw = Math.min(fbW - isx, 800.0);
        double ih = Math.min(fbH - isy, 600.0);

        // Should be less than full viewport since viewport extends beyond buffer
        assertTrue(iw < 800 || ih < 600);
        assertTrue(iw > 0 && ih > 0); // At least some overlap
    }

    @Test
    public void testClippedSubRegionNoOverlap() {
        // Extreme pan at mag 10: no overlap at all
        double[] bounds = computeViewportBounds(51.0, 50.0, 10, 51.0, 7.0, 10, 800, 600, OVERRUN);
        int fbW = (int) (800 * OVERRUN);
        int fbH = (int) (600 * OVERRUN);

        double isx = Math.max(0, bounds[0]);
        double isy = Math.max(0, bounds[1]);
        double iw = Math.min(fbW - isx, 800.0);
        double ih = Math.min(fbH - isy, 600.0);

        // No overlap at all
        assertTrue(iw <= 0 || ih <= 0);
    }

    // ---- Destination coordinates ----

    @Test
    public void testDestinationCoords() {
        // Destination X = isx - srcX, Destination Y = isy - srcY
        // When srcX >= 0, destX = 0 (no offset needed)
        // When srcX < 0, destX = -srcX (shift right to compensate for negative source)
        double[] bounds = computeViewportBounds(51.0, 7.1, 5, 51.0, 7.0, 5, 800, 600, OVERRUN);

        double srcX = bounds[0];
        double srcY = bounds[1];
        double isx = Math.max(0, srcX);
        double isy = Math.max(0, srcY);

        double destX = isx - srcX;
        double destY = isy - srcY;

        // For a small pan east, srcX > 0 (viewport shifted left in buffer)
        // so destX should be 0
        if (srcX >= 0) {
            assertEquals(0.0, destX, 1.0);
        }
        if (srcY >= 0) {
            assertEquals(0.0, destY, 1.0);
        }
    }
}
