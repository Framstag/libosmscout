package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * JNI integration tests for runtime stylesheet switching
 * ({@link OSMScoutClient#getAvailableStyleSheets()},
 * {@link OSMScoutClient#loadStyleSheet(String)},
 * {@link OSMScoutClient#getActiveStyleSheet()}).
 * <p>
 * These tests are skipped automatically when the native library is not
 * available (e.g. when running {@code mvn test} without
 * {@code -Dnative.lib.dir}).
 */
public class OSMScoutClientStyleTest {

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

    @AfterAll
    public static void tearDown() {
        if (client != null) {
            client.close();
        }
    }

    /**
     * Close any existing client (the native layer supports a single active
     * instance) and build a fresh one with the given stylesheet directory.
     */
    private static void rebuild(String stylesheetDir) {
        if (client != null) {
            client.close();
        }
        client = new OSMScoutClientBuilder()
            .withStyleSheetDirectory(stylesheetDir)
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
        assertNotNull(client, "builder.build() must succeed after closing the previous client");
    }

    private static void writeStyles(Path dir, String... names) throws IOException {
        Files.createDirectories(dir);
        for (String name : names) {
            Files.writeString(dir.resolve(name), "OSS\n");
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
            Files.copy(src, dst);
        }
    }

    @Test
    public void testStylesheetDirectoryAndActiveDefault(@TempDir Path tmp) {
        rebuild(tmp.toString());
        assertEquals(tmp.toAbsolutePath().toString(), client.getStyleSheetDirectory(),
            "configured stylesheet directory must be reported");
        assertEquals("standard.oss", client.getActiveStyleSheet(),
            "default active stylesheet is standard.oss");
    }

    @Test
    public void testEnumeration(@TempDir Path tmp) throws IOException {
        writeStyles(tmp, "standard.oss", "cycle.oss");
        Files.writeString(tmp.resolve("notes.txt"), "not a stylesheet");

        rebuild(tmp.toString());
        assertEquals(List.of("cycle", "standard"), client.getAvailableStyleSheets(),
            "styles are the *.oss file names without extension, sorted; non-.oss files ignored");
    }

    @Test
    public void testEnumerationEmptyDirectory(@TempDir Path tmp) {
        rebuild(tmp.toString());
        assertTrue(client.getAvailableStyleSheets().isEmpty(),
            "empty stylesheet directory yields empty style list");
    }

    @Test
    public void testLoadStyleSheetSuccess(@TempDir Path tmp) throws IOException {
        writeStyles(tmp, "standard.oss", "cycle.oss");

        rebuild(tmp.toString());
        assertTrue(client.loadStyleSheet("cycle"),
            "switching to an existing style succeeds");
        assertEquals("cycle.oss", client.getActiveStyleSheet(),
            "active stylesheet file name is updated after switch");

        // Accepting the file name including the extension is tolerated.
        assertTrue(client.loadStyleSheet("standard.oss"),
            "switching with explicit .oss extension succeeds");
        assertEquals("standard.oss", client.getActiveStyleSheet());
    }

    @Test
    public void testLoadStyleSheetUnknownStyleFails(@TempDir Path tmp) throws IOException {
        writeStyles(tmp, "standard.oss");

        rebuild(tmp.toString());
        assertFalse(client.loadStyleSheet("nonexistent"),
            "switching to an unknown style fails");
        assertEquals("standard.oss", client.getActiveStyleSheet(),
            "previous style stays active after a failed switch");
    }

    @Test
    public void testLoadStyleSheetRejectsPathTraversal(@TempDir Path tmp) throws IOException {
        writeStyles(tmp, "standard.oss");

        rebuild(tmp.toString());
        assertFalse(client.loadStyleSheet("../standard"),
            "path traversal style names are rejected");
        assertEquals("standard.oss", client.getActiveStyleSheet());
    }

    @Test
    public void testStyleFlagsSurviveSwitch(@TempDir Path tmp) throws IOException {
        writeStyles(tmp, "standard.oss", "cycle.oss");

        rebuild(tmp.toString());
        client.setStyleSheetFlag("daylight", true);

        assertTrue(client.loadStyleSheet("cycle"),
            "switching a style with enabled flags succeeds");
        assertEquals("cycle.oss", client.getActiveStyleSheet(),
            "active stylesheet file name is updated after switch");
    }

    @Test
    public void testLoadStyleSheetUnloadableKeepsPreviousStyle(@TempDir Path tmp) throws Exception {
        Path realStylesheets = Path.of("..", "stylesheets").toAbsolutePath();
        Assumptions.assumeTrue(Files.isDirectory(realStylesheets),
            "repository stylesheets directory not found");
        Path testregion = Path.of("..", "Tests", "data", "testregion").toAbsolutePath();
        Assumptions.assumeTrue(Files.isDirectory(testregion),
            "testregion database fixture not found");

        // Baseline style must parse; the bad style is syntactically invalid.
        // standard.oss references MODULE "include/*.oss" files, so the include
        // directory must be copied alongside it.
        Files.copy(realStylesheets.resolve("standard.oss"), tmp.resolve("standard.oss"));
        copyRecursively(realStylesheets.resolve("include"), tmp.resolve("include"));
        Files.writeString(tmp.resolve("bad.oss"), "OSS\nthis is not valid stylesheet content\n");

        rebuild(tmp.toString());
        assertTrue(client.openDatabase(testregion.toString()), "open test region database");
        assertTrue(client.loadStyleSheet("standard"), "baseline switch to a valid style");

        // Parse errors are only reported once the DB thread has opened the
        // database (style errors are collected per DB instance). Poll until
        // the invalid stylesheet is actually loaded against the open DB.
        boolean failed = false;
        for (int i = 0; i < 50 && !failed; i++) {
            failed = !client.loadStyleSheet("bad");
            if (!failed) {
                Thread.sleep(200);
            }
        }

        assertTrue(failed,
            "switching to an unloadable stylesheet must fail once the DB is open");
        assertEquals("standard.oss", client.getActiveStyleSheet(),
            "previous style stays active after a failed switch");
    }
}
