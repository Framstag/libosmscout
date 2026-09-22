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

    // -------------------------------------------------------------------
    // Multi-database scenarios
    //
    // A city extract and the larger extract containing it both hold the same
    // POIs. The search has to merge every loaded database, return each POI at
    // most once, prefer the copy of the database containing the search center
    // and keep the result order reproducible.
    //
    // Requires a directory with at least two openable .osmscout databases
    // whose areas overlap around the search center:
    //   -Dpoi.test.multidb.dir=<directory holding the overlapping databases>
    //   -Dpoi.test.multidb.single.dir=<the database containing the center>
    // The second property defaults to <dir>/Dortmund, matching the maps/
    // layout of this repository. Without the first property every scenario is
    // skipped.
    // -------------------------------------------------------------------

    private static final double MULTIDB_LAT = 51.5136;
    private static final double MULTIDB_LON = 7.4653;
    private static final double MULTIDB_RADIUS = 20000.0;
    private static final int MULTIDB_LIMIT = 100;

    // Spec: a hit that only a later database holds is found.
    @Test
    public void testMultiDatabaseSearchCoversEveryLoadedDatabase() throws InterruptedException {
        String dbDir = multiDatabaseDir();

        OSMScoutClient mergedClient = openLookupClient(dbDir);
        PoiEntry[] merged = searchStable(mergedClient, PoiCategories.RESTAURANTS);
        mergedClient.close();

        Assumptions.assumeTrue(merged != null && merged.length > 0,
            "the overlapping databases hold no restaurant around the search center");

        OSMScoutClient singleClient = openLookupClient(singleDatabaseDir(dbDir));
        PoiEntry[] single = searchStable(singleClient, PoiCategories.RESTAURANTS);
        singleClient.close();

        java.util.Set<String> singleKeys = new java.util.HashSet<>();
        if (single != null) {
            for (PoiEntry entry : single) {
                singleKeys.add(entryKey(entry));
            }
        }

        boolean otherDatabaseContributed = false;
        for (PoiEntry entry : merged) {
            if (!singleKeys.contains(entryKey(entry))) {
                otherDatabaseContributed = true;
                break;
            }
        }

        assertTrue(otherDatabaseContributed,
            "expected results from the database that is not the center's own, merged="
                + merged.length + " single=" + (single == null ? 0 : single.length));
        assertTrue(merged.length <= MULTIDB_LIMIT, "the limit applies to the merged list");
    }

    // Spec: a POI present in several databases is returned once.
    @Test
    public void testMultiDatabaseSearchCollapsesDuplicatePois() throws InterruptedException {
        String dbDir = multiDatabaseDir();

        OSMScoutClient mergedClient = openLookupClient(dbDir);
        PoiEntry[] merged = searchStable(mergedClient, PoiCategories.RESTAURANTS);
        mergedClient.close();

        Assumptions.assumeTrue(merged != null && merged.length > 0,
            "the overlapping databases hold no restaurant around the search center");

        java.util.Set<String> keys = new java.util.HashSet<>();
        for (PoiEntry entry : merged) {
            assertTrue(keys.add(entryKey(entry)),
                "duplicate POI in the merged result: " + entry.objectType
                    + " '" + entry.label + "'");
        }
    }

    // Spec: the copy from the database containing the search center survives.
    @Test
    public void testMultiDatabaseSearchKeepsTheCopyOfTheCentersDatabase()
        throws InterruptedException {
        String dbDir = multiDatabaseDir();

        OSMScoutClient mergedClient = openLookupClient(dbDir);
        PoiEntry[] merged = searchStable(mergedClient, PoiCategories.RESTAURANTS);
        mergedClient.close();

        Assumptions.assumeTrue(merged != null && merged.length > 0,
            "the overlapping databases hold no restaurant around the search center");

        OSMScoutClient singleClient = openLookupClient(singleDatabaseDir(dbDir));
        PoiEntry[] single = searchStable(singleClient, PoiCategories.RESTAURANTS);
        singleClient.close();

        Assumptions.assumeTrue(single != null && single.length > 0,
            "the center's database returned no restaurant");

        java.util.Map<String, PoiEntry> singleByKey = new java.util.HashMap<>();
        for (PoiEntry entry : single) {
            singleByKey.put(entryKey(entry), entry);
        }

        int compared = 0;
        for (PoiEntry entry : merged) {
            PoiEntry fromSingleDatabase = singleByKey.get(entryKey(entry));
            if (fromSingleDatabase == null) {
                continue;
            }
            compared++;
            assertEquals(fromSingleDatabase.label, entry.label,
                "the surviving copy must be the center's database copy");
            assertEquals(fromSingleDatabase.lat, entry.lat, 1e-9);
            assertEquals(fromSingleDatabase.lon, entry.lon, 1e-9);
            assertEquals(fromSingleDatabase.distance, entry.distance, 1e-6);
        }

        assertTrue(compared > 0,
            "expected the merged result to share POIs with the center's database");
    }

    // Spec: POI search result order is deterministic.
    @Test
    public void testMultiDatabaseSearchOrderIsDeterministic() throws InterruptedException {
        String dbDir = multiDatabaseDir();

        OSMScoutClient mergedClient = openLookupClient(dbDir);
        PoiEntry[] first = searchStable(mergedClient, PoiCategories.RESTAURANTS);
        PoiEntry[] second = mergedClient.searchPOIs(PoiCategories.RESTAURANTS,
            MULTIDB_LAT, MULTIDB_LON, MULTIDB_RADIUS, MULTIDB_LIMIT);
        mergedClient.close();

        Assumptions.assumeTrue(first != null && first.length > 0,
            "the overlapping databases hold no restaurant around the search center");
        assertNotNull(second);

        assertEquals(signatures(first), signatures(second),
            "the same search must return the same list");

        for (int i = 1; i < first.length; i++) {
            assertTrue(first[i - 1].distance <= first[i].distance,
                "results must be ordered nearest first");
        }
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

    private static String multiDatabaseDir() {
        String dbDir = System.getProperty("poi.test.multidb.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.multidb.dir not set - skipping multi-database scenario");
        return dbDir;
    }

    private static String singleDatabaseDir(String dbDir) {
        String single = System.getProperty("poi.test.multidb.single.dir");
        if (single == null || single.isEmpty()) {
            single = dbDir + "/Dortmund";
        }
        return single;
    }

    // The map lookup scan loads the databases asynchronously, so a search can
    // observe a partially loaded set. Poll until two consecutive searches
    // agree, bounded.
    private static PoiEntry[] searchStable(OSMScoutClient dbClient, String category)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + 30000;
        PoiEntry[] previous = null;
        PoiEntry[] results = null;
        while (System.currentTimeMillis() < deadline) {
            results = dbClient.searchPOIs(category, MULTIDB_LAT, MULTIDB_LON,
                MULTIDB_RADIUS, MULTIDB_LIMIT);
            if (previous != null && results != null
                && signatures(previous).equals(signatures(results))) {
                return results;
            }
            previous = results;
            Thread.sleep(500);
        }
        return results;
    }

    // The JNI allows only one open database client at a time, so building a
    // client is retried until the previous one is gone.
    private static OSMScoutClient openLookupClient(String lookupDir) throws InterruptedException {
        OSMScoutClient dbClient = null;
        for (int i = 0; i < 20 && dbClient == null; i++) {
            dbClient = new OSMScoutClientBuilder()
                .withMapLookupDirectories(lookupDir)
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .build();
            if (dbClient == null) {
                Thread.sleep(250);
            }
        }
        Assumptions.assumeTrue(dbClient != null, "could not build client");
        return dbClient;
    }

    private static String signatures(PoiEntry[] entries) {
        StringBuilder text = new StringBuilder();
        for (PoiEntry entry : entries) {
            text.append(entryKey(entry))
                .append('|').append(entry.label)
                .append('|').append(entry.distance)
                .append('\n');
        }
        return text.toString();
    }

    // Content-based identity of a POI: its object type and its position
    // rounded to about one meter, the same criterion the merge uses to
    // collapse the copies of one object held by overlapping databases.
    private static String entryKey(PoiEntry entry) {
        return entry.objectType + "@"
            + Math.round(entry.lat * 1e5) + "," + Math.round(entry.lon * 1e5);
    }
}
