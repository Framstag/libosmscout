package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#setNativeDataCacheBudget(long)} and
 * {@link OSMScoutClient#getNativeDataCacheUsage()}.
 * <p>
 * Skipped automatically when the native library is not available. The
 * database-driven scenario additionally requires {@code -Dsearch.test.db.dir}
 * (or the {@code JAVASCOUT_MAP_DIR} environment variable) pointing at an
 * openable .osmscout database directory — the same convention as
 * {@link OSMScoutClientDataCacheSizeTest}.
 */
public class OSMScoutClientDataCacheBudgetTest {

    private static final String DB_DIR_PROPERTY = "search.test.db.dir";

    private static final int RENDER_WIDTH = 64;
    private static final int RENDER_HEIGHT = 48;
    private static final int RENDER_ZOOM = 15;

    /**
     * Positions the scenarios try to render: the first one that yields map data is
     * used, so the tests work both with the map of the other JNI tests (Dortmund)
     * and with the map database bundled with the C++ tests (kokorinsko).
     */
    private static final double[][] RENDER_POSITIONS = {
        {51.514, 7.465},
        {50.42, 14.57},
    };

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
     * Poll a render until the database of a freshly opened client is loaded: the
     * scan runs on the background DB thread, and a client without map data renders
     * nothing.
     */
    private static void waitForLoadedDatabase(OSMScoutClient client) throws InterruptedException {
        long deadline = System.currentTimeMillis() + 30000;

        int[] pixels = render(client);
        while (pixels == null && System.currentTimeMillis() < deadline) {
            Thread.sleep(50);
            pixels = render(client);
        }

        assertNotNull(pixels, "the database should be loaded after the wait");
    }

    /**
     * Render at the first position that yields map data, and return the pixel data
     */
    private static int[] render(OSMScoutClient client) {
        for (double[] position : RENDER_POSITIONS) {
            int[] pixels = client.render(RENDER_WIDTH, RENDER_HEIGHT,
                position[0], position[1], 0.0, Math.pow(2, RENDER_ZOOM));
            if (pixels != null) {
                return pixels;
            }
        }

        return null;
    }

    private static void assertRendered(int[] pixels) {
        assertNotNull(pixels, "render must return pixel data");
        assertEquals(RENDER_WIDTH * RENDER_HEIGHT, pixels.length,
            "render must return a bitmap of the requested size");
    }

    /**
     * The budget may be set on a client without any database, in every accepted
     * form (a positive value, zero and a negative value), and the reported usage
     * of a client without open databases stays at zero.
     */
    @Test
    public void testBudgetWithoutDatabase(@TempDir Path emptyDir) {
        OSMScoutClient noMapClient = buildClient(emptyDir);

        noMapClient.setNativeDataCacheBudget(64L * 1024 * 1024);
        noMapClient.setNativeDataCacheBudget(0);
        noMapClient.setNativeDataCacheBudget(-1);

        assertEquals(0, noMapClient.getNativeDataCacheUsage(),
            "a client without a database holds no cached map data");

        assertNull(render(noMapClient),
            "without a database there is no map data to render");

        noMapClient.close();
    }

    /**
     * A configured budget bounds the caches of a loaded database: the reported
     * usage stays within the budget, and rendering over the database stays
     * usable, before and after the budget is set.
     */
    @Test
    public void testBudgetBoundsTheCachesOfALoadedDatabase() throws InterruptedException {
        assumeNativeLibrary();

        Path mapDir = providedMapDir();
        Assumptions.assumeTrue(mapDir != null,
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient localClient = buildClient(mapDir);

        try {
            assertTrue(localClient.openDatabase(mapDir.toString()), "database should open");
            waitForLoadedDatabase(localClient);

            // A budget far below the data of a view: the caches are bounded, so the
            // reported usage stays within it and the render still produces a bitmap.
            localClient.setNativeDataCacheBudget(4L * 1024 * 1024);
            assertRendered(render(localClient));

            assertTrue(localClient.getNativeDataCacheUsage() <= 4L * 1024 * 1024,
                "the caches of a bounded client must stay within its budget");

            // A budget far above the data of a view keeps the caches as they are.
            localClient.setNativeDataCacheBudget(512L * 1024 * 1024);
            assertRendered(render(localClient));

            long generousUsage = localClient.getNativeDataCacheUsage();

            assertTrue(generousUsage > 0,
                "a client that rendered over a database holds cached map data");
            assertTrue(generousUsage <= 512L * 1024 * 1024,
                "the caches must stay within the budget");

            // A non-positive value restores the default budget of the client.
            localClient.setNativeDataCacheBudget(0);
            assertRendered(render(localClient));
        } finally {
            localClient.close();
        }
    }
}
