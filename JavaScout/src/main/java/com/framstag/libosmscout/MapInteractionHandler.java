package com.framstag.libosmscout;

import javafx.animation.PauseTransition;
import javafx.scene.canvas.Canvas;
import javafx.scene.input.KeyCode;
import javafx.scene.input.KeyEvent;
import javafx.scene.input.MouseButton;
import javafx.scene.input.MouseEvent;
import javafx.scene.input.ScrollEvent;
import javafx.util.Duration;

/**
 * Handles keyboard and mouse interaction for the map view.
 * <p>
 * Keyboard: arrows (pan 10%), Page Up/Down (pan 50%), +/- (zoom)
 * Mouse: drag to pan, scroll wheel to zoom, long press for description
 */
public class MapInteractionHandler {

    /** Zoom level range. */
    private static final int MIN_MAG = 0;
    private static final int MAX_MAG = 18;

    private final Canvas canvas;
    private final MapRenderer renderer;
    private final double dpi;

    // Mouse drag state
    private double dragLastX;
    private double dragLastY;
    private boolean dragging = false;

    /** Callback invoked when the user starts a manual pan or zoom. */
    private Runnable onInteractionStarted;

    // Long-press state
    private final PauseTransition longPressTimer;
    private double pressX;
    private double pressY;
    private boolean longPressFired = false;

    /** Callback invoked on long press with (latitude, longitude). */
    private LongPressCallback onLongPress;

    /**
     * Callback interface for long-press events.
     */
    @FunctionalInterface
    public interface LongPressCallback {
        void onLongPress(double lat, double lon);
    }

    /**
     * Create interaction handler for the given canvas and renderer.
     *
     * @param canvas   the map Canvas to attach events to
     * @param renderer the MapRenderer to trigger re-renders
     * @param longPressTimeoutMs timeout in ms before a long press is triggered
     * @param dpi      physical DPI of the display (for coordinate conversion)
     */
    public MapInteractionHandler(Canvas canvas, MapRenderer renderer, int longPressTimeoutMs, double dpi) {
        this.canvas = canvas;
        this.renderer = renderer;
        this.dpi = dpi;

        this.longPressTimer = new PauseTransition(Duration.millis(longPressTimeoutMs));
        this.longPressTimer.setOnFinished(e -> fireLongPress());

        attachHandlers();
    }

    /**
     * Set the callback invoked on long press.
     */
    public void setOnLongPress(LongPressCallback callback) {
        this.onLongPress = callback;
    }

    /**
     * Set the callback invoked when the user starts a manual pan or zoom.
     */
    public void setOnInteractionStarted(Runnable callback) {
        this.onInteractionStarted = callback;
    }

    private void attachHandlers() {
        // Keyboard
        canvas.addEventHandler(KeyEvent.KEY_PRESSED, this::onKeyPressed);
        canvas.setFocusTraversable(true);

        // Mouse
        canvas.addEventHandler(MouseEvent.MOUSE_PRESSED, this::onMousePressed);
        canvas.addEventHandler(MouseEvent.MOUSE_DRAGGED, this::onMouseDragged);
        canvas.addEventHandler(MouseEvent.MOUSE_RELEASED, this::onMouseReleased);

        // Scroll wheel
        canvas.addEventHandler(ScrollEvent.SCROLL, this::onScroll);
    }

    private void onKeyPressed(KeyEvent event) {
        double lat = renderer.getLatitude();
        double lon = renderer.getLongitude();
        int mag = renderer.getMagnification();

        double viewWidth = canvas.getWidth();
        double viewHeight = canvas.getHeight();
        double degPerPx = 360.0 / Math.pow(2, mag) / viewWidth;

        // Pan amount: 10% of viewport for arrows, 50% for page keys
        double panFactor = (event.getCode() == KeyCode.PAGE_UP || event.getCode() == KeyCode.PAGE_DOWN)
                ? 0.5 : 0.1;
        double panDeg = panFactor * viewHeight * degPerPx;

        switch (event.getCode()) {
            case UP -> lat += panDeg;
            case DOWN -> lat -= panDeg;
            case LEFT -> lon -= panDeg;
            case RIGHT -> lon += panDeg;
            case PAGE_UP -> lat += panDeg;
            case PAGE_DOWN -> lat -= panDeg;
            case PLUS, EQUALS -> mag = Math.min(MAX_MAG, mag + 1);
            case MINUS, SUBTRACT -> mag = Math.max(MIN_MAG, mag - 1);
            default -> { return; }
        }

        event.consume();
        notifyInteractionStarted();
        renderer.requestRenderPreserveRoute(lat, lon, mag);
    }

