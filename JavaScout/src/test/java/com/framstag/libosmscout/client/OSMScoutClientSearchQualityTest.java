package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for the per-attribute match quality of location search
 * results (spec: search-ranking-match).
 * <p>
 * Skipped automatically when the native library is not available. The
 * database-driven scenarios additionally require {@code -Dsearch.test.db.dir}
 * (or the {@code JAVASCOUT_MAP_DIR} environment variable) pointing at an
 * openable .osmscout database directory — the same convention as
 * {@link SearchReproTest} and {@link OSMScoutClientNavigationLiveTest}.
 */
public class OSMScoutClientSearchQualityTest {

    private static final String DB_DIR_PROPERTY = "search.test.db.dir";

    /** Every value {@code matchedComponent} may take. */
    private static final Set<String> COMPONENTS = new HashSet<>(Arrays.asList(
        "adminRegion", "location", "poi", "address", "coordinate", "freeText"));

    /** Every value a per-attribute match quality may take. */
    private static final Set<String> QUALITIES = new HashSet<>(Arrays.asList(
        "match", "candidate", "none"));

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

    /** A client on the given map directory, or a skip when one is already active. */
    private static OSMScoutClient buildClient(Path mapDir) {
        assumeNativeLibrary();

        OSMScoutClient builtClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(mapDir.toString())
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        Assumptions.assumeTrue(builtClient != null, "client already initialised");
        return builtClient;
    }

    private static OSMScoutClient buildClientOnProvidedMap() {
        Path mapDir = providedMapDir();
        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient localClient = buildClient(mapDir);
        assertTrue(localClient.openDatabase(mapDir.toString()),
            "database should open");
        return localClient;
    }

    private static String[] perAttributeQualities(LocationEntry entry) {
        return new String[]{
            entry.adminRegionMatchQuality,
            entry.postalAreaMatchQuality,
            entry.locationMatchQuality,
            entry.addressMatchQuality,
            entry.poiMatchQuality};
    }

    /**
     * Poll until the search yields at least one entry. Opening a database hands
     * the scan and load to the background DB thread, so the first searches of a
     * freshly opened database run against a database that is not loaded yet and
     * legitimately come back empty.
     */
    private static LocationEntry[] waitForSearchResults(OSMScoutClient client, String query, int limit)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + 30000;

