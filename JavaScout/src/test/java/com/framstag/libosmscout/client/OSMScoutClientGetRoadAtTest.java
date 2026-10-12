package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#getRoadAt} (spec:
 * road-lookup-bearing). Skipped automatically when the native library is not
 * available. The database-driven scenario additionally requires
 * {@code -Dpoi.test.db.dir} pointing at an openable .osmscout database
 * directory (same fixture as the POI search tests).
 * <p>
 * Every test that needs a client builds its own and releases it however the test ends,
 * so a skipped database-driven scenario cannot leave a client behind for a later test.
 */
public class OSMScoutClientGetRoadAtTest {

    @BeforeEach
    public void setUp() {
        TestClients.assumeNativeLibrary();
    }

    /** A client of this test's own, or null when one is already active. */
    private static OSMScoutClient buildClient(String mapDir) {
        return new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
    }

    @Test
    public void testGetRoadAtBeforeDatabaseOpenReturnsNull(@TempDir Path emptyDir) {
        OSMScoutClient noMapClient = buildClient(emptyDir.toString());
        Assumptions.assumeTrue(noMapClient != null, "could not build client");

        try {
            // The claim is about a client without database, so it must not be answered by
            // the "client not initialised" guard instead.
            assertTrue(noMapClient.isInitialized(), "the test needs an initialised client");

            RoadInfo road = noMapClient.getRoadAt(52.0, 8.0, 90.0);
            assertNull(road, "no databases registered - the lookup must return null without error");
        } finally {
            noMapClient.close();
        }
    }

    @Test
    public void testGetRoadAtWithDatabaseReturnsRoadInfoOrNull() {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping database-driven scenario");

        OSMScoutClient dbClient = buildClient(dbDir);
        Assumptions.assumeTrue(dbClient != null, "could not build client");

        try {
            Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");

            // A coordinate in the middle of the map: the lookup must either
            // resolve a road (with consistent fields) or return null — never crash.
            RoadInfo road = dbClient.getRoadAt(52.0, 8.0, 90.0);
            if (road != null) {
                assertNotNull(road.typeName);
                assertTrue(road.maxSpeedKmH > 0.0 || Double.isNaN(road.maxSpeedKmH));
            }
        } finally {
            dbClient.close();
        }
    }

    @Test
    public void testGetRoadAtWithUnknownBearingDoesNotCrash() {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping database-driven scenario");

        OSMScoutClient dbClient = buildClient(dbDir);
        Assumptions.assumeTrue(dbClient != null, "could not build client");

        try {
            Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");

            // NaN bearing (stationary) must fall back to the nearest way.
            RoadInfo road = dbClient.getRoadAt(52.0, 8.0, Double.NaN);
            assertTrue(road == null || road.hasInfo() || !road.typeName.isEmpty());
        } finally {
            dbClient.close();
        }
    }
}
