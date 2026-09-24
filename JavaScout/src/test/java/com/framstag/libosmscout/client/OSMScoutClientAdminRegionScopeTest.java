package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for the admin region search scope:
 * {@link OSMScoutClient#getAdminRegionScopeName(long)} reports the scope region
 * a handle resolved by {@link OSMScoutClient#resolveAdminRegion(double, double)}
 * uses, and a search scoped by that handle stays usable.
 * <p>
 * Skipped automatically when the native library is not available. The
 * database-driven scenario additionally requires {@code -Dsearch.test.db.dir}
 * (or the {@code JAVASCOUT_MAP_DIR} environment variable) pointing at an
 * openable .osmscout database directory — the same convention as
 * {@link SearchReproTest} and {@link OSMScoutClientSearchQualityTest}.
 */
public class OSMScoutClientAdminRegionScopeTest {

    private static final String DB_DIR_PROPERTY = "search.test.db.dir";

    /** Dortmund city centre, inside the district the scope expands from. */
    private static final double CITY_LAT = 51.514;
    private static final double CITY_LON = 7.465;

    private static final long UNKNOWN_HANDLE = 987654321L;

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

    /**
     * Poll a search until the database of a freshly opened client is loaded:
     * the scan runs on the background DB thread.
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

    /** Unknown and zero handles have no scope; neither may crash. */
    @Test
    public void testScopeNameWithoutRegion(@TempDir Path emptyDir) {
        OSMScoutClient noMapClient = buildClient(emptyDir);

        assertNull(noMapClient.getAdminRegionScopeName(0),
            "handle 0 resolves no region, so it has no scope name");
        assertNull(noMapClient.getAdminRegionScopeName(UNKNOWN_HANDLE),
            "an unknown handle has no scope name");

        noMapClient.close();
    }

    /**
     * A resolved handle reports a scope region that exists in the database, and
     * scoping a search by the handle still returns results.
     */
    @Test
    public void testScopeNameOfResolvedRegionOnProvidedMap() throws InterruptedException {
        assumeNativeLibrary();

        Path mapDir = providedMapDir();
        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient localClient = buildClient(mapDir);

        try {
            assertTrue(localClient.openDatabase(mapDir.toString()), "database should open");
            long handle = 0;
            try {
                // Wait for the database, then resolve the region of a coordinate
                // in the middle of the city.
                waitForSearchResults(localClient, "Dortmund", 20);
                handle = localClient.resolveAdminRegion(CITY_LAT, CITY_LON);
                assertTrue(handle != 0, "the coordinate should resolve to an admin region");

                String regionName = localClient.getAdminRegionName(handle);
                String scopeName = localClient.getAdminRegionScopeName(handle);
                assertNotNull(regionName, "the handle should report its region name");
                assertNotNull(scopeName, "the handle should report its scope name");
                assertFalse(scopeName.isEmpty(), "the scope name must not be empty");

                // The scope denotes a region of this database: searching for the
                // scope name finds that region (a scope that were garbage would
                // name nothing).
                boolean scopeIsARegion = false;
                for (LocationEntry entry : waitForSearchResults(localClient, scopeName, 20)) {
                    if ("boundary_administrative".equals(entry.objectType)
                        && scopeName.equals(entry.label)) {
                        scopeIsARegion = true;
                        break;
                    }
                }
                assertTrue(scopeIsARegion,
                    "the scope '" + scopeName + "' should name an admin region of the database");

                // A search scoped by the handle must stay usable. Note that the
                // scope is a preference for ranking, not a filter: entries from
                // outside the scope may still be returned, and the measured order
                // for the queries tried here is the same as without a scope, so
                // the scope's observable effect is the reported name (the caller
                // ranks results with it).
                LocationEntry[] scoped = localClient.searchLocations(regionName, 20, handle);
                assertNotNull(scoped, "a scoped search must return an array");
                assertTrue(scoped.length > 0, "a scoped search should return results");
            } finally {
                if (handle != 0) {
                    localClient.releaseAdminRegion(handle);
                }
            }
        } finally {
            localClient.close();
        }
    }
}
