package com.framstag.libosmscout;

/**
 * Shared Mercator projection utilities used by MapRenderer and MapInteractionHandler.
 * <p>
 * All methods use the WGS84 ellipsoid (Earth radius 6378137m) and match the
 * projection formula used by libosmscout's MercatorProjection.
 */
public class ProjectionUtils {

    /** Earth radius in meters (WGS84). */
    public static final double EARTH_RADIUS = 6378137.0;

    /** Reference DPI for tile resolution calculation. */
    public static final double REFERENCE_DPI = 96.0;

    /**
     * Inverse hyperbolic tangent. Java 17 doesn't have {@code Math.atanh()}.
     */
    public static double atanh(double x) {
        return 0.5 * Math.log((1.0 + x) / (1.0 - x));
    }

    /**
     * Projection scale factors for a given magnification and viewport width.
     */
    public record ProjectionScale(double scale, double scaleGradtorad) {}

    /**
     * Compute projection scale factors.
     *
     * @param mag       magnification level (zoom)
     * @param viewWidth viewport width in pixels
     * @param dpi       physical display DPI
     * @return scale factors
     */
    public static ProjectionScale computeScale(int mag, double viewWidth, double dpi) {
        double extentMeter = 2.0 * Math.PI * EARTH_RADIUS;
        double magnif = Math.pow(2, mag);
        double equatorTileWidth = extentMeter / magnif;
        double equatorTileResolution = equatorTileWidth / 256.0;
        double equatorCorrectedResolution = equatorTileResolution * REFERENCE_DPI / dpi;
        double groundWidthEquatorMeter = viewWidth * equatorCorrectedResolution;
        double scale = viewWidth / (2.0 * Math.PI * groundWidthEquatorMeter / extentMeter);
        double scaleGradtorad = scale * Math.PI / 180.0;
        return new ProjectionScale(scale, scaleGradtorad);
    }

    /**
     * Convert geographic coordinates to screen pixel coordinates using Mercator projection.
     *
     * @param lat        latitude in degrees
     * @param lon        longitude in degrees
     * @param screenW    viewport width in pixels
     * @param screenH    viewport height in pixels
     * @param mag        magnification level (zoom)
     * @param centerLat  center latitude in degrees
     * @param centerLon  center longitude in degrees
     * @param dpi        physical display DPI
     * @return [screenX, screenY] pixel coordinates
     */
    public static double[] geoToScreen(double lat, double lon, int screenW, int screenH, int mag,
                                       double centerLat, double centerLon, double dpi) {
        ProjectionScale ps = computeScale(mag, screenW, dpi);
        double latOffset = atanh(Math.sin(Math.toRadians(centerLat)));
        double cx = (lon - centerLon) * ps.scaleGradtorad;
        double cy = -(atanh(Math.sin(Math.toRadians(lat))) - latOffset) * ps.scale;
        return new double[]{screenW / 2.0 + cx, screenH / 2.0 + cy};
    }

    /**
     * Convert screen pixel coordinates to geographic coordinates using Mercator projection.
     *
     * @param screenX    screen X in pixels
     * @param screenY    screen Y in pixels
     * @param screenW    viewport width in pixels
     * @param screenH    viewport height in pixels
     * @param mag        magnification level (zoom)
     * @param centerLat  center latitude in degrees
     * @param centerLon  center longitude in degrees
     * @param dpi        physical display DPI
     * @return [lat, lon] geographic coordinates in degrees
     */
    public static double[] screenToGeo(double screenX, double screenY, int screenW, int screenH,
                                        int mag, double centerLat, double centerLon, double dpi) {
        ProjectionScale ps = computeScale(mag, screenW, dpi);
        double latOffset = atanh(Math.sin(Math.toRadians(centerLat)));
        double cx = screenX - screenW / 2.0;
        double cy = screenH / 2.0 - screenY;
        double lon = centerLon + cx / ps.scaleGradtorad;
        double lat = Math.toDegrees(Math.asin(Math.tanh(cy / ps.scale + latOffset)));
        return new double[]{lat, lon};
    }

    /**
     * Compute new map center after a mouse drag delta.
     *
     * @param dx         mouse delta X in pixels (positive = right)
     * @param dy         mouse delta Y in pixels (positive = down)
     * @param mag        magnification level (zoom)
     * @param viewWidth  viewport width in pixels
     * @param viewHeight viewport height in pixels
     * @param centerLat  current center latitude in degrees
     * @param centerLon  current center longitude in degrees
     * @param dpi        physical display DPI
     * @return [newLat, newLon] new center coordinates in degrees
     */
    public static double[] dragDeltaToNewCenter(double dx, double dy, int mag,
                                                 double viewWidth, double viewHeight,
                                                 double centerLat, double centerLon, double dpi) {
        ProjectionScale ps = computeScale(mag, viewWidth, dpi);
        double latOffset = atanh(Math.sin(Math.toRadians(centerLat)));
        double newLon = centerLon - dx / ps.scaleGradtorad;
        double newLat = Math.toDegrees(Math.asin(Math.tanh(dy / ps.scale + latOffset)));
        return new double[]{newLat, newLon};
    }

    /**
     * Compute new map center after a zoom centered on a cursor position.
     * <p>
     * The geographic coordinate under the cursor stays fixed — the center moves
     * so that the cursor's geo position maps to the same screen pixel at the
     * new magnification.
     *
     * @param cursorX    cursor X in pixels
     * @param cursorY    cursor Y in pixels
     * @param oldMag     old magnification level
     * @param newMag     new magnification level
     * @param viewW      viewport width in pixels
     * @param viewH      viewport height in pixels
     * @param centerLat  current center latitude in degrees
     * @param centerLon  current center longitude in degrees
     * @param dpi        physical display DPI
     * @return [newLat, newLon] new center coordinates in degrees
     */
    public static double[] zoomAtCursor(double cursorX, double cursorY, int oldMag, int newMag,
                                         double viewW, double viewH,
                                         double centerLat, double centerLon, double dpi) {
        // Geo coord under cursor before zoom
        double[] cursorGeo = screenToGeo(cursorX, cursorY, (int) viewW, (int) viewH,
                                          oldMag, centerLat, centerLon, dpi);
        double cursorLat = cursorGeo[0];
        double cursorLon = cursorGeo[1];

        // New center so cursor geo coord stays at same screen position
        ProjectionScale newPs = computeScale(newMag, viewW, dpi);
        double newLatOffset = atanh(Math.sin(Math.toRadians(cursorLat)));
        double dx = cursorX - viewW / 2.0;
        double dy = viewH / 2.0 - cursorY;
        double newCenterLon = cursorLon - dx / newPs.scaleGradtorad;
        double newCenterLat = Math.toDegrees(Math.asin(Math.tanh(newLatOffset - dy / newPs.scale)));
        return new double[]{newCenterLat, newCenterLon};
    }
}
