package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#setNativeDataCacheSize(int)}.
 * <p>
 * Skipped automatically when the native library is not available. The
 * database-driven scenario additionally requires {@code -Dsearch.test.db.dir}
 * (or the {@code JAVASCOUT_MAP_DIR} environment variable) pointing at an
 * openable .osmscout database directory — the same convention as
 * {@link SearchReproTest} and {@link OSMScoutClientSearchQualityTest}.
 */
public class OSMScoutClientDataCacheSizeTest {

    private static final String DB_DIR_PROPERTY = "search.test.db.dir";

    private static final int RENDER_WIDTH = 64;
    private static final int RENDER_HEIGHT = 48;
    private static final double RENDER_LAT = 51.514;
    private static final double RENDER_LON = 7.465;
    private static final int RENDER_ZOOM = 15;

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
    private static void waitForLoadedDatabase(OSMScoutClient client) throws InterruptedException {
        long deadline = System.currentTimeMillis() + 30000;

        LocationEntry[] entries = client.searchLocations("Dortmund", 20);
        while ((entries == null || entries.length == 0) && System.currentTimeMillis() < deadline) {
            Thread.sleep(50);
            entries = client.searchLocations("Dortmund", 20);
        }
        assertNotNull(entries, "search must return an array");
        assertTrue(entries.length > 0, "the database should be loaded after the wait");
    }

    private static int[] render(OSMScoutClient client) {
        return client.render(RENDER_WIDTH, RENDER_HEIGHT, RENDER_LAT, RENDER_LON, 0.0, Math.pow(2, RENDER_ZOOM));
    }

    private static void assertRendered(int[] pixels) {
        assertNotNull(pixels, "render must return pixel data");
        assertEquals(RENDER_WIDTH * RENDER_HEIGHT, pixels.length,
            "render must return a bitmap of the requested size");
    }

    /**
     * The value may be set on a client without any database, before a database
     * exists, and every accepted form (a positive value, zero and a negative
     * value) must leave the client usable.
     */
    @Test
    public void testSetCacheSizeWithoutDatabase(@TempDir Path emptyDir) {
        OSMScoutClient noMapClient = buildClient(emptyDir);

        noMapClient.setNativeDataCacheSize(1024);
        noMapClient.setNativeDataCacheSize(0);
        noMapClient.setNativeDataCacheSize(-1);

        assertNull(render(noMapClient),
            "without a database there is no map data to render");

        noMapClient.close();
    }

    /**
     * A configured cache size must survive a render over a loaded database
     * (where it is applied to that database's map service) and may be reset to
     * the library default at runtime.
     */
    @Test
    public void testSetCacheSizeWithDatabase() throws InterruptedException {
        assumeNativeLibrary();

        Path mapDir = providedMapDir();
        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient localClient = buildClient(mapDir);

        try {
            // Configure before the database exists, then open and load it.
            localClient.setNativeDataCacheSize(32);
            assertTrue(localClient.openDatabase(mapDir.toString()), "database should open");
            waitForLoadedDatabase(localClient);

            // The render applies the configured capacity to the map service of
            // the loaded database and must produce the requested bitmap.
            assertRendered(render(localClient));

            // Reset to the library default: rendering stays usable.
            localClient.setNativeDataCacheSize(0);

            assertRendered(render(localClient));
        } finally {
            localClient.close();
        }
    }
}
