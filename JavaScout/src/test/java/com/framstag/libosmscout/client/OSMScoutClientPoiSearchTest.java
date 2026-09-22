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

    // The fixtures available to this project carry operator data for charging
    // stations (and, in the larger extracts, brand data), so that category is
    // used to check that the attributes reach the result. An object without
    // either attribute must still yield a usable result carrying an empty
    // value, and the attributes must never be null.
    @Test
    public void testSearchResultsExposeOperatorAndBrand() throws InterruptedException {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping attribute scenario");

        OSMScoutClient dbClient = openFixtureClient(dbDir);
        PoiEntry[] results = searchUntilLoaded(dbClient, PoiCategories.CHARGING_STATION);
        Assumptions.assumeTrue(results != null && results.length > 0,
            "fixture has no charging stations to check the attributes against");

        int withOperator = 0;
        for (PoiEntry entry : results) {
            assertNotNull(entry.operator, "operator must never be null");
            assertNotNull(entry.brand, "brand must never be null");
            if (!entry.operator.isEmpty()) {
                withOperator++;
            }
            // An object without the attribute is still a usable result: the
            // attribute is empty and the entry keeps its label and type.
            assertNotNull(entry.label);
            assertNotNull(entry.objectType);
        }

        assertTrue(withOperator > 0,
            "expected at least one result carrying an operator, results=" + results.length);

        dbClient.close();
    }

    // Spec: the label fallback must not consume the operator. A charging
    // station without a name has its label derived from the operator, and must
    // still carry that operator as an attribute of its own.
    @Test
    public void testOperatorDerivedLabelStillCarriesOperator() throws InterruptedException {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping attribute scenario");

        OSMScoutClient dbClient = openFixtureClient(dbDir);
        PoiEntry[] results = searchUntilLoaded(dbClient, PoiCategories.CHARGING_STATION);
        Assumptions.assumeTrue(results != null && results.length > 0,
            "fixture has no charging stations to check the label fallback against");

        int operatorDerivedLabels = 0;
        for (PoiEntry entry : results) {
            if (!entry.operator.isEmpty() && entry.label.equals(entry.operator)) {
                // The label came from the operator; the operator is still there.
                assertFalse(entry.label.isEmpty(),
                    "an operator-derived label is never empty");
                operatorDerivedLabels++;
            }
        }

        assertTrue(operatorDerivedLabels > 0,
            "expected at least one result whose label is derived from its operator, results="
                + results.length);

        dbClient.close();
    }

    // The JNI allows one open database client at a time and the database load
    // is asynchronous, so building the fixture client is retried until the
    // previous one is gone.
    private static OSMScoutClient openFixtureClient(String dbDir) throws InterruptedException {
        OSMScoutClient dbClient = null;
        for (int i = 0; i < 20 && dbClient == null; i++) {
            dbClient = new OSMScoutClientBuilder()
                .withMapLookupDirectories(dbDir)
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .build();
            if (dbClient == null) {
                Thread.sleep(250);
            }
        }
        Assumptions.assumeTrue(dbClient != null, "could not build client");
        Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");
        return dbClient;
    }

    // A search issued while the database is still loading returns no results;
    // poll until the fixture answers, bounded.
    private static PoiEntry[] searchUntilLoaded(OSMScoutClient dbClient, String category)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + 20000;
        PoiEntry[] results = null;
        while (System.currentTimeMillis() < deadline) {
            results = dbClient.searchPOIs(category, 51.5136, 7.4653, 20000, 50);
            if (results != null && results.length > 0) {
                return results;
            }
            Thread.sleep(250);
        }
        return results;
    }
}
