package com.framstag.libosmscout;

import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for {@link ProjectionUtils} Mercator projection math.
 * <p>
 * Tests verify atanh, geoToScreen, screenToGeo, dragDeltaToNewCenter,
 * zoomAtCursor, and computeScale against known values and round-trip consistency.
 */
public class ProjectionUtilsTest {

    private static final double DPI = 96.0;
    private static final double EPSILON = 1e-6;

    // ---- atanh ----

    @Test
    public void testAtanhZero() {
        assertEquals(0.0, ProjectionUtils.atanh(0.0), EPSILON);
    }

    @Test
    public void testAtanhPositive() {
        // atanh(0.5) = 0.5 * ln((1+0.5)/(1-0.5)) = 0.5 * ln(3) ≈ 0.549306
        double expected = 0.5 * Math.log(3.0);
        assertEquals(expected, ProjectionUtils.atanh(0.5), EPSILON);
    }

    @Test
    public void testAtanhNegative() {
        // atanh(-0.5) = -atanh(0.5)
        assertEquals(-ProjectionUtils.atanh(0.5), ProjectionUtils.atanh(-0.5), EPSILON);
    }

    @Test
    public void testAtanhNearOne() {
        // atanh(0.999) should be large but finite
        double result = ProjectionUtils.atanh(0.999);
        assertTrue(result > 3.0);
        assertTrue(result < 10.0);
    }

    // ---- computeScale ----

    @Test
    public void testComputeScaleMag5() {
        ProjectionUtils.ProjectionScale ps = ProjectionUtils.computeScale(5, 800, DPI);
        assertTrue(ps.scale() > 0);
        assertTrue(ps.scaleGradtorad() > 0);
    }

    @Test
    public void testComputeScaleHigherMagLargerScale() {
        ProjectionUtils.ProjectionScale ps5 = ProjectionUtils.computeScale(5, 800, DPI);
        ProjectionUtils.ProjectionScale ps10 = ProjectionUtils.computeScale(10, 800, DPI);
        // Higher magnification = larger scale (more pixels per degree)
        assertTrue(ps10.scale() > ps5.scale());
    }

    @Test
    public void testComputeScaleWiderViewSameScale() {
        // Scale is independent of viewWidth (viewWidth cancels out in formula)
        ProjectionUtils.ProjectionScale ps800 = ProjectionUtils.computeScale(5, 800, DPI);
        ProjectionUtils.ProjectionScale ps1600 = ProjectionUtils.computeScale(5, 1600, DPI);
        assertEquals(ps800.scale(), ps1600.scale(), 1e-6);
        assertEquals(ps800.scaleGradtorad(), ps1600.scaleGradtorad(), 1e-6);
    }

    // ---- geoToScreen ----

    @Test
    public void testGeoToScreenCenter() {
        // Center of screen should map to center of screen
        double[] pt = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        assertEquals(400.0, pt[0], 1.0);  // screenW / 2
        assertEquals(300.0, pt[1], 1.0);  // screenH / 2
    }

    @Test
    public void testGeoToScreenEastOfCenter() {
        // Point east of center should have larger X
        double[] center = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        double[] east = ProjectionUtils.geoToScreen(51.0, 7.5, 800, 600, 5, 51.0, 7.0, DPI);
        assertTrue(east[0] > center[0]);
    }

    @Test
    public void testGeoToScreenWestOfCenter() {
        // Point west of center should have smaller X
        double[] center = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        double[] west = ProjectionUtils.geoToScreen(51.0, 6.5, 800, 600, 5, 51.0, 7.0, DPI);
        assertTrue(west[0] < center[0]);
    }

    @Test
    public void testGeoToScreenNorthOfCenter() {
        // Point north of center should have smaller Y (screen Y increases downward)
        double[] center = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        double[] north = ProjectionUtils.geoToScreen(51.5, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        assertTrue(north[1] < center[1]);
    }

    @Test
    public void testGeoToScreenSouthOfCenter() {
        // Point south of center should have larger Y
        double[] center = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        double[] south = ProjectionUtils.geoToScreen(50.5, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        assertTrue(south[1] > center[1]);
    }

    @Test
    public void testGeoToScreenHigherMagMorePixels() {
        // At higher magnification, same geo delta = more pixel delta
        double[] pt5 = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, DPI);
        double[] east5 = ProjectionUtils.geoToScreen(51.0, 7.1, 800, 600, 5, 51.0, 7.0, DPI);
        double[] pt10 = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 10, 51.0, 7.0, DPI);
        double[] east10 = ProjectionUtils.geoToScreen(51.0, 7.1, 800, 600, 10, 51.0, 7.0, DPI);
        double delta5 = Math.abs(east5[0] - pt5[0]);
        double delta10 = Math.abs(east10[0] - pt10[0]);
        assertTrue(delta10 > delta5);
    }

    // ---- screenToGeo ----

    @Test
    public void testScreenToGeoCenter() {
        // Center of screen should map to center geo
        double[] geo = ProjectionUtils.screenToGeo(400, 300, 800, 600, 5, 51.0, 7.0, DPI);
        assertEquals(51.0, geo[0], 1e-4);
        assertEquals(7.0, geo[1], 1e-4);
    }

    @Test
    public void testScreenToGeoRightOfCenter() {
        // Right of center should be east
        double[] center = ProjectionUtils.screenToGeo(400, 300, 800, 600, 5, 51.0, 7.0, DPI);
        double[] right = ProjectionUtils.screenToGeo(500, 300, 800, 600, 5, 51.0, 7.0, DPI);
        assertTrue(right[1] > center[1]); // longitude increases eastward
    }

