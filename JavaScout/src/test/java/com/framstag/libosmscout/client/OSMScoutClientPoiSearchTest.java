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

    // Multi-database search: the JNI layer pre-sorts databases by bounding-box
    // containment of the search center, merges results from every loaded
    // non-basemap database, deduplicates overlapping databases and returns the
    // merged list distance-ascending, truncated to the limit. Requires
    // -Dpoi.test.multidb.dir pointing at a directory with at least two
    // openable .osmscout databases whose bounding boxes contain the search
    // center and overlap each other (e.g. a city extract and its state
    // extract: Dortmund + nordrhein-westfalen).
    @Test
    public void testMultiDatabaseSearchMergesAndSortsAllDatabases() {
        String dbDir = System.getProperty("poi.test.multidb.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.multidb.dir not set - skipping multi-database scenario");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(dbClient != null, "could not build client");

        // The builder's map-lookup scan loads every map under dbDir; the
        // database load is asynchronous and sequential per database, so a
        // search can observe a partially loaded set. Wait until two
        // consecutive searches return identical results (bounded).
        PoiEntry[] results = searchStable(dbClient, PoiCategories.RESTAURANTS, 51.51, 7.46, 20000, 100);
        assertNotNull(results);
        assertTrue(results.length > 0, "expected POIs from the loaded maps");
        assertTrue(results.length <= 100, "limit must be respected");

        // Results must be ordered by distance ascending.
        for (int i = 1; i < results.length; i++) {
            assertTrue(results[i - 1].distance <= results[i].distance,
                "results must be distance-ascending");
        }

        // The merged results must contain entries that only the second database
        // (nordrhein-westfalen) can provide - proof that the search does not
        // stop at the first database in load order. The two extracts come from
        // different OSM snapshots, so their copies of the same objects differ
        // in coordinates and labels; the dedup keeps the first (Dortmund)
        // copy, so entries unique to NRW must still appear in the merged list.
        // The JNI allows only one active client per JVM, so the multi client is
        // closed before the single-map client is built.
        dbClient.close();

        String firstDir = dbDir + "/Dortmund";
        OSMScoutClient firstClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(firstDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(firstClient != null, "could not build first-map client");
        PoiEntry[] first = searchStable(firstClient, PoiCategories.RESTAURANTS, 51.51, 7.46, 20000, 100);
        assertTrue(first.length > 0, "expected first-map POIs");

        java.util.Set<String> firstKeys = new java.util.HashSet<>();
        for (PoiEntry entry : first) {
            firstKeys.add(coordKey(entry));
        }
        boolean secondDbContributed = false;
        for (PoiEntry entry : results) {
            if (!firstKeys.contains(coordKey(entry))) {
                secondDbContributed = true;
                break;
            }
        }
        assertTrue(secondDbContributed,
            "expected results from the second database (nordrhein-westfalen)");

        firstClient.close();
    }

    @Test
    public void testMultiDatabaseSearchDeduplicatesOverlappingDatabases() {
        String dbDir = System.getProperty("poi.test.multidb.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.multidb.dir not set - skipping multi-database scenario");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(dbClient != null, "could not build client");

        // Around Dortmund the city extract and the state extract contain the
        // same objects; each object must appear at most once in the merged list.
        PoiEntry[] results = searchStable(dbClient, PoiCategories.RESTAURANTS, 51.51, 7.46, 20000, 100);
        assertNotNull(results);
        assertTrue(results.length > 0, "expected overlapping-map POIs");

        java.util.Set<String> coords = new java.util.HashSet<>();
        for (PoiEntry entry : results) {
            assertTrue(coords.add(coordKey(entry)),
                "duplicate POI coordinates in merged results: " + entry.label);
        }

        dbClient.close();
    }

    @Test
    public void testMultiDatabaseSearchAppliesLimit() {
        String dbDir = System.getProperty("poi.test.multidb.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.multidb.dir not set - skipping multi-database scenario");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(dbClient != null, "could not build client");

        PoiEntry[] results = searchStable(dbClient, PoiCategories.RESTAURANTS, 51.51, 7.46, 20000, 5);
        assertNotNull(results);
        assertTrue(results.length > 0, "expected POIs");
        assertTrue(results.length <= 5, "limit must truncate the merged list");
        for (int i = 1; i < results.length; i++) {
            assertTrue(results[i - 1].distance <= results[i].distance,
                "results must be distance-ascending");
        }

        dbClient.close();
    }

    private static String coordKey(PoiEntry entry) {
        long latKey = Math.round(entry.lat * 1e5);
        long lonKey = Math.round(entry.lon * 1e5);
        return latKey + "/" + lonKey;
    }

    /** Wait (bounded) until two consecutive searches return identical results. */
    private static PoiEntry[] searchStable(OSMScoutClient client, String category,
                                            double lat, double lon, double radius, int limit) {
        long deadline = System.currentTimeMillis() + 15000;
        PoiEntry[] prev = null;
        while (System.currentTimeMillis() < deadline) {
            PoiEntry[] cur = client.searchPOIs(category, lat, lon, radius, limit);
            if (cur != null && cur.length > 0 && sameResults(prev, cur)) {
                return cur;
            }
            prev = cur;
            try {
                Thread.sleep(500);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                break;
            }
        }
        return prev;
    }

    private static boolean sameResults(PoiEntry[] a, PoiEntry[] b) {
        if (a == null || b == null || a.length != b.length) {
            return false;
        }
        for (int i = 0; i < a.length; i++) {
            if (!coordKey(a[i]).equals(coordKey(b[i]))) {
                return false;
            }
        }
        return true;
    }
}
