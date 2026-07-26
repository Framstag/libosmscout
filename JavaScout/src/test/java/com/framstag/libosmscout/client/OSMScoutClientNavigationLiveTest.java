package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration test that opens a real map database, calculates a route,
 * starts live navigation, and feeds a simulated GPS fix.
 * <p>
 * Skipped when no native library or no map database is available.
 */
public class OSMScoutClientNavigationLiveTest {

    private static OSMScoutClient client;

    @BeforeAll
    public static void setUp() {
        try {
            client = new OSMScoutClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
        }
    }

    private static Path defaultMapDir() {
        String env = System.getenv("JAVASCOUT_MAP_DIR");
        if (env != null && !env.isEmpty()) {
            return Paths.get(env);
        }
        return Paths.get("../maps/nordrhein-westfalen").toAbsolutePath().normalize();
    }

    @Test
    public void testStartNavigationFromCalculatedRoute() throws InterruptedException {
        Path mapDir = defaultMapDir();
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir.toString())
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withCustomPoiType("_route_start")
            .withCustomPoiType("_route_end");

        OSMScoutClient localClient = builder.build();
        Assumptions.assumeTrue(localClient != null, "client already initialised");

        assertTrue(localClient.openDatabase(mapDir.toString()),
            "database should open");

        CountDownLatch routeLatch = new CountDownLatch(1);
        RouteEntry[] routeHolder = new RouteEntry[1];
        RouteCallback routeCallback = new RouteCallback() {
            @Override public void onProgress(int percent) {}
            @Override public void onSuccess(RouteEntry route) {
                routeHolder[0] = route;
                routeLatch.countDown();
            }
            @Override public void onError(String message) {
                routeLatch.countDown();
            }
            @Override public void onCancel() {
                routeLatch.countDown();
            }
        };

        // Route near Dortmund city center
        localClient.calculateRouteAsync(
            51.514227, 7.465279,
            51.515, 7.466,
            routeCallback);

        assertTrue(routeLatch.await(30, TimeUnit.SECONDS),
            "route calculation should complete");
        RouteEntry route = routeHolder[0];
        assertNotNull(route, "route should be calculated");
        assertTrue(route.routeHandle != 0, "route handle should be set");

        List<NavigationPosition> positions = new ArrayList<>();
        CountDownLatch positionLatch = new CountDownLatch(2);
        CountDownLatch instructionLatch = new CountDownLatch(1);
        RouteInstruction[] lastInstructions = new RouteInstruction[1];
        NavigationListener listener = new NavigationListener() {
            @Override
            public void onPositionEstimate(NavigationPosition position) {
                System.out.println("[NavigationListener] position: " + position);
                if (position != null) {
                    positions.add(position);
                    positionLatch.countDown();
                }
            }

            @Override
            public void onRerouteRequest(double lat, double lon, double bearing, double destLat, double destLon) {
                System.out.println("[NavigationListener] reroute request at " + lat + "," + lon);
            }

            @Override
            public void onError(String message) {
                System.out.println("[NavigationListener] error: " + message);
            }

            @Override
            public void onRouteInstructions(RouteInstruction[] instructions) {
                System.out.println("[NavigationListener] received " + instructions.length + " route instructions");
                lastInstructions[0] = instructions.length > 0 ? instructions[0] : null;
            }

            @Override
            public void onNextRouteInstruction(RouteInstruction instruction) {
                System.out.println("[NavigationListener] next instruction: " + instruction);
                instructionLatch.countDown();
            }
        };

        NavigationController controller = localClient.startNavigation(route.routeHandle, listener);
        assertNotNull(controller, "navigation controller should be created");

        // Feed a GPS fix exactly at the snapped route start
        double startLat = route.latitudes[0];
        double startLon = route.longitudes[0];
        long baseTime = System.currentTimeMillis();
        System.out.println("[Test] feeding GPS at snapped start " + startLat + "," + startLon);
        controller.processLocation(startLat, startLon, -1.0, -1.0, baseTime);

        // Feed a second fix a little further along the route, 2 seconds later
        int nextIndex = Math.min(5, route.latitudes.length - 1);
        double nextLat = route.latitudes[nextIndex];
        double nextLon = route.longitudes[nextIndex];
        System.out.println("[Test] feeding GPS at route node " + nextIndex + " " + nextLat + "," + nextLon);
        controller.processLocation(nextLat, nextLon, -1.0, -1.0, baseTime + 2000);

        assertTrue(positionLatch.await(10, TimeUnit.SECONDS),
            "navigation should report two position estimates");

        assertTrue(positions.size() >= 2, "expected at least two position callbacks");
        NavigationPosition first = positions.get(0);
        NavigationPosition last = positions.get(positions.size() - 1);
        double dx = Math.abs(first.lat - last.lat);
        double dy = Math.abs(first.lon - last.lon);
        assertTrue(dx > 1e-7 || dy > 1e-7,
            "position estimate should move between fixes, first=" + first + " last=" + last);

        // Verify that route instructions were delivered
        assertTrue(instructionLatch.await(10, TimeUnit.SECONDS),
            "navigation should report at least one next-route instruction");
        assertNotNull(lastInstructions[0],
            "route instructions callback should have been called");

