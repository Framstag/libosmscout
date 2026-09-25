package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * JNI integration tests for the distances the navigation bridge reports for route
 * instructions (spec turn-by-turn-instructions: the next instruction's distance follows
 * the route progress, and the arrival instruction carries the distance of its own node).
 * <p>
 * Needs a routable map database directory: {@code -Dnav.test.db.dir=<dir>} or the
 * {@code JAVASCOUT_MAP_DIR} environment variable, the same convention as
 * {@link OSMScoutClientNavigationLiveTest}. Skipped, not failed, without one, and also
 * skipped when the native library is not available.
 * <p>
 * The arrival instruction's distance is asserted on the full instruction list, which is
 * generated from the route description and therefore always contains the arrival: whether
 * the arrival is also the *next* instruction depends on how the manoeuvres of the map data
 * are spaced, so that case is not asserted here.
 */
public class OSMScoutClientInstructionDistanceTest {

    private static final String DB_DIR_PROPERTY = "nav.test.db.dir";

    /** A route near Dortmund city centre, as in the live navigation test. */
    private static final double START_LAT = 51.514227;
    private static final double START_LON = 7.465279;
    private static final double DEST_LAT = 51.515;
    private static final double DEST_LON = 7.466;

    /** Skip the test when the native library cannot be loaded. */
    private static void assumeNativeLibrary() {
        try {
            new OSMScoutClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
        }
    }

    /** Operator-provided map database directory, or null when unset. */
    private static Path providedMapDir() {
        String dir = System.getProperty(DB_DIR_PROPERTY);
        if (dir != null && !dir.isEmpty()) {
            return Paths.get(dir);
        }
        String env = System.getenv("JAVASCOUT_MAP_DIR");
        if (env != null && !env.isEmpty()) {
            return Paths.get(env);
        }
        return null;
    }