        LocationEntry[] entries = client.searchLocations(query, limit);
        while ((entries == null || entries.length == 0) && System.currentTimeMillis() < deadline) {
            Thread.sleep(50);
            entries = client.searchLocations(query, limit);
        }
        return entries;
    }

    /**
     * A client without an open database must return an empty result set, so a
     * per-attribute quality can never be observed without a database.
     */
    @Test
    public void testSearchWithoutDatabaseReturnsNoEntries(@TempDir Path emptyDir) {
        OSMScoutClient noMapClient = buildClient(emptyDir);

        // An empty directory: the scan cannot pick up a map that happens to lie
        // around (a stale database in a shared directory aborts the JVM while
        // scanning it, which is not this test's subject).
        LocationEntry[] entries = noMapClient.searchLocations("Dortmund", 20);

        assertNotNull(entries, "search must return an array");
        assertEquals(0, entries.length,
            "no database open - search must not return entries");

        noMapClient.close();
    }

    /**
     * The per-attribute match quality, the house-number flag and the matched
     * name/component of real search results against an operator-provided map.
     */
    @Test
    public void testPerAttributeMatchQualityOnProvidedMap() throws InterruptedException {
        OSMScoutClient localClient = buildClientOnProvidedMap();

        try {
            checkProvidedMapResults(localClient);
        } finally {
            // Release the singleton so a failing case does not make the other
            // database-driven case skip itself.
            localClient.close();
        }
    }

    private static void checkProvidedMapResults(OSMScoutClient localClient) throws InterruptedException {
        // The query names the admin region itself, so the entry for it must
        // report a region match - the collapsed "matchQuality" cannot show that,
        // it only says that some attribute matched.
        LocationEntry[] entries = waitForSearchResults(localClient, "Dortmund", 20);

        assertNotNull(entries, "search must return an array");
        assertTrue(entries.length > 0,
            "search for the name of the region should return entries");

        boolean sawRegionMatch = false;
        boolean sawRegionCandidate = false;
        boolean sawEntryNamedLikeTheQuery = false;

        for (LocationEntry entry : entries) {
            assertNotNull(entry.matchedName,
                "matchedName must never be null: " + entry.label);
            assertNotNull(entry.matchedComponent,
                "matchedComponent must never be null: " + entry.label);
            assertTrue(COMPONENTS.contains(entry.matchedComponent),
                "unknown matchedComponent '" + entry.matchedComponent
                    + "' for '" + entry.label + "'");

            for (String quality : perAttributeQualities(entry)) {
                assertNotNull(quality,
                    "per-attribute match quality must never be null: " + entry.label);
                assertTrue(QUALITIES.contains(quality),
                    "unknown match quality '" + quality + "' for '" + entry.label + "'");
            }

            // The entry the query names reports the region as the matching
            // component, and names the region as the component it matched with.
            if ("adminRegion".equals(entry.matchedComponent)
                && "Dortmund".equals(entry.matchedName)) {
                sawEntryNamedLikeTheQuery = true;
                assertEquals("match", entry.adminRegionMatchQuality,
                    "the query names this region, so it must report a region match: " + entry.label);
            }

            // Entries with no attribute attribution must not claim one: the
            // coordinate result of the query and a text-index hit carry "none"
            // for every component, and the app-side ranker relies on that.
            if ("coordinate".equals(entry.matchedComponent)
                || "freeText".equals(entry.matchedComponent)) {
                for (String quality : perAttributeQualities(entry)) {
                    assertEquals("none", quality,
                        "a '" + entry.matchedComponent + "' entry must not claim an attribute match"
                            + " for '" + entry.label + "'");
                }
            }

            // A house-level entry carries the house number; its matched name is
            // the street, not the composite label.
            if (entry.hasHouseNumber) {
                assertEquals("location", entry.matchedComponent,
                    "a house-level entry should attribute the match to the street: " + entry.label);
                assertFalse(entry.matchedName.isEmpty(),
                    "a house-level entry should name the street: " + entry.label);
            }

            if ("match".equals(entry.adminRegionMatchQuality)) {
                sawRegionMatch = true;
            }
            if ("candidate".equals(entry.adminRegionMatchQuality)) {
                sawRegionCandidate = true;
            }
        }

        assertTrue(sawEntryNamedLikeTheQuery,
            "the search for a region name should return the region itself");
        assertTrue(sawRegionMatch,
            "the query names the admin region, so at least one entry should report a region match");
        assertTrue(sawRegionCandidate,
            "a name that only partly matches should be reported as a candidate,"
                + " so the region quality is not a constant 'match'");
    }

    /**
     * A query that is a coordinate pair yields a coordinate result that belongs
     * to no attribute of the queried data; it must therefore claim no attribute
     * match at all.
     */
    @Test
    public void testCoordinateQueryClaimsNoAttributeMatch() throws InterruptedException {
        OSMScoutClient localClient = buildClientOnProvidedMap();

        try {
            checkCoordinateQuery(localClient);
        } finally {
            localClient.close();
        }
    }

    private static void checkCoordinateQuery(OSMScoutClient localClient) throws InterruptedException {
        LocationEntry[] entries = waitForSearchResults(localClient, "51.514, 7.465", 20);

        assertNotNull(entries, "search must return an array");
        assertTrue(entries.length > 0, "a coordinate query should return the coordinate result");

        LocationEntry coordinateEntry = null;
        for (LocationEntry entry : entries) {
            if ("coordinate".equals(entry.matchedComponent)) {
                coordinateEntry = entry;
                break;
            }
        }

        assertNotNull(coordinateEntry, "a coordinate query should yield a coordinate entry");
        assertEquals("51.514, 7.465", coordinateEntry.matchedName,
            "the coordinate result is named by the query itself");
        assertFalse(coordinateEntry.hasHouseNumber,
            "a coordinate result carries no house number");
        for (String quality : perAttributeQualities(coordinateEntry)) {
            assertEquals("none", quality,
                "a coordinate result must not claim an attribute match");
        }
    }
}