        controller.stop();
        localClient.close();
    }

    @Test
    public void testBicycleRouteCalculation() throws InterruptedException {
        Path mapDir = defaultMapDir();
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir.toString())
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withCustomPoiType("_route_start")
            .withCustomPoiType("_route_end");

        OSMScoutClient localClient = builder.build();
        Assumptions.assumeTrue(localClient != null, "client already initialised");
        assertTrue(localClient.openDatabase(mapDir.toString()), "database should open");

        CountDownLatch routeLatch = new CountDownLatch(1);
        RouteEntry[] routeHolder = new RouteEntry[1];
        RouteCallback routeCallback = new RouteCallback() {
            @Override public void onProgress(int percent) {}
            @Override public void onSuccess(RouteEntry route) {
                routeHolder[0] = route;
                routeLatch.countDown();
            }
            @Override public void onError(String message) {
                routeLatch.countDown();
            }
            @Override public void onCancel() {
                routeLatch.countDown();
            }
        };

        // Route near Dortmund city center with bicycle profile
        RoutingProfile bikeProfile = new RoutingProfile(Vehicle.BICYCLE);
        localClient.calculateRouteAsync(
            51.514227, 7.465279,
            51.515, 7.466,
            bikeProfile,
            routeCallback);

        assertTrue(routeLatch.await(30, TimeUnit.SECONDS),
            "bicycle route calculation should complete");
        RouteEntry route = routeHolder[0];
        assertNotNull(route, "bicycle route should be calculated");
        assertTrue(route.routeHandle != 0, "route handle should be set");
        assertTrue(route.distance > 0, "bicycle route distance should be positive");
        System.out.println("[Test] bicycle route distance=" + route.distance + "m duration=" + route.duration + "s");

        localClient.close();
    }

    @Test
    public void testPedestrianRouteCalculation() throws InterruptedException {
        Path mapDir = defaultMapDir();
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir.toString())
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withCustomPoiType("_route_start")
            .withCustomPoiType("_route_end");

        OSMScoutClient localClient = builder.build();
        Assumptions.assumeTrue(localClient != null, "client already initialised");
        assertTrue(localClient.openDatabase(mapDir.toString()), "database should open");

        CountDownLatch routeLatch = new CountDownLatch(1);
        RouteEntry[] routeHolder = new RouteEntry[1];
        RouteCallback routeCallback = new RouteCallback() {
            @Override public void onProgress(int percent) {}
            @Override public void onSuccess(RouteEntry route) {
                routeHolder[0] = route;
                routeLatch.countDown();
            }
            @Override public void onError(String message) {
                routeLatch.countDown();
            }
            @Override public void onCancel() {
                routeLatch.countDown();
            }
        };

        // Route near Dortmund city center with pedestrian profile
        RoutingProfile pedProfile = new RoutingProfile(Vehicle.PEDESTRIAN);
        localClient.calculateRouteAsync(
            51.514227, 7.465279,
            51.515, 7.466,
            pedProfile,
            routeCallback);

        assertTrue(routeLatch.await(30, TimeUnit.SECONDS),
            "pedestrian route calculation should complete");
        RouteEntry route = routeHolder[0];
        assertNotNull(route, "pedestrian route should be calculated");
        assertTrue(route.routeHandle != 0, "route handle should be set");
        assertTrue(route.distance > 0, "pedestrian route distance should be positive");
        System.out.println("[Test] pedestrian route distance=" + route.distance + "m duration=" + route.duration + "s");

        localClient.close();
    }

    @Test
    public void testBicycleNavigationSession() throws InterruptedException {
        Path mapDir = defaultMapDir();
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir.toString())
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withCustomPoiType("_route_start")
            .withCustomPoiType("_route_end");

        OSMScoutClient localClient = builder.build();
        Assumptions.assumeTrue(localClient != null, "client already initialised");
        assertTrue(localClient.openDatabase(mapDir.toString()), "database should open");

        // Calculate bicycle route
        CountDownLatch routeLatch = new CountDownLatch(1);
        RouteEntry[] routeHolder = new RouteEntry[1];
        RouteCallback routeCallback = new RouteCallback() {
            @Override public void onProgress(int percent) {}
            @Override public void onSuccess(RouteEntry route) {
                routeHolder[0] = route;
                routeLatch.countDown();
            }
            @Override public void onError(String message) {
                routeLatch.countDown();
            }
            @Override public void onCancel() {
                routeLatch.countDown();
            }
        };

        localClient.calculateRouteAsync(
            51.514227, 7.465279,
            51.515, 7.466,
            new RoutingProfile(Vehicle.BICYCLE),
            routeCallback);

        assertTrue(routeLatch.await(30, TimeUnit.SECONDS),
            "bicycle route calculation should complete");
        RouteEntry route = routeHolder[0];
        assertNotNull(route, "bicycle route should be calculated");

        // Start navigation with bicycle vehicle
        List<NavigationPosition> positions = new ArrayList<>();
        CountDownLatch positionLatch = new CountDownLatch(1);
        NavigationListener listener = new NavigationListener() {
            @Override
            public void onPositionEstimate(NavigationPosition position) {
                System.out.println("[BikeNav] position: " + position);
                if (position != null) {
                    positions.add(position);
                    positionLatch.countDown();
                }
            }
            @Override
            public void onError(String message) {
                System.out.println("[BikeNav] error: " + message);
            }
        };

        NavigationController controller = localClient.startNavigation(
            route.routeHandle, Vehicle.BICYCLE, listener);
        assertNotNull(controller, "bicycle navigation controller should be created");

        // Feed a GPS fix at the snapped route start
        double startLat = route.latitudes[0];
        double startLon = route.longitudes[0];
        controller.processLocation(startLat, startLon, -1.0, -1.0, System.currentTimeMillis());

        assertTrue(positionLatch.await(10, TimeUnit.SECONDS),
            "bicycle navigation should report a position estimate");
        assertFalse(positions.isEmpty(), "should have at least one position");

        controller.stop();
        localClient.close();
    }
}