    @Test
    public void testScreenToGeoAboveCenter() {
        // Above center should be north
        double[] center = ProjectionUtils.screenToGeo(400, 300, 800, 600, 5, 51.0, 7.0, DPI);
        double[] above = ProjectionUtils.screenToGeo(400, 200, 800, 600, 5, 51.0, 7.0, DPI);
        assertTrue(above[0] > center[0]); // latitude increases northward
    }

    // ---- Round-trip: geoToScreen -> screenToGeo ----

    @Test
    public void testRoundTripGeoToScreenToGeo() {
        double origLat = 52.0;
        double origLon = 8.0;
        double[] screen = ProjectionUtils.geoToScreen(origLat, origLon, 800, 600, 5, 51.0, 7.0, DPI);
        double[] geo = ProjectionUtils.screenToGeo(screen[0], screen[1], 800, 600, 5, 51.0, 7.0, DPI);
        assertEquals(origLat, geo[0], 1e-4);
        assertEquals(origLon, geo[1], 1e-4);
    }

    @Test
    public void testRoundTripScreenToGeoToScreen() {
        double origX = 500;
        double origY = 200;
        double[] geo = ProjectionUtils.screenToGeo(origX, origY, 800, 600, 5, 51.0, 7.0, DPI);
        double[] screen = ProjectionUtils.geoToScreen(geo[0], geo[1], 800, 600, 5, 51.0, 7.0, DPI);
        assertEquals(origX, screen[0], 1.0);
        assertEquals(origY, screen[1], 1.0);
    }

    @Test
    public void testRoundTripAtDifferentZoom() {
        double origLat = 48.8566; // Paris
        double origLon = 2.3522;
        double[] screen = ProjectionUtils.geoToScreen(origLat, origLon, 1024, 768, 8, 48.8566, 2.3522, DPI);
        double[] geo = ProjectionUtils.screenToGeo(screen[0], screen[1], 1024, 768, 8, 48.8566, 2.3522, DPI);
        assertEquals(origLat, geo[0], 1e-4);
        assertEquals(origLon, geo[1], 1e-4);
    }

    // ---- dragDeltaToNewCenter ----

    @Test
    public void testDragDeltaRightMovesCenterWest() {
        // Dragging right means map moves left, so center moves west (lon decreases)
        double[] newCenter = ProjectionUtils.dragDeltaToNewCenter(100, 0, 5, 800, 600, 51.0, 7.0, DPI);
        assertTrue(newCenter[1] < 7.0);
    }

    @Test
    public void testDragDeltaDownMovesCenterNorth() {
        // Dragging down means map moves up, so center moves north (lat increases)
        double[] newCenter = ProjectionUtils.dragDeltaToNewCenter(0, 100, 5, 800, 600, 51.0, 7.0, DPI);
        assertTrue(newCenter[0] > 51.0);
    }

    @Test
    public void testDragDeltaZeroNoChange() {
        double[] newCenter = ProjectionUtils.dragDeltaToNewCenter(0, 0, 5, 800, 600, 51.0, 7.0, DPI);
        assertEquals(51.0, newCenter[0], 1e-6);
        assertEquals(7.0, newCenter[1], 1e-6);
    }

    // ---- zoomAtCursor ----

    @Test
    public void testZoomAtCursorCenter() {
        // Zoom centered on screen center should keep same center
        double[] newCenter = ProjectionUtils.zoomAtCursor(400, 300, 5, 6, 800, 600, 51.0, 7.0, DPI);
        assertEquals(51.0, newCenter[0], 1e-4);
        assertEquals(7.0, newCenter[1], 1e-4);
    }

    @Test
    public void testZoomAtCursorRightEdge() {
        // Zoom centered on right edge should move center east
        double[] newCenter = ProjectionUtils.zoomAtCursor(700, 300, 5, 6, 800, 600, 51.0, 7.0, DPI);
        assertTrue(newCenter[1] > 7.0);
    }

    @Test
    public void testZoomAtCursorTopEdge() {
        // Zoom centered on top edge should move center north
        double[] newCenter = ProjectionUtils.zoomAtCursor(400, 100, 5, 6, 800, 600, 51.0, 7.0, DPI);
        assertTrue(newCenter[0] > 51.0);
    }

    // ---- Edge cases ----

    @Test
    public void testGeoToScreenEquator() {
        // At equator, sin(lat) = 0, atanh(0) = 0, so latOffset = 0
        double[] pt = ProjectionUtils.geoToScreen(0.0, 0.0, 800, 600, 5, 0.0, 0.0, DPI);
        assertEquals(400.0, pt[0], 1.0);
        assertEquals(300.0, pt[1], 1.0);
    }

    @Test
    public void testScreenToGeoEquator() {
        double[] geo = ProjectionUtils.screenToGeo(400, 300, 800, 600, 5, 0.0, 0.0, DPI);
        assertEquals(0.0, geo[0], 1e-4);
        assertEquals(0.0, geo[1], 1e-4);
    }

    @Test
    public void testGeoToScreenDifferentDPI() {
        // Higher DPI = more pixels per degree = larger pixel offset for same geo delta
        double[] pt96 = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, 96.0);
        double[] east96 = ProjectionUtils.geoToScreen(51.0, 7.1, 800, 600, 5, 51.0, 7.0, 96.0);
        double[] pt192 = ProjectionUtils.geoToScreen(51.0, 7.0, 800, 600, 5, 51.0, 7.0, 192.0);
        double[] east192 = ProjectionUtils.geoToScreen(51.0, 7.1, 800, 600, 5, 51.0, 7.0, 192.0);
        double delta96 = Math.abs(east96[0] - pt96[0]);
        double delta192 = Math.abs(east192[0] - pt192[0]);
        assertTrue(delta192 > delta96);
    }
}
