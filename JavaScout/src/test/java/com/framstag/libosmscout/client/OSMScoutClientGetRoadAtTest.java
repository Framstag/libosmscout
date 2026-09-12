package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#getRoadAt} (spec:
 * road-lookup-bearing). Skipped automatically when the native library is not
 * available. The database-driven scenario additionally requires
 * {@code -Dpoi.test.db.dir} pointing at an openable .osmscout database
 * directory (same fixture as the POI search tests).
 */
public class OSMScoutClientGetRoadAtTest {

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

    @Test
    public void testGetRoadAtBeforeDatabaseOpenReturnsNull() {
        // No databases registered — the lookup must return null without error.
        RoadInfo road = client.getRoadAt(52.0, 8.0, 90.0);
        assertNull(road);
    }

    @Test
    public void testGetRoadAtWithDatabaseReturnsRoadInfoOrNull() {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping database-driven scenario");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(dbClient != null, "could not build client");
        Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");

        // A coordinate in the middle of the map: the lookup must either
        // resolve a road (with consistent fields) or return null — never crash.
        RoadInfo road = dbClient.getRoadAt(52.0, 8.0, 90.0);
        if (road != null) {
            assertNotNull(road.typeName);
            assertTrue(road.maxSpeedKmH > 0.0 || Double.isNaN(road.maxSpeedKmH));
        }

        dbClient.close();
    }

    @Test
    public void testGetRoadAtWithUnknownBearingDoesNotCrash() {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping database-driven scenario");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(dbClient != null, "could not build client");
        Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");

        // NaN bearing (stationary) must fall back to the nearest way.
        RoadInfo road = dbClient.getRoadAt(52.0, 8.0, Double.NaN);
        assertTrue(road == null || road.hasInfo() || !road.typeName.isEmpty());

        dbClient.close();
    }
}