    private static OSMScoutClient buildClient(Path mapDir) {
        return new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir.toString())
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withCustomPoiType("_route_start")
            .withCustomPoiType("_route_end")
            .build();
    }

    private static RouteEntry calculateRoute(OSMScoutClient client) throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        AtomicReference<RouteEntry> routeHolder = new AtomicReference<>();

        client.calculateRouteAsync(START_LAT, START_LON, DEST_LAT, DEST_LON, new RouteCallback() {
            @Override public void onProgress(int percent) {
            }

            @Override public void onSuccess(RouteEntry route) {
                routeHolder.set(route);
                latch.countDown();
            }

            @Override public void onError(String message) {
                latch.countDown();
            }

            @Override public void onCancel() {
                latch.countDown();
            }
        });

        assertTrue(latch.await(60, TimeUnit.SECONDS), "route calculation should complete");
        RouteEntry route = routeHolder.get();
        assertNotNull(route, "route should be calculated");
        assertTrue(route.routeHandle != 0, "route handle should be set");

        return route;
    }

    @Test
    @Timeout(180)
    public void testInstructionDistancesFollowTheRoute() throws InterruptedException {
        assumeNativeLibrary();

        Path mapDir = providedMapDir();
        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient client = buildClient(mapDir);
        Assumptions.assumeTrue(client != null, "client already initialised");

        try {
            assertTrue(client.openDatabase(mapDir.toString()), "database should open");

            RouteEntry route = calculateRoute(client);
            assertTrue(route.distance > 0.0, "route should have a length");
            assertTrue(route.latitudes.length > 2, "route geometry should have several points");

            AtomicReference<RouteInstruction[]> listHolder = new AtomicReference<>();
            AtomicReference<RouteInstruction> nextHolder = new AtomicReference<>();
            CountDownLatch listLatch = new CountDownLatch(1);
            CountDownLatch nextLatch = new CountDownLatch(1);

            NavigationListener listener = new NavigationListener() {
                @Override
                public void onRouteInstructions(RouteInstruction[] instructions) {
                    listHolder.set(instructions);
                    listLatch.countDown();
                }

                @Override
                public void onNextRouteInstruction(RouteInstruction instruction) {
                    nextHolder.set(instruction);
                    nextLatch.countDown();
                }

                @Override
                public void onError(String message) {
                    System.out.println("[InstructionDistanceTest] navigation error: " + message);
                }
            };

            NavigationController controller = client.startNavigation(route.routeHandle, listener);
            assertNotNull(controller, "navigation controller should be created");

            long baseTime = System.currentTimeMillis();

            // First fix at the route start: the full instruction list is published on the
            // route change, and the next instruction with it.
            controller.processLocation(route.latitudes[0], route.longitudes[0],
                -1.0, -1.0, baseTime);

            assertTrue(listLatch.await(30, TimeUnit.SECONDS),
                "the route instruction list should be delivered on the route change");
            assertTrue(nextLatch.await(30, TimeUnit.SECONDS),
                "the next instruction should be delivered on the route change");

            RouteInstruction[] list = listHolder.get();
            assertNotNull(list, "instruction list should not be null");
            assertTrue(list.length > 0, "instruction list should not be empty");

            System.out.println("[InstructionDistanceTest] route.distance=" + route.distance
                + " geometryLength=" + polylineLength(route)
                + " instructions=" + list.length);
            for (RouteInstruction instruction : list) {
                System.out.println("[InstructionDistanceTest]   " + instruction.turnType
                    + " distanceTo=" + instruction.distanceTo
                    + " short=" + instruction.shortDescription);
            }

            // The arrival instruction is the last entry of the list and carries the
            // distance of its own node from the route start, which is the furthest node:
            // it must be greater than zero and at least as large as every other
            // instruction's distance. Reporting 0 here made it indistinguishable from the
            // route start.
            RouteInstruction arrival = list[list.length - 1];
            assertEquals(TurnType.TARGET_REACHED, arrival.turnType,
                "the last instruction of the list should be the arrival");
            assertTrue(arrival.distanceTo > 0.0,
                "the arrival instruction should carry a distance, not 0 (was " +
                    arrival.distanceTo + ")");
            for (RouteInstruction instruction : list) {
                assertTrue(arrival.distanceTo >= instruction.distanceTo,
                    "the arrival instruction should carry the largest distance of the list"
                        + " (arrival " + arrival.distanceTo + ", " + instruction.turnType
                        + " " + instruction.distanceTo + ")");
            }

            RouteInstruction firstNext = nextHolder.get();
            assertNotNull(firstNext, "the next instruction should be delivered");
            assertTrue(firstNext.distanceTo >= 0.0,
                "the next instruction's distance should not be negative");

            // Feed a fix between two route points, so the position agent reports progress
            // *inside* a segment instead of exactly on a node (where progress would be 0 and
            // the straight-line fallback would be used): the remaining distance must shrink
            // and must never become negative.
            int laterIndex = Math.max(1, route.latitudes.length / 2);
            double laterLat = (route.latitudes[laterIndex - 1] + route.latitudes[laterIndex]) / 2.0;
            double laterLon = (route.longitudes[laterIndex - 1] + route.longitudes[laterIndex]) / 2.0;
            nextHolder.set(null);

            controller.processLocation(laterLat, laterLon, -1.0, -1.0, baseTime + 2000);

            RouteInstruction laterNext = waitForNextInstruction(nextHolder, 30);
            assertNotNull(laterNext, "the next instruction should be delivered for the later fix");
            assertTrue(laterNext.distanceTo >= 0.0,
                "the next instruction's distance should not be negative");
            assertTrue(laterNext.distanceTo <= firstNext.distanceTo,
                "the remaining distance should shrink as the position advances along the route"
                    + " (was " + firstNext.distanceTo + ", later " + laterNext.distanceTo + ")");

            controller.stop();
        } finally {
            client.close();
        }
    }

    /** Polls the holder for a next instruction, so the test does not depend on callback timing. */
    private static RouteInstruction waitForNextInstruction(AtomicReference<RouteInstruction> holder,
                                                          long timeoutSeconds)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + timeoutSeconds * 1000;

        while (System.currentTimeMillis() < deadline) {
            RouteInstruction instruction = holder.get();
            if (instruction != null) {
                return instruction;
            }
            Thread.sleep(50);
        }

        return holder.get();
    }

    /** Length of the route polyline in meters, for orientation in the test output. */
    private static double polylineLength(RouteEntry route) {
        double length = 0.0;

        for (int i = 1; i < route.latitudes.length; i++) {
            length += haversineMeters(route.latitudes[i - 1], route.longitudes[i - 1],
                route.latitudes[i], route.longitudes[i]);
        }

        return length;
    }

    private static double haversineMeters(double lat1, double lon1, double lat2, double lon2) {
        final double earthRadius = 6371000.0;
        double dLat = Math.toRadians(lat2 - lat1);
        double dLon = Math.toRadians(lon2 - lon1);
        double a = Math.sin(dLat / 2) * Math.sin(dLat / 2)
            + Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2))
            * Math.sin(dLon / 2) * Math.sin(dLon / 2);

        return 2 * earthRadius * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
    }
}
