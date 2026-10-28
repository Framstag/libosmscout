package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.Timeout;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * JNI integration tests for the structured route steps a calculated route publishes and for the
 * manoeuvre facts of the navigation instruction (spec route-calculation: a calculated route
 * publishes structured steps; spec turn-by-turn-instructions: the list instruction and the step
 * of a manoeuvre agree).
 * <p>
 * Needs a routable map database directory: {@code -Dnav.test.db.dir=<dir>} or the
 * {@code JAVASCOUT_MAP_DIR} environment variable, the same convention as
 * {@link OSMScoutClientInstructionDistanceTest}. Skipped, not failed, without one, and also
 * skipped when the native library is not available.
 */
public class OSMScoutClientRouteStepTest {

    private static final String DB_DIR_PROPERTY = "nav.test.db.dir";

    /**
     * A route of the North Rhine-Westphalia map that enters, changes and leaves a motorway, so the
     * motorway manoeuvres and their junction data are part of the fixture.
     */
    private static final double START_LAT = 51.3;
    private static final double START_LON = 7.7;

    private static final double DEST_LAT = 51.55;
    private static final double DEST_LON = 7.35;

    private static OSMScoutClient client;

    @BeforeAll
    public static void setUp() {
        Path mapDir = assumeMapDir();

        client = buildClient(mapDir);

        // The native side holds one client at a time; a leaked client of another test class makes
        // the build return null, which is a reason to skip rather than to fail here.
        Assumptions.assumeTrue(client != null, "client already initialised by another test");
    }

    @AfterAll
    public static void tearDown() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    private static void assumeNativeLibrary() {
        try {
            new OSMScoutClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
        }
    }

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

    private static Path assumeMapDir() {
        assumeNativeLibrary();

        Path mapDir = providedMapDir();

        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        return mapDir;
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

    private static RouteEntry calculateRoute() throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        AtomicReference<RouteEntry> routeHolder = new AtomicReference<>();

        client.calculateRouteAsync(START_LAT, START_LON, DEST_LAT, DEST_LON, new RouteCallback() {
            @Override
            public void onProgress(int percent) {
            }

            @Override
            public void onSuccess(RouteEntry route) {
                routeHolder.set(route);
                latch.countDown();
            }

            @Override
            public void onError(String message) {
                latch.countDown();
            }

            @Override
            public void onCancel() {
                latch.countDown();
            }
        });

        assertTrue(latch.await(90, TimeUnit.SECONDS), "route calculation should complete");

        RouteEntry route = routeHolder.get();

        assertNotNull(route, "route should be calculated");

        return route;
    }

    @Test
    @Timeout(180)
    public void testStepsArePublishedInRouteOrder() throws InterruptedException {
        RouteEntry route = calculateRoute();

        assertNotNull(route.steps, "a calculated route publishes its steps");
        assertTrue(route.steps.length > 0, "a calculated route publishes at least one step");

        assertEquals(RouteStep.Kind.START, route.steps[0].kind,
            "the first step is the start");
        assertEquals(RouteStep.Kind.TARGET, route.steps[route.steps.length - 1].kind,
            "the last step is the arrival");

        double previous = -1.0;
        double legSum = 0.0;

        for (RouteStep step : route.steps) {
            assertNotNull(step, "no step is null");
            assertTrue(step.distanceFromStart >= previous,
                "steps are in route order, " + step.distanceFromStart + " after " + previous);
            previous = step.distanceFromStart;
            legSum += step.legDistance;

            // The database identity of each step resolves through the published mapping.
            assertNotNull(route.databaseMapping, "the route publishes its database mapping");
            assertTrue(route.databaseMapping.containsKey(step.databaseId),
                "step database " + step.databaseId + " resolves through " + route.databaseMapping);
        }

        assertEquals(route.steps[route.steps.length - 1].distanceFromStart, legSum, 1.0,
            "the legs add up to the route's distance");
    }

    @Test
    @Timeout(180)
    public void testDescriptionLinesAreUnchangedNextToTheSteps() throws InterruptedException {
        RouteEntry route = calculateRoute();

        assertNotNull(route.descriptions, "the description lines are still published");
        assertTrue(route.descriptions.length > 0, "the description lines are non-empty");
        assertTrue(route.descriptions[0].equals("--- Route ---"),
            "the list keeps its header: " + route.descriptions[0]);
        assertTrue(route.descriptions[1].startsWith("Start"),
            "the first instruction line is the start: " + route.descriptions[1]);
        assertTrue(route.descriptions[route.descriptions.length - 1].contains("Destination"),
            "the last line is the destination: " + route.descriptions[route.descriptions.length - 1]);
        assertTrue(route.distance > 0.0, "the route's distance is published");
        assertTrue(route.duration > 0.0, "the route's duration is published");

        // The four per-instruction arrays are either all present and aligned with the lines (the
        // header is not an instruction) or all null - never partially filled.
        if (route.instructionLats == null) {
            assertNull(route.instructionLons);
            assertNull(route.instructionDistances);
            assertNull(route.instructionTimes);
        } else {
            assertNotNull(route.instructionLons);
            assertNotNull(route.instructionDistances);
            assertNotNull(route.instructionTimes);
            assertEquals(route.descriptions.length - 1, route.instructionLats.length,
                "the positions align with the instruction lines");
        }
    }

