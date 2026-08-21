package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import java.nio.file.Path;
import java.nio.file.Paths;

import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Reproduction test for the garbage free-text search entries.
 * Opens a map and runs searchLocations twice, dumping every result field.
 */
class SearchReproTest {

    private static Path defaultMapDir() {
        String env = System.getenv("TESTS_MAP_DIR");
        if (env != null && !env.isEmpty()) {
            return Paths.get(env);
        }
        return Paths.get("../maps/nordrhein-westfalen").toAbsolutePath().normalize();
    }

    @Test
    public void searchTwiceAndDump() {
        Path mapDir = defaultMapDir();
        org.junit.jupiter.api.Assumptions.assumeTrue(mapDir.toFile().isDirectory(),
            "Map database not found at " + mapDir);

        OSMScoutClient client = null;
        try {
            OSMScoutClientBuilder builder = new OSMScoutClientBuilder()
                .withMapLookupDirectories(mapDir.toString())
                .withStyleSheetDirectory("../stylesheets")
                .withPhysicalDpi(96.0)
                .withUnits("metrics")
                .withCustomPoiType("_route_start")
                .withCustomPoiType("_route_end");
            client = builder.build();
        } catch (UnsatisfiedLinkError e) {
            org.junit.jupiter.api.Assumptions.abort("native library not available: " + e.getMessage());
        }
        org.junit.jupiter.api.Assumptions.assumeTrue(client != null, "client already initialised");

        assertTrue(client.openDatabase(mapDir.toString()), "database should open");

        for (String query : new String[]{"Aldi Eving", "Java Eving"}) {
            LocationEntry[] results = client.searchLocations(query, 50, "Nordrhein-Westfalen", false);
            org.junit.jupiter.api.Assumptions.assumeTrue(results != null, "null results");
            for (LocationEntry e : results) {
                // Regression: free-text padding used to produce empty entries
                // with (0,0) coordinates and no label.
                assertTrue(e.label != null && !e.label.isEmpty(),
                    "entry must have a label for query '" + query + "'");
                assertTrue(e.lat != 0.0 || e.lon != 0.0,
                    "entry must not have (0,0) coordinates for query '" + query + "'");
            }
        }
    }
}