    private void onMousePressed(MouseEvent event) {
        if (event.getButton() == MouseButton.PRIMARY) {
            dragLastX = event.getX();
            dragLastY = event.getY();
            dragging = false;
            longPressFired = false;
            pressX = event.getX();
            pressY = event.getY();

            // Start long-press timer
            longPressTimer.playFromStart();
        }
    }

    private void onMouseDragged(MouseEvent event) {
        if (event.getButton() != MouseButton.PRIMARY) return;

        // Cancel long press on drag
        if (!longPressFired) {
            longPressTimer.stop();
        }

        if (!dragging) {
            // Only start drag after a small threshold to avoid conflict with long press
            double dx = event.getX() - dragLastX;
            double dy = event.getY() - dragLastY;
            if (Math.abs(dx) > 3 || Math.abs(dy) > 3) {
                dragging = true;
                notifyInteractionStarted();
            }
        }

        if (!dragging) return;

        double dx = event.getX() - dragLastX;
        double dy = event.getY() - dragLastY;
        dragLastX = event.getX();
        dragLastY = event.getY();

        int mag = renderer.getMagnification();
        double viewWidth = canvas.getWidth();
        double viewHeight = canvas.getHeight();
        double centerLat = renderer.getLatitude();
        double centerLon = renderer.getLongitude();

        // Use Mercator projection (same formula as fireLongPress)
        double[] newCenter = ProjectionUtils.dragDeltaToNewCenter(dx, dy, mag, viewWidth, viewHeight,
                                                                   centerLat, centerLon, dpi);

        renderer.requestRenderPreserveRoute(newCenter[0], newCenter[1], mag);

        // Redraw current-location marker after pan so it tracks during drag
        renderer.drawMarker();
    }

    private void onMouseReleased(MouseEvent event) {
        if (event.getButton() == MouseButton.PRIMARY) {
            // Cancel long press on release
            if (!longPressFired) {
                longPressTimer.stop();
            }
            dragging = false;
        }
    }

    private void fireLongPress() {
        if (longPressFired) return;
        longPressFired = true;

        if (onLongPress == null) return;

        // Convert screen to geo using libosmscout MercatorProjection formula
        int mag = renderer.getMagnification();
        double viewWidth = canvas.getWidth();
        double viewHeight = canvas.getHeight();

        double centerLat = renderer.getLatitude();
        double centerLon = renderer.getLongitude();

        double[] geo = ProjectionUtils.screenToGeo(pressX, pressY, (int) viewWidth, (int) viewHeight,
                                                    mag, centerLat, centerLon, dpi);

        onLongPress.onLongPress(geo[0], geo[1]);
    }

    private void notifyInteractionStarted() {
        if (onInteractionStarted != null) {
            onInteractionStarted.run();
        }
    }

    private void onScroll(ScrollEvent event) {
        double deltaY = event.getDeltaY();
        if (deltaY == 0) return;

        int dir = deltaY > 0 ? 1 : -1;
        int oldMag = renderer.getMagnification();
        int newMag = Math.max(MIN_MAG, Math.min(MAX_MAG, oldMag + dir));
        if (newMag == oldMag) return;

        notifyInteractionStarted();

        // Zoom centered on cursor using libosmscout MercatorProjection formula
        double cx = event.getX();
        double cy = event.getY();
        double viewW = canvas.getWidth();
        double viewH = canvas.getHeight();

        double centerLat = renderer.getLatitude();
        double centerLon = renderer.getLongitude();

        double[] newCenter = ProjectionUtils.zoomAtCursor(cx, cy, oldMag, newMag, viewW, viewH,
                                                           centerLat, centerLon, dpi);

        renderer.requestRenderPreserveRoute(newCenter[0], newCenter[1], newMag);
        event.consume();
    }
}
