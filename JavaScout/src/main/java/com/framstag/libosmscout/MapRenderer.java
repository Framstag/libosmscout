package com.framstag.libosmscout;

import com.framstag.libosmscout.client.OSMScoutClient;

import javafx.application.Platform;
import javafx.scene.canvas.Canvas;
import javafx.scene.canvas.GraphicsContext;
import javafx.scene.image.PixelFormat;
import javafx.scene.image.WritableImage;
import javafx.scene.image.WritablePixelFormat;
import javafx.scene.paint.Color;
import javafx.stage.Screen;

import java.nio.IntBuffer;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicLong;
import java.util.concurrent.atomic.AtomicReference;
import java.util.concurrent.locks.ReentrantLock;

public class MapRenderer {

    public static final int DEFAULT_MAGNIFICATION = 5;
    public static final double DEFAULT_LATITUDE = 51.5142273;
    public static final double DEFAULT_LONGITUDE = 7.4652789;
    public static final double DEFAULT_CANVAS_OVERRUN = 2.5;

    private static final long PAN_DEBOUNCE_MS = 50;
    private static final long ZOOM_ROTATE_DEBOUNCE_MS = 200;

    private static final WritablePixelFormat<IntBuffer> ARGB_FORMAT =
            PixelFormat.getIntArgbPreInstance();

    private final Canvas canvas;
    private final OSMScoutClient client;
    private final double dpi;

    private volatile double currentLat = DEFAULT_LATITUDE;
    private volatile double currentLon = DEFAULT_LONGITUDE;
    private volatile int currentMag = DEFAULT_MAGNIFICATION;
    private volatile double currentAngle = 0.0;

    private volatile double[] routeLats;
    private volatile double[] routeLons;
    private volatile double[] favoriteLats;
    private volatile double[] favoriteLons;
    private volatile double searchSelectedLat = Double.NaN;
    private volatile double searchSelectedLon = Double.NaN;
    private volatile double[] trackLats;
    private volatile double[] trackLons;
    private volatile double locationLat = Double.NaN;
    private volatile double locationLon = Double.NaN;
    private volatile double locationBearing = Double.NaN;
    private volatile double locationAccuracy = -1.0;

    private volatile double canvasOverrun = DEFAULT_CANVAS_OVERRUN;

    private final ReentrantLock bufferLock = new ReentrantLock();
    private WritableImage backBuffer;
    private WritableImage frontBuffer;
    private long frontBufferEpoch;
    private double frontBufferLat;
    private double frontBufferLon;
    private int frontBufferMag;
    private double frontBufferAngle;

    private final TileCache tileCache = new TileCache();

    private final AtomicReference<RenderJob> pendingJob = new AtomicReference<>(null);
    private final AtomicBoolean renderThreadRunning = new AtomicBoolean(true);
    private Thread renderThread;

    private final AtomicLong epoch = new AtomicLong(0);

    private final AtomicReference<DebounceState> debounceState = new AtomicReference<>(DebounceState.IDLE);
    private final Object debounceLock = new Object();
    private final Object renderLock = new Object();
    private Thread debounceThread;
    private volatile boolean running = true;
    private volatile boolean initialRenderPending = true;

    private final List<ViewChangeListener> listeners = new ArrayList<>();

    @FunctionalInterface
    public interface ViewChangeListener {
        void onViewChanged(double lat, double lon, int mag, double angle);
    }

    private record RenderJob(
        double lat, double lon, int mag, double angle,
        double[] routeLats, double[] routeLons,
        double[] favoriteLats, double[] favoriteLons,
        double searchSelectedLat, double searchSelectedLon,
        double[] trackLats, double[] trackLons,
        int width, int height, long epoch
    ) {
        boolean hasOverlays() {
            return (routeLats != null && routeLons != null && routeLats.length > 0) ||
                   (favoriteLats != null && favoriteLons != null && favoriteLats.length > 0) ||
                   (trackLats != null && trackLons != null && trackLats.length > 0) ||
                   !Double.isNaN(searchSelectedLat);
        }
    }

