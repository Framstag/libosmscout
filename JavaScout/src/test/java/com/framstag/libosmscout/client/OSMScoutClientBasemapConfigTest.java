package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.util.function.BooleanSupplier;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * JNI integration tests for the basemap configuration API:
 * {@link OSMScoutClientBuilder#withBasemapStyleSheet(String)} and
 * {@link OSMScoutClient#setBasemapLookupDirectory(String)}.
 * <p>
 * The native layer supports a single active client, so every test closes the
 * client it built. Skipped automatically when the native library is not
 * available.
 * <p>
 * Stylesheets must be parseable to reach the basemap load path, so the tests
 * copy the repository {@code stylesheets/standard.oss} together with the
 * {@code include/} directory it references instead of writing a stub file.
 * <p>
 * The tests that have to tell "a basemap stylesheet was selected" from "the
 * selection was ignored" need a real basemap database and use the repository
 * {@code Tests/data/testregion} fixture, copied into a temporary directory so
 * that the basemap and the map are distinct databases. {@code Database::Open}
 * only reads {@code types.dat}, so the fixture is usable as a basemap.
 * <p>
 * Ordering that makes these tests deterministic: the DB thread runs its jobs in
 * FIFO order, so a {@code setBasemapLookupDirectory} call queued before a
 * blocking {@code loadStyleSheet} call has reloaded the basemap by the time the
 * latter returns.
 */
public class OSMScoutClientBasemapConfigTest {

    private static final double FIXTURE_LAT = 50.4114;
    private static final double FIXTURE_LON = 14.5286;
    private static final int FIXTURE_MAG = 15;

    private OSMScoutClient client;

    @AfterEach
    public void closeClient() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    /** Skip the test when the native library cannot be loaded. */
    private static void assumeNativeLibrary() {
        try {
            new OSMScoutClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
        }
    }

    /** Repository stylesheet directory, or skip the test when it is missing. */
    private static Path repoStylesheets() {
        Path dir = Path.of("..", "stylesheets").toAbsolutePath();
        Assumptions.assumeTrue(Files.isDirectory(dir),
            "repository stylesheets directory not found at " + dir);
        return dir;
    }

    /** Repository test fixture database, or skip the test when it is missing. */
    private static Path fixtureDir() {
        Path fixture = Path.of("..", "Tests", "data", "testregion").toAbsolutePath();
        Assumptions.assumeTrue(Files.isDirectory(fixture),
            "testregion database fixture not found at " + fixture);
        return fixture;
    }

    private static void writeStyle(Path dir, String name, String content) throws IOException {
        Files.createDirectories(dir);
        Files.writeString(dir.resolve(name), content);
    }

    /**
     * Write a parseable stylesheet named {@code name.oss} into {@code dir}: a
     * copy of the repository {@code standard.oss} plus the {@code include/}
     * directory it references through MODULE.
     */
    private static void writeParsableStyle(Path dir, String name) throws IOException {
        Path source = repoStylesheets();
        Files.createDirectories(dir);
        Files.copy(source.resolve("standard.oss"), dir.resolve(name + ".oss"),
            StandardCopyOption.REPLACE_EXISTING);
        Path include = dir.resolve("include");
        if (!Files.isDirectory(include)) {
            copyRecursively(source.resolve("include"), include);
        }
    }

    private static void copyRecursively(Path src, Path dst) throws IOException {
        if (Files.isDirectory(src)) {
            Files.createDirectories(dst);
            try (var stream = Files.list(src)) {
                for (Path child : stream.toList()) {
                    copyRecursively(child, dst.resolve(child.getFileName().toString()));
                }
            }
        } else {
            Files.copy(src, dst, StandardCopyOption.REPLACE_EXISTING);
        }
    }

    /** A private copy of the fixture database, usable as the basemap directory. */
    private static Path copyFixture() throws IOException {
        Path dst = Files.createTempDirectory("basemap-fixture");
        dst.toFile().deleteOnExit();
        copyRecursively(fixtureDir(), dst);
        return dst;
    }

    /**
     * Build a client from the given stylesheet directory, basemap stylesheet
     * name (may be null) and basemap directory (may be null).
     */
    private OSMScoutClient build(Path styleDir, String basemapStyleSheet, Path basemapDir) {
        assumeNativeLibrary();

        OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
            .withStyleSheetDirectory(styleDir.toString())
            .withPhysicalDpi(96.0)
            .withUnits("metrics");
        if (basemapStyleSheet != null) {
            builder = builder.withBasemapStyleSheet(basemapStyleSheet);
        }
        if (basemapDir != null) {
            builder = builder.withBasemapLookupDirectory(basemapDir.toString());
        }

        client = builder.build();
        assertNotNull(client, "builder.build() must create a client");
        return client;
    }

    /** Render the fixture area, waiting briefly for the database to come up. */
    private static int[] renderFixture(OSMScoutClient subject) throws InterruptedException {
        long deadline = System.currentTimeMillis() + 20000;
        int[] pixels = subject.render(64, 48, FIXTURE_LAT, FIXTURE_LON, 0.0, FIXTURE_MAG);
        while (pixels == null && System.currentTimeMillis() < deadline) {
            Thread.sleep(50);
            pixels = subject.render(64, 48, FIXTURE_LAT, FIXTURE_LON, 0.0, FIXTURE_MAG);
        }
        return pixels;
    }

    /** Poll until the condition holds, or return false after the deadline. */
    private static boolean waitFor(BooleanSupplier condition, long timeoutMs)
        throws InterruptedException {
        long deadline = System.currentTimeMillis() + timeoutMs;
        while (System.currentTimeMillis() < deadline) {
            if (condition.getAsBoolean()) {
                return true;
            }
            Thread.sleep(50);
        }
        return condition.getAsBoolean();
    }

    /**
     * Checks that can be made without a basemap database: an unusable basemap
     * stylesheet selection must never prevent client creation and must never
     * change the active map style.
     */
    private void assertMapStyleUnaffected(OSMScoutClient created) {
        assertEquals("standard.oss", created.getActiveStyleSheet(),
            "the active map style must not change");
        assertTrue(created.loadStyleSheet("standard"),
            "loading the map style must still succeed");
        assertEquals("standard.oss", created.getActiveStyleSheet());
        assertTrue(created.wasLastStyleLoadSuccessful(),
            "the map style load must still be reported as successful");
    }

    // ---- Selection cases without a basemap database ----

    @Test
    public void testNamedBasemapStyleSheetBuildsClient(@TempDir Path tmp) throws IOException {
        writeParsableStyle(tmp, "standard");
        writeParsableStyle(tmp, "basemap-render");

        assertMapStyleUnaffected(build(tmp, "basemap-render", null));
    }

    @Test
    public void testBasemapStyleSheetWithExtensionBuildsClient(@TempDir Path tmp) throws IOException {
        writeParsableStyle(tmp, "standard");
        writeParsableStyle(tmp, "basemap-render");

        assertMapStyleUnaffected(build(tmp, "basemap-render.oss", null));
    }

    @Test
    public void testNoBasemapStyleSheetBuildsClient(@TempDir Path tmp) throws IOException {
        writeParsableStyle(tmp, "standard");

        assertMapStyleUnaffected(build(tmp, null, null));
    }

    @Test
    public void testPathLikeBasemapStyleSheetIsIgnored(@TempDir Path tmp) throws IOException {
        writeParsableStyle(tmp, "standard");

        assertMapStyleUnaffected(build(tmp, "../../etc/passwd", null));
    }

    @Test
    public void testUnmatchedBasemapStyleSheetIsIgnored(@TempDir Path tmp) throws IOException {
        writeParsableStyle(tmp, "standard");
        writeParsableStyle(tmp, "basemap-render");

        assertMapStyleUnaffected(build(tmp, "missing", null));
    }

    // ---- Selection cases that need a basemap database ----

    @Test
    public void testValidBasemapStyleSheetLoads(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        writeParsableStyle(tmp, "basemap-render");
        OSMScoutClient created = build(tmp, "basemap-render", null);

        // FIFO on the DB thread: the basemap reload runs before the style load.
        created.setBasemapLookupDirectory(copyFixture().toString());
        created.loadStyleSheet("standard");

        assertTrue(created.wasLastStyleLoadSuccessful(),
            "a valid basemap stylesheet must load alongside the map style");
        assertEquals("standard.oss", created.getActiveStyleSheet(),
            "the basemap stylesheet must not become the active map style");
    }

    @Test
    public void testUnparseableBasemapStyleSheetIsReported(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        writeStyle(tmp, "basemap-bad.oss", "OSS\nthis is not valid stylesheet content\n");
        OSMScoutClient created = build(tmp, "basemap-bad", null);

        created.setBasemapLookupDirectory(copyFixture().toString());
        created.loadStyleSheet("standard");

        assertFalse(created.wasLastStyleLoadSuccessful(),
            "a basemap stylesheet that cannot be parsed must be reported");
    }

    @Test
    public void testUnmatchedBasemapStyleSheetFallsBackToMapStyle(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        OSMScoutClient created = build(tmp, "missing", null);

        created.setBasemapLookupDirectory(copyFixture().toString());
        created.loadStyleSheet("standard");

        // Had the name been resolved to a stylesheet path, the basemap load
        // would have failed; falling back to the map style succeeds instead.
        assertTrue(created.wasLastStyleLoadSuccessful(),
            "a basemap stylesheet name with no matching stylesheet counts as not set");
    }

    @Test
    public void testPathLikeBasemapStyleSheetFallsBackToMapStyle(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        OSMScoutClient created = build(tmp, "../../etc/passwd", null);

        created.setBasemapLookupDirectory(copyFixture().toString());
        created.loadStyleSheet("standard");

        assertTrue(created.wasLastStyleLoadSuccessful(),
            "a path-like basemap stylesheet name counts as not set");
    }

    @Test
    public void testBuilderBasemapDirectoryWithUnparseableStyleIsReported(@TempDir Path tmp)
        throws Exception {
        writeParsableStyle(tmp, "standard");
        writeStyle(tmp, "basemap-bad.oss", "OSS\nthis is not valid stylesheet content\n");

        // Basemap directory and basemap stylesheet are both configured at build
        // time: the initial database scan reloads the basemap asynchronously, so
        // poll for the reported failure.
        OSMScoutClient created = build(tmp, "basemap-bad", copyFixture());

        assertTrue(waitFor(() -> !created.wasLastStyleLoadSuccessful(), 20000),
            "the initial basemap load must report the unparseable stylesheet");
    }

    // ---- Runtime basemap directory ----

    @Test
    public void testSetBasemapLookupDirectoryIsPrompt(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        OSMScoutClient created = build(tmp, null, null);
        created.openDatabase(fixtureDir().toString());
        created.loadStyleSheet("standard");

        long started = System.nanoTime();
        created.setBasemapLookupDirectory(copyFixture().toString());
        long elapsedMs = (System.nanoTime() - started) / 1_000_000;

        // The reload runs on the native database thread; the call itself only
        // queues it. The bound is generous for a small fixture but far below the
        // time the world basemap this API targets needs to open and parse.
        assertTrue(elapsedMs < 2000,
            "setBasemapLookupDirectory must not wait for the reload (took " + elapsedMs + " ms)");
        assertNotNull(renderFixture(created),
            "the map must keep rendering while the basemap reloads");
    }

    @Test
    public void testSetBasemapLookupDirectoryTakesEffect(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        writeStyle(tmp, "basemap-bad.oss", "OSS\nthis is not valid stylesheet content\n");

        // No basemap at build time, so nothing loads the basemap stylesheet yet.
        OSMScoutClient created = build(tmp, "basemap-bad", null);
        assertTrue(created.loadStyleSheet("standard"),
            "the map style loads while no basemap is configured");
        assertTrue(created.wasLastStyleLoadSuccessful(),
            "no basemap style is loaded yet");

        created.setBasemapLookupDirectory(copyFixture().toString());
        assertTrue(waitFor(() -> !created.wasLastStyleLoadSuccessful(), 20000),
            "the runtime directory change must load the basemap with the configured "
                + "stylesheet, without a further Java-side call");
    }

    @Test
    public void testClearingBasemapLookupDirectoryUnloads(@TempDir Path tmp) throws Exception {
        writeParsableStyle(tmp, "standard");
        writeParsableStyle(tmp, "basemap-render");
        OSMScoutClient created = build(tmp, "basemap-render", null);
        created.openDatabase(fixtureDir().toString());
        created.loadStyleSheet("standard");

        created.setBasemapLookupDirectory(copyFixture().toString());
        created.loadStyleSheet("standard");
        assertTrue(created.wasLastStyleLoadSuccessful(),
            "the basemap loads before it is unloaded");

        created.setBasemapLookupDirectory("");
        assertTrue(created.loadStyleSheet("standard"),
            "the map style still loads after unloading the basemap");
        assertNotNull(renderFixture(created),
            "the map must keep rendering after the basemap is unloaded");
    }

    @Test
    public void testBasemapLookupDirectoryWithoutDatabaseKeepsRendering(@TempDir Path tmp)
        throws Exception {
        writeParsableStyle(tmp, "standard");
        Path empty = Files.createTempDirectory("no-basemap");
        OSMScoutClient created = build(tmp, null, null);
        created.openDatabase(fixtureDir().toString());
        created.loadStyleSheet("standard");

        created.setBasemapLookupDirectory(empty.toString());
        created.loadStyleSheet("standard");

        assertNotNull(renderFixture(created),
            "a directory without an openable basemap must not stop map rendering");
        created.setBasemapLookupDirectory(tmp.resolve("does-not-exist").toString());
        assertNotNull(renderFixture(created),
            "a missing basemap directory must not stop map rendering");
    }
}
