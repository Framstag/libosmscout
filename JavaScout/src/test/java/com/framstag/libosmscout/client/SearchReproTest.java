package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Regression tests for garbage free-text search entries.
 * <p>
 * Automatic tests must not assume that anything besides checked-in files
 * exists — in particular no generated map database. The default (automatic)
 * tests therefore run against a client without any database. The
 * database-driven reproduction scenario only runs when an operator explicitly
 * provides a map via the {@code -Dsearch.test.db.dir} system property
 * (same convention as {@code poi.test.db.dir} in {@code OSMScoutClientPoiSearchTest});
 * it is skipped otherwise.
 */
class SearchReproTest {

    private static final String DB_DIR_PROPERTY = "search.test.db.dir";

    /** Optional, operator-provided map database directory. Null when unset. */
    private static Path providedMapDir() {
        String dir = System.getProperty(DB_DIR_PROPERTY);
        if (dir == null || dir.isEmpty()) {
            return null;
        }
        return Paths.get(dir);
    }

    private static OSMScoutClient buildClient() {
        try {
            return new OSMScoutClientBuilder()
                .withMapLookupDirectories(".")
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .withCustomPoiType("_route_start")
                .withCustomPoiType("_route_end")
                .build();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.abort("native library not available: " + e.getMessage());
        }
        return null; // unreachable; Assumptions.abort throws
    }

    private static void assertNoGarbageResults(LocationEntry[] results, String query) {
        assertNotNull(results, "results must not be null for query '" + query + "'");
        for (LocationEntry e : results) {
            // Regression: free-text padding used to produce empty entries
            // with (0,0) coordinates and no label.
            assertTrue(e.label != null && !e.label.isEmpty(),
                "entry must have a label for query '" + query + "'");
            assertTrue(e.lat != 0.0 || e.lon != 0.0,
                "entry must not have (0,0) coordinates for query '" + query + "'");
        }
    }

    /**
     * Without an open database a search must yield an empty result set — and
     * never garbage entries with (0,0) coordinates and no label, no matter how
     * often the search is repeated.
     */
    @Test
    public void searchWithoutDatabaseReturnsNoGarbage() {
        OSMScoutClient client = buildClient();
        Assumptions.assumeTrue(client != null, "client not initialised");

        for (String query : new String[]{"Aldi Eving", "Java Eving"}) {
            LocationEntry[] first = client.searchLocations(query, 50, "Nordrhein-Westfalen", false);
            assertNoGarbageResults(first, query);
            assertEquals(0, first.length,
                "no database open - search must not return entries for query '" + query + "'");

            LocationEntry[] second = client.searchLocations(query, 50, "Nordrhein-Westfalen", false);
            assertNoGarbageResults(second, query);
            assertEquals(0, second.length,
                "repeated search must stay empty for query '" + query + "'");
        }

        client.close();
    }

    /**
     * An empty query must yield an empty result set, not padding entries.
     */
    @Test
    public void emptyQueryReturnsEmptyArray() {
        OSMScoutClient client = buildClient();
        Assumptions.assumeTrue(client != null, "client not initialised");

        LocationEntry[] results = client.searchLocations("", 50, null, false);
        assertNoGarbageResults(results, "");
        assertEquals(0, results.length, "empty query must not return entries");

        client.close();
    }

    /**
     * Full reproduction of the garbage-entry regression against a real map
     * database. Skipped unless an operator provides one explicitly via
     * {@code -Dsearch.test.db.dir}; automatic runs never assume a map exists.
     */
    @Test
    public void searchTwiceOnProvidedMapReturnsNoGarbage() {
        Path mapDir = providedMapDir();
        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient client = new OSMScoutClientBuilder()
                .withMapLookupDirectories(mapDir.toString())
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .withCustomPoiType("_route_start")
                .withCustomPoiType("_route_end")
                .build();
        Assumptions.assumeTrue(client != null, "client already initialised");

        assertTrue(client.openDatabase(mapDir.toString()), "database should open");

        for (String query : new String[]{"Aldi Eving", "Java Eving"}) {
            LocationEntry[] results = client.searchLocations(query, 50, "Nordrhein-Westfalen", false);
            Assumptions.assumeTrue(results != null, "null results");
            assertNoGarbageResults(results, query);
        }

        client.close();
    }
}