    private enum DebounceState { IDLE, PAN_WAITING, ZOOM_ROTATE_WAITING }

    public void addViewChangeListener(ViewChangeListener listener) { listeners.add(listener); }
    public void removeViewChangeListener(ViewChangeListener listener) { listeners.remove(listener); }

    public MapRenderer(Canvas canvas, OSMScoutClient client) {
        this.canvas = canvas;
        this.client = client;
        this.dpi = Screen.getPrimary().getDpi();
        startRenderThread();
        startDebounceLoop();

        // When canvas gets its first non-zero size, fire the initial render.
        // enqueueRenderJob() silently drops renders when canvas size is 0,
        // so we need to retry once the layout completes.
        canvas.widthProperty().addListener((obs, oldVal, newVal) -> {
            if (initialRenderPending && newVal != null && newVal.doubleValue() > 0
                && canvas.getHeight() > 0) {
                initialRenderPending = false;
                enqueueRenderJob(currentLat, currentLon, currentMag, currentAngle);
            }
        });
        canvas.heightProperty().addListener((obs, oldVal, newVal) -> {
            if (initialRenderPending && newVal != null && newVal.doubleValue() > 0
                && canvas.getWidth() > 0) {
                initialRenderPending = false;
                enqueueRenderJob(currentLat, currentLon, currentMag, currentAngle);
            }
        });
    }

    public void requestRenderPreserveRoute(double lat, double lon, int mag) {
        requestRenderPreserveRoute(lat, lon, mag, currentAngle);
    }

    public void requestRenderPreserveRoute(double lat, double lon, int mag, double angle) {
        double oldLat = currentLat;
        double oldLon = currentLon;
        int oldMag = currentMag;
        double oldAngle = currentAngle;
        currentLat = lat;
        currentLon = lon;
        currentMag = mag;
        currentAngle = angle;
        submitDebounced(lat, lon, mag, angle, oldLat, oldLon, oldMag, oldAngle, false);
    }

    public void requestRender(double lat, double lon, int mag) {
        requestRender(lat, lon, mag, currentAngle, null, null);
    }

    public void requestRender(double lat, double lon, int mag, double angle) {
        requestRender(lat, lon, mag, angle, null, null);
    }

    public void requestRender(double lat, double lon, int mag,
                              double[] routeLats, double[] routeLons) {
        requestRender(lat, lon, mag, currentAngle, routeLats, routeLons);
    }

    public void requestRender(double lat, double lon, int mag, double angle,
                              double[] routeLats, double[] routeLons) {
        double oldLat = currentLat;
        double oldLon = currentLon;
        int oldMag = currentMag;
        double oldAngle = currentAngle;
        this.routeLats = routeLats;
        this.routeLons = routeLons;
        currentLat = lat;
        currentLon = lon;
        currentMag = mag;
        currentAngle = angle;
        submitDebounced(lat, lon, mag, angle, oldLat, oldLon, oldMag, oldAngle, false);
    }