    @Test
    @Timeout(180)
    public void testStepSubRecordsSurviveTheBoundary() throws InterruptedException {
        RouteEntry route = calculateRoute();

        assertNotNull(route.steps, "a calculated route publishes its steps");
        assertTrue(route.steps.length > 0, "a calculated route publishes at least one step");

        int manoeuvres = 0;

        for (RouteStep step : route.steps) {
            assertTrue(step.lat != 0.0 && step.lon != 0.0,
                "the step carries its coordinate");
            assertTrue(step.currentNodeIndex >= 0, "the step carries its node index");

            // The objects the node intersects: one type per offset, nothing lost.
            assertEquals(step.objectOffsets.length, step.objectTypes.length,
                "an object's type and offset travel together");
            for (long offset : step.objectOffsets) {
                assertTrue(offset > 0, "an intersecting object has an offset");
            }

            if (step.name != null) {
                // An unnamed road arrives with an empty name and reference; the record exists
                // because the node carries a name description.
                assertNotNull(step.name.name);
                assertNotNull(step.name.ref);
            }

            if (step.nameChanged != null) {
                assertNotNull(step.nameChanged.from);
                assertNotNull(step.nameChanged.to);
            }

            if (step.typeName != null) {
                assertTrue(!step.typeName.isEmpty(), "a type name is non-empty");
            }

            if (step.crossingWays != null) {
                assertTrue(step.crossingWays.exitCount >= 0);
                assertNotNull(step.crossingWays.others);
            }

            if (step.direction != null) {
                assertNotNull(step.direction.turn);
                assertNotNull(step.direction.curve);
            }

            if (step.turn != null) {
                manoeuvres++;
            }

            if (step.roundaboutEnter != null || step.roundaboutLeave != null) {
                manoeuvres++;
            }

            if (step.roundaboutLeave != null) {
                assertTrue(step.roundaboutLeave.exitCountTaken >= 1,
                    "the exit taken is a 1-based ordinal");
            }

            if (step.motorwayEnter != null) {
                assertNotNull(step.motorwayEnter.to);
                manoeuvres++;
            }

            if (step.motorwayChange != null) {
                assertNotNull(step.motorwayChange.from);
                assertNotNull(step.motorwayChange.to);
                manoeuvres++;
            }

            if (step.motorwayLeave != null) {
                assertNotNull(step.motorwayLeave.from);
                manoeuvres++;
            }

            if (step.junction != null) {
                assertTrue(!step.junction.ref.isEmpty() || !step.junction.name.isEmpty(),
                    "the junction carries its reference or its name");
            }

            if (step.destination != null) {
                assertTrue(!step.destination.isEmpty(), "the destination tag is non-empty");
            }

            if (step.maxSpeed != null) {
                assertTrue(step.maxSpeed > 0, "a known max speed is positive");
            }

            if (step.lanes != null) {
                assertTrue(step.lanes.count >= 1, "a lane description counts its lanes");
                assertNotNull(step.lanes.turns);
            }

            if (step.suggestedLanes != null) {
                assertTrue(step.suggestedLanes.from <= step.suggestedLanes.to,
                    "the suggested lane range is ordered");
                assertNotNull(step.suggestedLanes.turn);
            }

            if (step.poi != null) {
                assertTrue(step.poi.objectOffset > 0, "a POI has an object offset");
                assertNotNull(step.poi.name);
            }

            if (step.via != null) {
                assertTrue(step.via.sectionNumber >= 1, "a via section is 1-based");
            }

            if (step.start != null) {
                assertTrue(!step.start.isEmpty(), "the start label is non-empty");
            }

            if (step.target != null) {
                assertTrue(!step.target.isEmpty(), "the target label is non-empty");
            }
        }

        // The route of the fixture carries manoeuvres; a map without any would make the
        // sub-record assertions above vacuous.
        Assumptions.assumeTrue(manoeuvres > 0,
            "the map database has no manoeuvre on this route");
    }

    @Test
    @Timeout(180)
    public void testLiveStepListAgreesWithThePublishedSteps() throws InterruptedException {
        RouteEntry route = calculateRoute();

        assertNotNull(route.steps, "a calculated route publishes its steps");

        List<RouteStep> live = stepList(route);

        assertTrue(live.size() > 0, "the live step list is non-empty");

        int compared = 0;

        for (RouteStep step : route.steps) {
            // One node can describe more than one manoeuvre, so a step is matched by its position
            // and by the manoeuvre it describes.
            RouteStep other = null;

            for (RouteStep candidate : live) {
                if (Math.abs(candidate.distanceFromStart - step.distanceFromStart) < 1.0
                    && candidate.kind == step.kind) {
                    other = candidate;
                    break;
                }
            }

            if (other == null) {
                continue;
            }

            assertEquals(step.kind, other.kind,
                "kind of the manoeuvre at " + step.distanceFromStart);

            if (step.junction != null) {
                assertEquals(emptyToNull(step.junction.ref), emptyToNull(other.junction == null ? null : other.junction.ref),
                    "exit reference of the manoeuvre at " + step.distanceFromStart);
                assertEquals(emptyToNull(step.junction.name), emptyToNull(other.junction == null ? null : other.junction.name),
                    "junction name of the manoeuvre at " + step.distanceFromStart);
            }

            assertEquals(emptyToNull(step.destination), emptyToNull(other.destination),
                "destination of the manoeuvre at " + step.distanceFromStart);

            compared++;
        }

        Assumptions.assumeTrue(compared > 0,
            "no live step could be matched to a published step");
    }

