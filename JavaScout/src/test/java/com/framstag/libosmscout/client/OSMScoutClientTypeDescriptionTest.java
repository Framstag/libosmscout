package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeAll;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for the description language of the results.
 * <p>
 * A search result carries {@code objectTypeDescription}: the description of its object type in the
 * language the client was configured with. When the database does not carry that language, the
 * English description answers; when it carries no description for the type at all, the description
 * is empty and a caller shows {@code objectType} instead.
 * <p>
 * Skipped automatically when the native library is not available. The database-driven scenarios
 * additionally require {@code -Dpoi.test.db.dir} pointing at an openable .osmscout database
 * directory, like {@link OSMScoutClientPoiSearchTest}. The scenario that needs a database which
 * carries descriptions requires {@code -Ddescription.test.db.dir} and is skipped without it, since
 * a database generated before the descriptions existed - like the one committed under
 * {@code Tests/data/testregion} - carries none for the types a search returns.
 */
public class OSMScoutClientTypeDescriptionTest {

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

    /** A client over the database named by the test property, or null when it is not set. */
    private static OSMScoutClient openDatabaseClient(String descriptionLanguage) {
        String dbDir = System.getProperty("poi.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "poi.test.db.dir not set - skipping database-driven scenario");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withDescriptionLanguage(descriptionLanguage)
            .build();

        Assumptions.assumeTrue(dbClient != null, "could not build client");
        Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");

        return dbClient;
    }

    private static PoiEntry[] hotels(OSMScoutClient dbClient) {
        return dbClient.searchPOIs(PoiCategories.HOTELS, 52.0, 8.0, 20000, 50);
    }

    @Test
    public void testTheDescriptionLanguageCanBeSetOnARunningClient() {
        // Setting the language must be accepted by a client that has no database yet;
        // the search reports no result instead of failing.
        client.setDescriptionLanguage("de");

        PoiEntry[] results = client.searchPOIs(PoiCategories.HOTELS, 52.0, 8.0, 1000, 10);

        assertNotNull(results);
        assertEquals(0, results.length);

        client.setDescriptionLanguage("en");
    }

    @Test
    public void testResultsCarryADescriptionField() {
        OSMScoutClient dbClient = openDatabaseClient("en");

        PoiEntry[] results = hotels(dbClient);

        assertNotNull(results);

        for (PoiEntry entry : results) {
            assertNotNull(entry.objectType);
            assertNotNull(entry.objectTypeDescription,
                "the description of a result must never be null, it is empty when the database "
                + "carries none");

            if (!entry.objectTypeDescription.isEmpty()) {
                // The description names the object as a user knows it, so it is never the
                // technical type name the caller falls back to.
                assertNotEquals(entry.objectType, entry.objectTypeDescription,
                    "the description must not restate the type identifier");
            }
        }

        dbClient.close();
    }

    @Test
    public void testEnglishAnswersALanguageTheDatabaseDoesNotCarry() {
        // No shipped configuration carries a French description, so a search asking for
        // French must answer exactly like a search asking for English: with the English
        // description, or with an empty one where the database describes the type not at all.
        OSMScoutClient frenchClient = openDatabaseClient("fr");
        PoiEntry[] french = hotels(frenchClient);
        frenchClient.close();

        OSMScoutClient englishClient = openDatabaseClient("en");
        PoiEntry[] english = hotels(englishClient);
        englishClient.close();

        assertNotNull(french);
        assertNotNull(english);
        assertEquals(english.length, french.length,
            "the description language must not change which results are returned");

        for (int i = 0; i < english.length; i++) {
            assertEquals(english[i].objectType, french[i].objectType);
            assertNotNull(french[i].objectTypeDescription);
            assertEquals(english[i].objectTypeDescription, french[i].objectTypeDescription,
                "an absent language is answered by the English description");
        }
    }

    @Test
    public void testTheLanguageCanBeChangedBetweenSearches() {
        OSMScoutClient dbClient = openDatabaseClient("en");

        PoiEntry[] english = hotels(dbClient);

        dbClient.setDescriptionLanguage("de");

        PoiEntry[] german = hotels(dbClient);

        assertNotNull(english);
        assertNotNull(german);
        assertEquals(english.length, german.length);

        for (int i = 0; i < english.length; i++) {
            assertNotNull(german[i].objectTypeDescription);
        }

        dbClient.close();
    }

    @Test
    public void testDescribedTypesReportTheirDescription() {
        String dbDir = System.getProperty("description.test.db.dir");
        Assumptions.assumeTrue(dbDir != null && !dbDir.isEmpty(),
            "description.test.db.dir not set - skipping the scenario that needs descriptions");

        OSMScoutClient dbClient = new OSMScoutClientBuilder()
            .withMapLookupDirectories(dbDir)
            .withStyleSheetDirectory("../stylesheets")
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .withDescriptionLanguage("en")
            .build();

        Assumptions.assumeTrue(dbClient != null, "could not build client");
        Assumptions.assumeTrue(dbClient.openDatabase(dbDir), "could not open database");

        PoiEntry[] english = hotels(dbClient);

        assertNotNull(english);
        Assumptions.assumeTrue(english.length > 0,
            "the database holds no hotel around the search center");

        dbClient.setDescriptionLanguage("de");

        PoiEntry[] german = hotels(dbClient);

        assertNotNull(german);
        assertEquals(english.length, german.length);

        int described = 0;

        for (int i = 0; i < english.length; i++) {
            String description = english[i].objectTypeDescription;

            assertNotNull(description);
            assertNotEquals(english[i].objectType, description,
                "the description must not restate the type identifier");
            assertNotNull(german[i].objectTypeDescription);

            if (!description.isEmpty()) {
                described++;
                assertEquals(german[i].objectType, english[i].objectType);
                assertNotEquals(english[i].objectType, german[i].objectTypeDescription,
                    "the description must not restate the type identifier");
            }
        }

        assertTrue(described > 0,
            "a database generated after the descriptions existed must report a description for "
            + "the object type of a hotel");

        dbClient.close();
    }
}