    public void setFavoriteLocations(com.framstag.libosmscout.client.FavoriteLocation[] favorites) {
        if (favorites == null || favorites.length == 0) {
            this.favoriteLats = null;
            this.favoriteLons = null;
        } else {
            double[] lats = new double[favorites.length];
            double[] lons = new double[favorites.length];
            for (int i = 0; i < favorites.length; i++) {
                lats[i] = favorites[i].lat;
                lons[i] = favorites[i].lon;
            }
            this.favoriteLats = lats;
            this.favoriteLons = lons;
        }
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void setSearchSelected(double lat, double lon) {
        this.searchSelectedLat = lat;
        this.searchSelectedLon = lon;
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void clearSearchSelected() {
        this.searchSelectedLat = Double.NaN;
        this.searchSelectedLon = Double.NaN;
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void clearRouteOverlay() {
        routeLats = null;
        routeLons = null;
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void setTrackPoints(com.framstag.libosmscout.client.TrackPoint[] points) {
        if (points == null || points.length == 0) {
            this.trackLats = null;
            this.trackLons = null;
        } else {
            double[] lats = new double[points.length];
            double[] lons = new double[points.length];
            for (int i = 0; i < points.length; i++) {
                lats[i] = points[i].lat;
                lons[i] = points[i].lon;
            }
            this.trackLats = lats;
            this.trackLons = lons;
        }
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void clearTrack() {
        trackLats = null;
        trackLons = null;
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void setCurrentLocation(double lat, double lon, double bearing, double accuracy) {
        this.locationLat = lat;
        this.locationLon = lon;
        this.locationBearing = bearing;
        this.locationAccuracy = accuracy;
        Platform.runLater(this::drawMarker);
    }

    public void clearCurrentLocation() {
        this.locationLat = Double.NaN;
        this.locationLon = Double.NaN;
        this.locationBearing = Double.NaN;
        this.locationAccuracy = -1.0;
        Platform.runLater(this::drawMarker);
    }

    public double getLatitude() { return currentLat; }
    public double getLongitude() { return currentLon; }
    public int getMagnification() { return currentMag; }
    public double getAngle() { return currentAngle; }

    public void setAngle(double angle) {
        double oldAngle = currentAngle;
        currentAngle = angle;
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, angle,
                         currentLat, currentLon, currentMag, oldAngle, true);
    }

    public double getCanvasOverrun() { return canvasOverrun; }

    public void setCanvasOverrun(double overrun) {
        this.canvasOverrun = overrun;
        epoch.incrementAndGet();
        submitDebounced(currentLat, currentLon, currentMag, currentAngle,
                         currentLat, currentLon, currentMag, currentAngle, true);
    }

    public void shutdown() {
        running = false;
        renderThreadRunning.set(false);
        if (debounceThread != null) {
            debounceThread.interrupt();
            try { debounceThread.join(1000); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        }
        if (renderThread != null) {
            renderThread.interrupt();
            try { renderThread.join(1000); } catch (InterruptedException e) { Thread.currentThread().interrupt(); }
        }
        bufferLock.lock();
        try { backBuffer = null; frontBuffer = null; } finally { bufferLock.unlock(); }
    }

    // ---- Public marker drawing ----

    public void drawMarker() {
        double lat = locationLat;
        double lon = locationLon;
        if (Double.isNaN(lat) || Double.isNaN(lon)) return;
        int w = (int) canvas.getWidth();
        int h = (int) canvas.getHeight();
        if (w <= 0 || h <= 0) return;
        double[] pt = ProjectionUtils.geoToScreen(lat, lon, w, h, currentMag, currentLat, currentLon, dpi);
        var gc = canvas.getGraphicsContext2D();
        drawLocationMarkerShape(gc, pt[0], pt[1]);
    }

    private void drawLocationMarkerShape(GraphicsContext gc, double x, double y) {
        double lat = locationLat;
        double lon = locationLon;
        if (Double.isNaN(lat) || Double.isNaN(lon)) return;
        Color fillColor;
        boolean hasAccuracy = !Double.isNaN(locationAccuracy) && locationAccuracy >= 0.0;
        if (hasAccuracy) {
            fillColor = locationAccuracy <= 10.0 ? Color.rgb(0, 204, 51, 0.65)
                     : locationAccuracy <= 50.0 ? Color.rgb(255, 204, 0, 0.65)
                     : Color.rgb(255, 51, 51, 0.65);
        } else {
            fillColor = Color.rgb(0, 102, 255, 0.65);
        }
        double markerRadius = 8.0;
        double arrowLength = markerRadius * 2.2;
        gc.save();
        gc.setGlobalAlpha(1.0);
        gc.setFill(fillColor);
        gc.fillOval(x - markerRadius, y - markerRadius, markerRadius * 2.0, markerRadius * 2.0);
        gc.setStroke(Color.rgb(255, 255, 255, 0.85));
        gc.setLineWidth(2.0);
        gc.strokeOval(x - markerRadius, y - markerRadius, markerRadius * 2.0, markerRadius * 2.0);
        if (!Double.isNaN(locationBearing)) {
            gc.setStroke(Color.rgb(255, 255, 255, 0.9));
            gc.setLineWidth(2.5);
            gc.translate(x, y);
            gc.rotate(locationBearing + Math.toDegrees(currentAngle));
            gc.strokeLine(0, 0, 0, -arrowLength);
            double head = 5.5;
            gc.setFill(Color.rgb(255, 255, 255, 0.9));
            gc.fillPolygon(new double[]{0, -head, head}, new double[]{-arrowLength, -arrowLength + head, -arrowLength + head}, 3);
            gc.rotate(-(locationBearing - Math.toDegrees(currentAngle)));
            gc.translate(-x, -y);
        }
        gc.restore();
    }

    // ---- Internal: Debounce ----

    private void submitDebounced(double lat, double lon, int mag, double angle,
                                 double oldLat, double oldLon, int oldMag, double oldAngle,
                                 boolean forceFullRender) {
        long debounceMs = (mag != oldMag || angle != oldAngle || forceFullRender)
                ? ZOOM_ROTATE_DEBOUNCE_MS : PAN_DEBOUNCE_MS;
        // Try sub-region blit for pan-only changes. Skip when forceFullRender is true
        // (overlay data changed — sub-region blit would succeed since view hasn't moved,
        //  preventing the new overlay from being rendered).
        if (!forceFullRender) {
            bufferLock.lock();
            try {
                if (frontBuffer != null) {
                    boolean ok = trySubRegionBlit(lat, lon, mag, angle);
                    if (ok) {
                        pendingRender.set(null);
                        return;
                    }
                }
            } finally { bufferLock.unlock(); }
        }
        // Set pendingRender BEFORE debounceState to avoid race:
        // debounce thread reads state first, then pendingRender.
        // Volatile happens-before guarantees: write to pendingRender is visible
        // when debounce thread reads debounceState (set after pendingRender).
        pendingRender.set(new PendingRender(lat, lon, mag, angle));
        debounceState.set(debounceMs == PAN_DEBOUNCE_MS
                ? DebounceState.PAN_WAITING : DebounceState.ZOOM_ROTATE_WAITING);
        synchronized (debounceLock) { debounceLock.notify(); }
    }

    private final AtomicReference<PendingRender> pendingRender = new AtomicReference<>(null);
    private record PendingRender(double lat, double lon, int mag, double angle) {}

    private void startDebounceLoop() {
        debounceThread = new Thread(() -> {
            while (running) {
                try {
                    DebounceState state = debounceState.get();
                    if (state == DebounceState.IDLE) {
                        synchronized (debounceLock) {
                            debounceLock.wait(); // No busy-wait when idle
                        }
                        continue;
                    }
                    long timeout = (state == DebounceState.PAN_WAITING) ? PAN_DEBOUNCE_MS : ZOOM_ROTATE_DEBOUNCE_MS;
                    Thread.sleep(timeout);
                    if (!running) break;
                    PendingRender req = pendingRender.getAndSet(null);
                    if (req != null) {
                        debounceState.set(DebounceState.IDLE);
                        enqueueRenderJob(req.lat, req.lon, req.mag, req.angle);
                    }
                } catch (InterruptedException e) { Thread.currentThread().interrupt(); break; }
            }
        }, "map-debounce");
        debounceThread.setDaemon(true);
        debounceThread.start();
    }

    // ---- Internal: Render job queue ----

    private void enqueueRenderJob(double lat, double lon, int mag, double angle) {
        int w = (int) canvas.getWidth();
        int h = (int) canvas.getHeight();
        if (w <= 0 || h <= 0) return;
        int renderW = (int) (w * canvasOverrun);
        int renderH = (int) (h * canvasOverrun);
        long jobEpoch = epoch.get();
        pendingJob.set(new RenderJob(lat, lon, mag, angle,
            routeLats, routeLons, favoriteLats, favoriteLons,
            searchSelectedLat, searchSelectedLon, trackLats, trackLons,
            renderW, renderH, jobEpoch));
        synchronized (renderLock) { renderLock.notify(); }
    }

    private void startRenderThread() {
        renderThread = new Thread(() -> {
            while (renderThreadRunning.get()) {
                RenderJob job = pendingJob.getAndSet(null);
                if (job == null) {
                    synchronized (renderLock) {
                        try { renderLock.wait(); } catch (InterruptedException e) { Thread.currentThread().interrupt(); break; }
                    }
                    continue;
                }
                if (!renderThreadRunning.get()) break;
                executeRender(job);
            }
        }, "map-render");
        renderThread.setDaemon(true);
        renderThread.start();
    }

    private void executeRender(RenderJob job) {
        int[] pixels = null;
        for (int attempt = 0; attempt < 2; attempt++) {
            try {
                if (job.hasOverlays()) {
                    pixels = client.renderWithRouteAndPois(
                        job.width(), job.height(), job.lat(), job.lon(), job.angle(), job.mag(),
                        job.routeLats(), job.routeLons(), job.favoriteLats(), job.favoriteLons(),
                        job.searchSelectedLat(), job.searchSelectedLon(), job.trackLats(), job.trackLons());
                } else {
                    pixels = client.render(job.width(), job.height(), job.lat(), job.lon(), job.angle(), job.mag());
                }
                break; // Success
            } catch (Exception e) {
                if (attempt == 0) {
                    Log.error("[MapRenderer] JNI render failed (retrying): " + e.getMessage());
                    try { Thread.sleep(100); } catch (InterruptedException ie) { Thread.currentThread().interrupt(); return; }
                } else {
                    Log.error("[MapRenderer] JNI render failed: " + e.getMessage());
                    return;
                }
            }
        }
        if (pixels == null) return;
        if (job.epoch() != epoch.get()) return;
        boolean viewMoved = Math.abs(currentLat - job.lat()) > 1e-8 || Math.abs(currentLon - job.lon()) > 1e-8;
        long completionEpoch = epoch.get();
        bufferLock.lock();
        try {
            if (job.epoch() != epoch.get()) return;
            if (backBuffer == null || backBuffer.getWidth() != job.width() || backBuffer.getHeight() != job.height()) {
                backBuffer = new WritableImage(job.width(), job.height());
            }
            backBuffer.getPixelWriter().setPixels(0, 0, job.width(), job.height(), ARGB_FORMAT, pixels, 0, job.width());
            tileCache.storeTiles(pixels, job.width(), job.height(), job.mag(), job.epoch());
            WritableImage tmp = frontBuffer;
            frontBuffer = backBuffer;
            backBuffer = tmp;
            frontBufferEpoch = job.epoch();
            frontBufferLat = job.lat();
            frontBufferLon = job.lon();
            frontBufferMag = job.mag();
            frontBufferAngle = job.angle();
        } finally { bufferLock.unlock(); }
        Platform.runLater(() -> {
            if (!running) return;
            // Skip blit if zoom/angle changed after render completed —
            // trySubRegionBlit() already displayed a scaled placeholder at the new level.
            // Blitting the stale buffer would cause a visible jump back.
            if (currentMag != job.mag() || currentAngle != job.angle()) return;
            if (completionEpoch != epoch.get()) return;
            bufferLock.lock();
            try {
                if (frontBuffer == null) return;
                if (viewMoved) {
                    int sw = (int) canvas.getWidth();
                    int sh = (int) canvas.getHeight();
                    int fbw = (int) frontBuffer.getWidth();
                    int fbh = (int) frontBuffer.getHeight();
                    double[] oldC = ProjectionUtils.geoToScreen(job.lat(), job.lon(), sw, sh, job.mag(), job.lat(), job.lon(), dpi);
                    double[] newC = ProjectionUtils.geoToScreen(currentLat, currentLon, sw, sh, job.mag(), job.lat(), job.lon(), dpi);
                    double dx = newC[0] - oldC[0];
                    double dy = newC[1] - oldC[1];
                    double srcX = (fbw / 2.0) - (sw / 2.0) + dx;
                    double srcY = (fbh / 2.0) - (sh / 2.0) + dy;
                    var gc = canvas.getGraphicsContext2D();
                    gc.setFill(Color.rgb(240, 240, 240));
                    gc.fillRect(0, 0, sw, sh);
                    double isx = Math.max(0, srcX);
                    double isy = Math.max(0, srcY);
                    double iw = Math.min(fbw - isx, sw);
                    double ih = Math.min(fbh - isy, sh);
                    if (iw > 0 && ih > 0) {
                        gc.drawImage(frontBuffer, isx, isy, iw, ih, isx - srcX, isy - srcY, iw, ih);
                    }
                    if (!Double.isNaN(locationLat) && !Double.isNaN(locationLon)) {
                        double[] pt = ProjectionUtils.geoToScreen(locationLat, locationLon, sw, sh, currentMag, currentLat, currentLon, dpi);
                        drawLocationMarkerShape(gc, pt[0], pt[1]);
                    }
                } else {
                    blitFrontBuffer(job.lat(), job.lon(), job.mag(), job.angle());
                }
            } finally { bufferLock.unlock(); }
            for (ViewChangeListener l : listeners) l.onViewChanged(job.lat(), job.lon(), job.mag(), job.angle());
        });
    }

    // ---- Internal: Sub-region blit (canvas overrun) ----

    private boolean trySubRegionBlit(double newLat, double newLon, int newMag, double newAngle) {
        if (frontBuffer == null) return false;
        int fbW = (int) frontBuffer.getWidth();
        int fbH = (int) frontBuffer.getHeight();
        int screenW = (int) canvas.getWidth();
        int screenH = (int) canvas.getHeight();

        // Handle zoom: scale current buffer to new mag as placeholder
        if (newMag != frontBufferMag) {
            double zoomScale = Math.pow(2, newMag - frontBufferMag);
            var gc = canvas.getGraphicsContext2D();
            gc.setFill(Color.rgb(240, 240, 240));
            gc.fillRect(0, 0, screenW, screenH);
            // Find where the new center is in the front buffer (accounts for pan offset)
            double[] newCenterInBuf = ProjectionUtils.geoToScreen(
                newLat, newLon, fbW, fbH, frontBufferMag, frontBufferLat, frontBufferLon, dpi);
            if (zoomScale >= 1) {
                // Zoom in: take region around new center, scale up to fill screen.
                // Source dimensions are based on SCREEN size, not buffer size.
                // Buffer is 2.5x screen; using fbW/zoomScale would give a source
                // 2.5x too large, showing too much area (lower zoom level look).
                double srcW = screenW / zoomScale;
                double srcH = screenH / zoomScale;
                double srcX = newCenterInBuf[0] - srcW / 2.0;
                double srcY = newCenterInBuf[1] - srcH / 2.0;
                // Clamp to buffer bounds
                srcX = Math.max(0, Math.min(srcX, fbW - srcW));
                srcY = Math.max(0, Math.min(srcY, fbH - srcH));
                gc.drawImage(frontBuffer, srcX, srcY, srcW, srcH, 0, 0, screenW, screenH);
            } else {
                // Zoom out: scale buffer down, position so new center aligns with screen center
                double dstW = fbW * zoomScale;
                double dstH = fbH * zoomScale;
                double dstX = screenW / 2.0 - newCenterInBuf[0] * zoomScale;
                double dstY = screenH / 2.0 - newCenterInBuf[1] * zoomScale;
                gc.drawImage(frontBuffer, 0, 0, fbW, fbH, dstX, dstY, dstW, dstH);
            }
            currentLat = newLat;
            currentLon = newLon;
            currentMag = newMag;
            currentAngle = newAngle;
            epoch.incrementAndGet(); // discard stale zoom renders in queue
            if (!Double.isNaN(locationLat) && !Double.isNaN(locationLon)) {
                double[] pt = ProjectionUtils.geoToScreen(locationLat, locationLon, screenW, screenH, currentMag, currentLat, currentLon, dpi);
                drawLocationMarkerShape(gc, pt[0], pt[1]);
            }
            // Notify listeners immediately so zoom level indicator updates
            // even before the full render completes.
            for (ViewChangeListener l : listeners) {
                l.onViewChanged(currentLat, currentLon, currentMag, currentAngle);
            }
            return false; // trigger full render for high-quality version
        }

        if (newAngle != frontBufferAngle) return false;
        // Use Mercator projection for offset (matches geoToScreen used for marker)
        double[] oldCenter = ProjectionUtils.geoToScreen(frontBufferLat, frontBufferLon, screenW, screenH, frontBufferMag, frontBufferLat, frontBufferLon, dpi);
        double[] newCenter = ProjectionUtils.geoToScreen(newLat, newLon, screenW, screenH, frontBufferMag, frontBufferLat, frontBufferLon, dpi);
        double dx = newCenter[0] - oldCenter[0];
        double dy = newCenter[1] - oldCenter[1];
        double viewLeft = (fbW / 2.0) - (screenW / 2.0) + dx;
        double viewTop = (fbH / 2.0) - (screenH / 2.0) + dy;
        double viewRight = viewLeft + screenW;
        double viewBottom = viewTop + screenH;

        double srcX = (fbW / 2.0) - (screenW / 2.0) + dx;
        double srcY = (fbH / 2.0) - (screenH / 2.0) + dy;

        var gc = canvas.getGraphicsContext2D();
        gc.setFill(Color.rgb(240, 240, 240));
        gc.fillRect(0, 0, screenW, screenH);
        double isx = Math.max(0, srcX);
        double isy = Math.max(0, srcY);
        double iw = Math.min(fbW - isx, screenW);
        double ih = Math.min(fbH - isy, screenH);
        if (iw > 0 && ih > 0) {
            gc.drawImage(frontBuffer, isx, isy, iw, ih, isx - srcX, isy - srcY, iw, ih);
        }

        currentLat = newLat;
        currentLon = newLon;
        if (!Double.isNaN(locationLat) && !Double.isNaN(locationLon)) {
            double[] pt = ProjectionUtils.geoToScreen(locationLat, locationLon, screenW, screenH, currentMag, currentLat, currentLon, dpi);
            drawLocationMarkerShape(gc, pt[0], pt[1]);
        }

        return viewLeft >= 0 && viewTop >= 0 && viewRight <= fbW && viewBottom <= fbH;
    }

    private void blitFrontBuffer(double centerLat, double centerLon, int mag, double angle) {
        if (frontBuffer == null) {
            var gc = canvas.getGraphicsContext2D();
            gc.setFill(Color.rgb(240, 240, 240));
            gc.fillRect(0, 0, canvas.getWidth(), canvas.getHeight());
            return;
        }
        int screenW = (int) canvas.getWidth();
        int screenH = (int) canvas.getHeight();
        int fbW = (int) frontBuffer.getWidth();
        int fbH = (int) frontBuffer.getHeight();
        var gc = canvas.getGraphicsContext2D();
        if (fbW == screenW && fbH == screenH) {
            gc.drawImage(frontBuffer, 0, 0);
        } else {
            gc.drawImage(frontBuffer, (fbW - screenW) / 2.0, (fbH - screenH) / 2.0, screenW, screenH, 0, 0, screenW, screenH);
        }
        if (!Double.isNaN(locationLat) && !Double.isNaN(locationLon)) {
            double[] pt = ProjectionUtils.geoToScreen(locationLat, locationLon, screenW, screenH, mag, centerLat, centerLon, dpi);
            drawLocationMarkerShape(gc, pt[0], pt[1]);
        }
    }
}