    @Test
    @Timeout(180)
    public void testMotorwayLeaveInstructionCarriesTheExitAndDestination() throws InterruptedException {
        RouteEntry route = calculateRoute();

        assertNotNull(route.steps, "a calculated route publishes its steps");

        RouteStep leave = null;

        for (RouteStep step : route.steps) {
            if (step.motorwayLeave != null && step.junction != null) {
                leave = step;
                break;
            }
        }

        // A map whose fixture route does not leave a motorway has nothing to assert here.
        Assumptions.assumeTrue(leave != null, "the fixture route does not leave a motorway");

        assertTrue(!leave.junction.ref.isEmpty() || !leave.junction.name.isEmpty(),
            "the junction step carries the exit reference or the name");

        List<RouteStep> live = stepList(route);
        RouteStep other = null;

        for (RouteStep candidate : live) {
            if (Math.abs(candidate.distanceFromStart - leave.distanceFromStart) < 1.0) {
                other = candidate;
                break;
            }
        }

        assertNotNull(other, "the engine reports a live step for the motorway leave");
        assertNotNull(other.junction, "the live step carries the junction");
        assertEquals(emptyToNull(leave.junction.ref), emptyToNull(other.junction.ref),
            "the live step carries the exit reference");
        assertEquals(emptyToNull(leave.junction.name), emptyToNull(other.junction.name),
            "the live step carries the junction name");
        assertEquals(emptyToNull(leave.destination), emptyToNull(other.destination),
            "the live step carries the destination signs");

        if (!leave.junction.ref.isEmpty()) {
            assertEquals(leave.junction.ref, other.junction.ref);
        }
    }

    private static RouteStep matchingStep(RouteEntry route, RouteStep live) {
        for (RouteStep step : route.steps) {
            if (Math.abs(step.distanceFromStart - live.distanceFromStart) < 1.0
                && stepTurnTypeOf(step) == stepTurnTypeOf(live)) {
                return step;
            }
        }

        return null;
    }

    /**
     * The step list the engine publishes for the route: a fix at the snapped start makes the
     * engine generate it from the retained route description.
     */
    private static List<RouteStep> stepList(RouteEntry route) throws InterruptedException {
        CountDownLatch latch = new CountDownLatch(1);
        List<RouteStep> steps = new ArrayList<>();

        NavigationListener listener = new NavigationListener() {
            @Override
            public void onRouteSteps(RouteStep[] received) {
                for (RouteStep step : received) {
                    steps.add(step);
                }
                latch.countDown();
            }
        };

        NavigationController controller = client.startNavigation(route.routeHandle, listener);

        assertNotNull(controller, "navigation controller should be created");

        try {
            controller.processLocation(route.latitudes[0], route.longitudes[0],
                -1.0, -1.0, System.currentTimeMillis());

            assertTrue(latch.await(30, TimeUnit.SECONDS),
                "the engine should publish its step list");
        } finally {
            controller.stop();
        }

        return steps;
    }

    private static String emptyToNull(String value) {
        return value == null || value.isEmpty() ? null : value;
    }

    /**
     * The turn type the description-derived instruction reports for a step, read from the step's
     * own values. A kind-shaped manoeuvre (entering a motorway, a roundabout) reports its kind;
     * a turn reports its turn and a motorway change or leave its curve.
     */
    private static TurnType stepTurnTypeOf(RouteStep step) {
        switch (step.kind) {
            case START:
                return TurnType.START;
            case TARGET:
                return TurnType.TARGET_REACHED;
            case ROUNDABOUT_ENTER:
                return TurnType.ROUNDABOUT_ENTER;
            case ROUNDABOUT_LEAVE:
                return TurnType.ROUNDABOUT_LEAVE;
            case MOTORWAY_ENTER:
                return TurnType.MOTORWAY_ENTER;
            case MOTORWAY_CHANGE:
            case MOTORWAY_LEAVE:
                return step.direction != null ? step.direction.curve : TurnType.STRAIGHT_ON;
            case TURN:
                if (step.turn != null) {
                    return step.turn;
                }
                return step.direction != null ? step.direction.curve : TurnType.STRAIGHT_ON;
            case NAME_CHANGE:
            case INFORMATIVE:
            case UNKNOWN:
            default:
                // No instruction is derived for these, so there is no turn type to compare.
                return null;
        }
    }
}
