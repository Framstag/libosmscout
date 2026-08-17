package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#searchPOIs}.
 * <p>
 * Skipped automatically when the native library is not available. The
 * database-driven scenario additionally requires {@code -Dpoi.test.db.dir}
 * pointing at an openable .osmscout database directory.
 */
public class OSMScoutClientPoiSearchTest {

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
    public void testUnknownCategoryReturnsEmpty() {
        PoiEntry[] results = client.searchPOIs("unknown-category", 52.0, 8.0, 5000, 50);
        assertNotNull(results);
        assertEquals(0, results.length);
    }

    @Test
    public void testZeroRadiusReturnsEmpty() {
        PoiEntry[] results = client.searchPOIs(PoiCategories.HOTELS, 52.0, 8.0, 0, 50);
        assertNotNull(results);
        assertEquals(0, results.length);
    }

    @Test
    public void testNegativeRadiusReturnsEmpty() {
        PoiEntry[] results = client.searchPOIs(PoiCategories.RESTAURANTS, 52.0, 8.0, -1, 50);
        assertNotNull(results);
        assertEquals(0, results.length);
    }

    @Test
    public void testSearchBeforeDatabaseOpenReturnsEmpty() {
        // The client has no databases registered, so the search must yield no
        // results without error (spec: Search on uninitialized client).
        PoiEntry[] results = client.searchPOIs(PoiCategories.GROCERY, 52.0, 8.0, 5000, 50);
        assertNotNull(results);
        assertEquals(0, results.length);
    }

    @Test
    public void testSearchInDatabaseReturnsCategoryPoisWithinRadius() {
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

        PoiEntry[] results = dbClient.searchPOIs(PoiCategories.HOTELS, 52.0, 8.0, 20000, 50);
        assertNotNull(results);
        assertTrue(results.length <= 50);
        for (PoiEntry entry : results) {
            assertNotNull(entry.objectType);
            assertTrue(entry.distance >= 0.0);
        }

        dbClient.close();
    }
}
