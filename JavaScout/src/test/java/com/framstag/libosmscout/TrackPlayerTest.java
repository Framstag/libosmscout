package com.framstag.libosmscout;

import com.framstag.libosmscout.client.NavigationController;
import com.framstag.libosmscout.client.TrackPoint;

import javafx.application.Platform;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for {@link TrackPlayer}.
 */
public class TrackPlayerTest {

    @BeforeAll
    public static void initJavaFx() {
        try {
            Platform.startup(() -> {});
        } catch (IllegalStateException e) {
            // already initialized
        }
    }

    @Test
    public void testSpeedMultiplierDefaultsToOne() {
        TrackPlayer player = createPlayer(new TrackPoint[0]);
        assertEquals(1.0, player.getSpeedMultiplier(), 0.0001);
    }

    @Test
    public void testSetSpeedMultiplier() {
        TrackPlayer player = createPlayer(new TrackPoint[0]);
        player.setSpeedMultiplier(2.5);
        assertEquals(2.5, player.getSpeedMultiplier(), 0.0001);
    }

    @Test
    public void testNegativeSpeedMultiplierRejected() {
        TrackPlayer player = createPlayer(new TrackPoint[0]);
        assertThrows(IllegalArgumentException.class, () -> player.setSpeedMultiplier(0));
        assertThrows(IllegalArgumentException.class, () -> player.setSpeedMultiplier(-1));
    }

    @Test
    public void testEmptyTrackDoesNotPlay() throws InterruptedException {
        List<String> statuses = new ArrayList<>();
        TrackPlayer player = new TrackPlayer(new TrackPoint[0], null, statuses::add);

        player.play();

        assertFalse(player.isPlaying());
        assertTrue(statuses.contains("stopped"));
    }

    @Test
    public void testPlayEmitsPoints() throws InterruptedException {
        TrackPoint[] points = {
            new TrackPoint(51.0, 7.0, "2024-01-01T10:00:00Z"),
            new TrackPoint(51.001, 7.001, "2024-01-01T10:00:01Z"),
            new TrackPoint(51.002, 7.002, "2024-01-01T10:00:02Z")
        };

        CountDownLatch latch = new CountDownLatch(points.length);
        List<TrackPoint> emitted = new ArrayList<>();
        NavigationController controller = new FakeNavigationController(latch, emitted);

        TrackPlayer player = new TrackPlayer(points, controller, s -> {});
        player.play();

        assertTrue(latch.await(5, TimeUnit.SECONDS), "all points should be emitted");
        assertEquals(points.length, emitted.size());
        for (int i = 0; i < points.length; i++) {
            assertEquals(points[i].lat, emitted.get(i).lat, 0.00001);
            assertEquals(points[i].lon, emitted.get(i).lon, 0.00001);
        }
    }

    private TrackPlayer createPlayer(TrackPoint[] points) {
        return new TrackPlayer(points, null, s -> {});
    }

    private static class FakeNavigationController extends NavigationController {
        private final CountDownLatch latch;
        private final List<TrackPoint> emitted;

        FakeNavigationController(CountDownLatch latch, List<TrackPoint> emitted) {
            this.latch = latch;
            this.emitted = emitted;
        }

        @Override
        public void processLocation(double lat, double lon, double speed, double accuracy, long timestamp) {
            emitted.add(new TrackPoint(lat, lon));
            latch.countDown();
        }

        @Override
        public void stop() {
        }
    }
}
