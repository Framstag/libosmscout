package com.framstag.libosmscout;

import com.framstag.libosmscout.client.NavigationController;
import com.framstag.libosmscout.client.TrackPoint;

import javafx.animation.Animation;
import javafx.animation.KeyFrame;
import javafx.animation.Timeline;
import javafx.beans.property.DoubleProperty;
import javafx.beans.property.SimpleDoubleProperty;
import javafx.util.Duration;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * Replays an imported GPX track as simulated GPS fixes for the navigation engine.
 */
public class TrackPlayer {

    /** Default playback speed multiplier. */
    public static final double DEFAULT_SPEED_MULTIPLIER = 1.0;

    private TrackPoint[] points;
    private NavigationController controller;
    private final Consumer<String> onStatusChange;
    private final DoubleProperty speedMultiplier = new SimpleDoubleProperty(DEFAULT_SPEED_MULTIPLIER);

    private Timeline timeline;
    private int emittedIndex = -1;

    /**
     * Create a track player.
     *
     * @param points        track points in chronological order
     * @param controller    navigation controller to feed with GPS updates
     * @param onStatusChange optional status callback ("playing", "paused", "stopped")
     */
    public TrackPlayer(TrackPoint[] points,
                       NavigationController controller,
                       Consumer<String> onStatusChange) {
        this.points = (points != null) ? points : new TrackPoint[0];
        this.controller = controller;
        this.onStatusChange = onStatusChange != null ? onStatusChange : s -> {};
    }

    /** Speed multiplier property (1.0 = recorded timing). */
    public DoubleProperty speedMultiplierProperty() {
        return speedMultiplier;
    }

    public double getSpeedMultiplier() {
        return speedMultiplier.get();
    }

    public void setSpeedMultiplier(double value) {
        if (value <= 0) {
            throw new IllegalArgumentException("speed multiplier must be positive");
        }
        boolean wasRunning = timeline != null && timeline.getStatus() == Animation.Status.RUNNING;
        speedMultiplier.set(value);
        if (wasRunning) {
            // Rebuild from the next unplayed point so the marker keeps moving;
            // rebuilding from the beginning would leave it stuck until the
            // already-played duration elapses.
            buildTimeline(emittedIndex + 1);
            timeline.play();
        }
    }

    /** Start or resume playback. */
    public void play() {
        if (points.length == 0 || controller == null) {
            onStatusChange.accept("stopped");
            return;
        }
        if (timeline == null || timeline.getStatus() == Animation.Status.STOPPED) {
            // Full stop: start from the beginning.
            emittedIndex = -1;
        }
        // Always rebuild so a speed change while paused is honoured and so we continue
        // from the next unplayed point rather than re-emitting already-played fixes.
        buildTimeline(emittedIndex + 1);
        if (timeline == null) {
            return;
        }
        timeline.play();
        onStatusChange.accept("playing");
    }

    /** Pause playback. */
    public void pause() {
        if (timeline != null) {
            timeline.pause();
            onStatusChange.accept("paused");
        }
    }

    /** Stop playback and reset to the beginning. */
    public void stop() {
        if (timeline != null) {
            timeline.stop();
        }
        emittedIndex = -1;
        onStatusChange.accept("stopped");
    }

    /** Whether playback is currently running. */
    public boolean isPlaying() {
        return timeline != null && timeline.getStatus() == Animation.Status.RUNNING;
    }

    /**
     * Swap the navigation controller without recreating the player.
     * Used after reroute to point the existing track at the new controller.
     */
    public void setController(NavigationController controller) {
        this.controller = controller;
    }

    private void buildTimeline(int startIndex) {
        if (startIndex < 0) {
            startIndex = 0;
        }
        if (startIndex >= points.length) {
            timeline = new Timeline();
            timeline.setOnFinished(e -> onStatusChange.accept("stopped"));
            return;
        }

        if (timeline != null) {
            timeline.stop();
        }

        List<KeyFrame> keyFrames = new ArrayList<>();
        long baseTime = System.currentTimeMillis();
        double cumulativeMs = 0.0;

        // Emit the resume/start point immediately
        final double startSpeed = (startIndex > 0)
            ? computeSpeedMps(startIndex - 1, startIndex, computeSegmentDurationMs(startIndex - 1, startIndex))
            : 0.0;
        final int firstIndex = startIndex;
        keyFrames.add(new KeyFrame(Duration.ZERO, e -> emitPoint(firstIndex, baseTime, startSpeed)));

        for (int i = startIndex + 1; i < points.length; i++) {
            double segmentMs = computeSegmentDurationMs(i - 1, i);
            cumulativeMs += segmentMs / speedMultiplier.get();
            final int index = i;
            final double speedMps = computeSpeedMps(i - 1, i, segmentMs);
            final double cueMs = cumulativeMs;
            keyFrames.add(new KeyFrame(Duration.millis(cueMs),
                e -> emitPoint(index, baseTime + (long) cueMs, speedMps)));
        }

        timeline = new Timeline(keyFrames.toArray(new KeyFrame[0]));
        timeline.setOnFinished(e -> onStatusChange.accept("stopped"));
    }

    private double computeSegmentDurationMs(int from, int to) {
        TrackPoint a = points[from];
        TrackPoint b = points[to];
        if (a.timestamp != null && b.timestamp != null) {
            try {
                long ta = parseTimestampMs(a.timestamp);
                long tb = parseTimestampMs(b.timestamp);
                double dt = tb - ta;
                if (dt > 0) {
                    return dt;
                }
            } catch (Exception ignored) {
            }
        }
        return 1000.0; // default 1 second between fixes
    }

    private double computeSpeedMps(int from, int to, double durationMs) {
        if (durationMs <= 0) {
            return -1.0;
        }
        TrackPoint a = points[from];
        TrackPoint b = points[to];
        double d = haversineM(a.lat, a.lon, b.lat, b.lon);
        return d / (durationMs / 1000.0);
    }

    private void emitPoint(int index, long timestampMs, double speedMps) {
        try {
            if (index <= emittedIndex || controller == null) {
                return;
            }
            emittedIndex = index;
            TrackPoint p = points[index];
            controller.processLocation(p.lat, p.lon, speedMps, -1.0, timestampMs);
        } catch (Exception ex) {
            Log.error("[TrackPlayer] emitPoint failed at index=" + index + ": " + ex);
        }
    }

    private static long parseTimestampMs(String iso) {
        return java.time.Instant.parse(iso).toEpochMilli();
    }

    private static double haversineM(double lat1, double lon1, double lat2, double lon2) {
        final double R = 6371000.0;
        double dLat = Math.toRadians(lat2 - lat1);
        double dLon = Math.toRadians(lon2 - lon1);
        double a = Math.sin(dLat / 2) * Math.sin(dLat / 2)
            + Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2))
            * Math.sin(dLon / 2) * Math.sin(dLon / 2);
        double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
        return R * c;
    }
}
