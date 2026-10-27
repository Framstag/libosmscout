package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterAll;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashSet;
import java.util.Set;

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Regression tests for the merge of free-text results with structured results.
 * <p>
 * The merge deduplicates by object reference, so a hit the text index reports
 * and no structured result reports must reach the caller. The bridge used to
 * erase every free-text hit in that dedup pass, because the pass compared the
 * hits against the offset set the free-text loop itself had filled.
 * <p>
 * Both cases need a map database with a text index, so they only run when an
 * operator provides one via {@code -Dsearch.test.db.dir} (the convention of
 * {@code SearchReproTest} and {@code OSMScoutClientPoiSearchTest}); automatic
 * runs skip them. The client is built once for the class: the builder returns
 * the process-wide instance, so a client closed by one case would be handed to
 * the next one already closed.
 */
class OSMScoutClientFreeTextResultTest {

    private static final String DB_DIR_PROPERTY = "search.test.db.dir";

    /** A bare street name: the structured search resolves it only with a default region. */
    private static final String STREET_QUERY = "Hauptstraße";

    /** A region to scope the structured search with, so both sources report streets. */
    private static final String DEFAULT_REGION = "Nordrhein-Westfalen";

    private static OSMScoutClient client;

    @BeforeAll
    static void openClient() {
        String dir = System.getProperty(DB_DIR_PROPERTY);
        Assumptions.assumeTrue(dir != null && !dir.isEmpty(),
            DB_DIR_PROPERTY + " not set - skipping database-driven scenario");

        Path mapDir = Paths.get(dir);
        Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        try {
            client = new OSMScoutClientBuilder()
                .withMapLookupDirectories(mapDir.toString())
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .build();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.abort("native library not available: " + e.getMessage());
        }
        Assumptions.assumeTrue(client != null, "client not initialised");
        Assumptions.assumeTrue(client.openDatabase(mapDir.toString()),
            "database should open: " + mapDir);
    }

    @AfterAll
    static void closeClient() {
        if (client != null) {
            client.close();
        }
    }

    /** Count the entries whose match the bridge attributes to the text index. */
    private static int countFreeTextMatches(LocationEntry[] results) {
        int count = 0;
        for (LocationEntry e : results) {
            if ("freeText".equals(e.matchedComponent)) {
                count++;
            }
        }
        return count;
    }

    /**
     * A hit that only the text index reports is part of the result list.
     * <p>
     * Unscoped, the structured search resolves no street for a bare street name,
     * while the text index holds thousands of them, so an empty list means the
     * merge dropped the free-text part.
     */
    @Test
    public void freeTextOnlyHitIsReturned() {
        LocationEntry[] results = client.searchLocations(STREET_QUERY, 50);
        assertNotNull(results, "results must not be null");
        assertTrue(results.length > 0,
            "the text index holds streets named '" + STREET_QUERY
                + "', so the result list must not be empty");
        assertTrue(countFreeTextMatches(results) > 0,
            "at least one entry must report that the text index matched it");
    }

    /**
     * An object both sources report appears once. Scoped, the structured search
     * resolves streets of the region as well, so the merge has to drop the
     * free-text duplicates of those objects.
     * <p>
     * The assertion is a cross-source one: an object the text index reports must
     * not also be reported by the structured search. Two structured entries for
     * one object are that search's own result and out of this merge's scope.
     */
    @Test
    public void objectFoundByBothSearchesAppearsOnce() {
        LocationEntry[] results = client.searchLocations(STREET_QUERY, 50, DEFAULT_REGION);
        assertNotNull(results, "results must not be null");
        assertTrue(countFreeTextMatches(results) > 0,
            "a scoped search for a bare street name must include text-index hits");

        Set<String> structured = new HashSet<>();
        for (LocationEntry e : results) {
            if (!"freeText".equals(e.matchedComponent)) {
                structured.add(e.label + "@" + e.lat + "," + e.lon);
            }
        }

        for (LocationEntry e : results) {
            if (!"freeText".equals(e.matchedComponent)) {
                continue;
            }
            String identity = e.label + "@" + e.lat + "," + e.lon;
            assertTrue(!structured.contains(identity),
                "entry '" + identity + "' is reported by both the structured search"
                    + " and the text index, but must appear once");
        }
    }
}
