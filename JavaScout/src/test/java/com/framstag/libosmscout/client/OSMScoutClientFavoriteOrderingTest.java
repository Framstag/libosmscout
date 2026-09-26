package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Assumptions;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * JNI integration tests for the favorite ordering operations
 * ({@link OSMScoutClient#moveGroup(String, int)},
 * {@link OSMScoutClient#moveFavoriteToGroup(String, String, String, int)},
 * {@link OSMScoutClient#moveStarredFavorite(String, String, int)},
 * {@link OSMScoutClient#getStarredFavorites()}) and for the favorites file
 * version state ({@link OSMScoutClient#getFavoriteFileFormatVersion()},
 * {@link OSMScoutClient#isFavoriteFileFormatSupported()}).
 * <p>
 * These tests are skipped automatically when the native library is not
 * available (e.g. when running {@code mvn test} without
 * {@code -Dnative.lib.dir}).
 */
public class OSMScoutClientFavoriteOrderingTest {

    @TempDir
    Path tempDir;

    private OSMScoutClient client;
    private String filePath;

    @BeforeEach
    public void setUp() {
        try {
            client = buildClient();
        } catch (UnsatisfiedLinkError | NoClassDefFoundError e) {
            Assumptions.assumeTrue(false,
                "Native library not available: " + e.getMessage());
        }

        assertNotNull(client, "client must be built");
        filePath = tempDir.resolve("favorites.json").toString();
    }

    @AfterEach
    public void tearDown() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    private static OSMScoutClient buildClient() {
        return new OSMScoutClientBuilder()
            .withPhysicalDpi(96.0)
            .withUnits("metrics")
            .build();
    }

    private static String groupNames(OSMScoutClient client) {
        StringBuilder result = new StringBuilder();

        for (FavoriteLocationGroup group : client.getFavoriteGroups()) {
            if (result.length() > 0) {
                result.append(',');
            }
            result.append(group.name);
        }

        return result.toString();
    }

    private static String favoriteNames(OSMScoutClient client, String groupName) {
        StringBuilder result = new StringBuilder();

        for (FavoriteLocationGroup group : client.getFavoriteGroups()) {
            if (!group.name.equals(groupName)) {
                continue;
            }

            for (FavoriteLocation fav : group.favorites) {
                if (result.length() > 0) {
                    result.append(',');
                }
                result.append(fav.name);
            }
        }

        return result.toString();
    }

    private static String starredNames(OSMScoutClient client) {
        StringBuilder result = new StringBuilder();

        for (StarredFavoriteLocation entry : client.getStarredFavorites()) {
            if (result.length() > 0) {
                result.append(',');
            }
            result.append(entry.groupName).append('/').append(entry.favorite.name);
        }

        return result.toString();
    }

    private void loadEmptyFile() {
        assertTrue(client.loadFavoriteLocations(filePath),
            "loading a fresh favorites file must succeed");
    }

    private void addFavorites(String groupName, String... favNames) {
        for (String favName : favNames) {
            assertTrue(client.addFavorite(groupName, favName, 51.5, 7.25),
                "addFavorite(" + groupName + ", " + favName + ")");
        }
    }

    @Test
    public void groupsAreReportedInTheStoredOrderAndCanBeMoved() {
        loadEmptyFile();

        assertTrue(client.addGroup("Work"));
        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Uni"));

        assertEquals("Work,Home,Uni", groupNames(client),
            "groups must be reported in the order they were added");

        // Move to the front
        assertTrue(client.moveGroup("Uni", 0));
        assertEquals("Uni,Work,Home", groupNames(client));

        // A negative index means the first position
        assertTrue(client.moveGroup("Home", -1));
        assertEquals("Home,Uni,Work", groupNames(client));

        // An index beyond the end is clamped to the last position
        assertTrue(client.moveGroup("Home", 100));
        assertEquals("Uni,Work,Home", groupNames(client));

        // An unknown group fails without changing the order
        assertFalse(client.moveGroup("Missing", 0));
        assertEquals("Uni,Work,Home", groupNames(client));
    }

    @Test
    public void aFavoriteCanBeMovedIntoAnotherGroup() {
        loadEmptyFile();

        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Work"));
        addFavorites("Home", "A", "B", "C");
        addFavorites("Work", "X", "Y");

        // Into the middle of the destination group
        assertTrue(client.moveFavoriteToGroup("Home", "B", "Work", 1));
        assertEquals("A,C", favoriteNames(client, "Home"));
        assertEquals("X,B,Y", favoriteNames(client, "Work"));

        // To the front of the destination group
        assertTrue(client.moveFavoriteToGroup("Home", "A", "Work", 0));
        assertEquals("C", favoriteNames(client, "Home"));
        assertEquals("A,X,B,Y", favoriteNames(client, "Work"));

        // An index beyond the end is clamped to the last position
        assertTrue(client.moveFavoriteToGroup("Home", "C", "Work", 100));
        assertEquals("", favoriteNames(client, "Home"));
        assertEquals("A,X,B,Y,C", favoriteNames(client, "Work"));

        // Moving a favorite into the group it already belongs to succeeds and
        // leaves the positions unchanged
        assertTrue(client.moveFavoriteToGroup("Work", "A", "Work", 7));
        assertEquals("A,X,B,Y,C", favoriteNames(client, "Work"));
    }

    @Test
    public void aCollidingNameRefusesTheMoveAndChangesNothing() {
        loadEmptyFile();

        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Work"));
        addFavorites("Home", "A", "B", "C");
        addFavorites("Work", "B", "X");

        assertFalse(client.moveFavoriteToGroup("Home", "B", "Work", 0),
            "the destination group already holds a favorite of that name");

        assertEquals("A,B,C", favoriteNames(client, "Home"));
        assertEquals("B,X", favoriteNames(client, "Work"));

        // Unknown source group, destination group and favorite all fail
        assertFalse(client.moveFavoriteToGroup("Missing", "A", "Work", 0));
        assertFalse(client.moveFavoriteToGroup("Home", "A", "Missing", 0));
        assertFalse(client.moveFavoriteToGroup("Home", "Missing", "Work", 0));

        assertEquals("A,B,C", favoriteNames(client, "Home"));
        assertEquals("B,X", favoriteNames(client, "Work"));
    }

    @Test
    public void starredFavoritesAreOrderedAcrossGroups() {
        loadEmptyFile();

        assertTrue(client.addGroup("Work"));
        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Sport"));
        addFavorites("Work", "Office", "Desk");
        addFavorites("Home", "Home");
        addFavorites("Sport", "Gym");

        // Starring appends at the end of the starred order
        assertTrue(client.setStarred("Work", "Office", true));
        assertTrue(client.setStarred("Home", "Home", true));
        assertTrue(client.setStarred("Sport", "Gym", true));
        assertEquals("Work/Office,Home/Home,Sport/Gym", starredNames(client));

        // Starring again keeps the existing position
        assertTrue(client.setStarred("Work", "Office", true));
        assertEquals("Work/Office,Home/Home,Sport/Gym", starredNames(client));

        // A positive index moves within the starred order
        assertTrue(client.moveStarredFavorite("Sport", "Gym", 0));
        assertEquals("Sport/Gym,Work/Office,Home/Home", starredNames(client));

        // A negative index means the first position
        assertTrue(client.moveStarredFavorite("Home", "Home", -1));
        assertEquals("Home/Home,Sport/Gym,Work/Office", starredNames(client));

        // A favorite that is not starred cannot be moved
        assertFalse(client.moveStarredFavorite("Work", "Desk", 0));
        assertEquals("Home/Home,Sport/Gym,Work/Office", starredNames(client));

        // Unknown group and unknown favorite fail
        assertFalse(client.moveStarredFavorite("Missing", "Gym", 0));
        assertFalse(client.moveStarredFavorite("Sport", "Missing", 0));
        assertEquals("Home/Home,Sport/Gym,Work/Office", starredNames(client));

        // Unstarring removes the entry and starring again appends at the end
        assertTrue(client.setStarred("Home", "Home", false));
        assertEquals("Sport/Gym,Work/Office", starredNames(client));
        assertTrue(client.setStarred("Home", "Home", true));
        assertEquals("Sport/Gym,Work/Office,Home/Home", starredNames(client));

        // Only starred favorites appear
        assertFalse(client.isStarred("Work", "Desk"));
    }

    @Test
    public void theArrangedOrdersReachTheFile() {
        loadEmptyFile();

        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Work"));
        addFavorites("Home", "A", "B");
        addFavorites("Work", "Office");

        assertTrue(client.moveGroup("Work", 0));
        assertTrue(client.moveFavoriteToGroup("Home", "B", "Work", 0));
        assertTrue(client.setStarred("Work", "B", true));
        assertTrue(client.setStarred("Work", "Office", true));
        assertTrue(client.moveStarredFavorite("Work", "Office", 0));

        assertEquals("Work/Office,Work/B", starredNames(client));

        // Persist, then reopen the file with a fresh client
        assertTrue(client.saveFavoriteLocations(filePath, client.getFavoriteGroups()));

        client.close();
        client = buildClient();

        assertTrue(client.loadFavoriteLocations(filePath));
        assertEquals("Work,Home", groupNames(client));
        assertEquals("A", favoriteNames(client, "Home"));
        assertEquals("B,Office", favoriteNames(client, "Work"));
        assertEquals("Work/Office,Work/B", starredNames(client));
    }

    @Test
    public void theFileVersionIsReported() throws IOException {
        loadEmptyFile();

        assertTrue(client.addGroup("Work"));
        assertTrue(client.saveFavoriteLocations(filePath, client.getFavoriteGroups()));

        assertEquals(1, client.getFavoriteFileFormatVersion());
        assertTrue(client.isFavoriteFileFormatSupported());

        // A file written by a newer client is reported and cannot be written over
        String content =
            "{\n" +
            "  \"formatVersion\": 2,\n" +
            "  \"groups\": [\n" +
            "    { \"name\": \"Work\", \"attributes\": {}, \"favorites\": [] }\n" +
            "  ]\n" +
            "}\n";
        Files.writeString(tempDir.resolve("newer.json"), content);

        client.close();
        client = buildClient();

        assertTrue(client.loadFavoriteLocations(tempDir.resolve("newer.json").toString()),
            "loading reports the store as installed");

        assertEquals(2, client.getFavoriteFileFormatVersion());
        assertFalse(client.isFavoriteFileFormatSupported());
        assertEquals(0, client.getFavoriteGroups().length, "no groups are read from it");
        assertEquals(0, client.getStarredFavorites().length);

        assertFalse(client.saveFavoriteLocations(tempDir.resolve("newer.json").toString(),
                new FavoriteLocationGroup[0]),
            "saving over a newer file must fail");

        assertEquals(content,
            Files.readString(tempDir.resolve("newer.json")),
            "the newer file must be unchanged");
    }

    @Test
    public void aMovedGroupAndAMovedFavoriteKeepTheirData() {
        loadEmptyFile();

        assertTrue(client.addGroup("Home"));
        assertTrue(client.addGroup("Work"));
        assertTrue(client.addGroup("Uni"));
        assertTrue(client.setGroupColor("Home", "FF5733"));
        assertTrue(client.addFavorite("Home", "A", 51.5, 7.25));
        assertTrue(client.addFavorite("Home", "B", 51.4, 7.2));
        assertTrue(client.addFavorite("Work", "X", 51.3, 7.1));
        assertTrue(client.setStarred("Home", "B", true));

        assertTrue(client.moveGroup("Home", 2));
        assertEquals("Work,Uni,Home", groupNames(client));

        assertTrue(client.moveFavoriteToGroup("Home", "B", "Work", 0));

        // The moved group keeps its color, the moved favorite its star and its
        // place in the starred order
        assertEquals("FF5733", client.getGroupColor("Home"));
        assertTrue(client.isStarred("Work", "B"));
        assertEquals("Work/B", starredNames(client));
        assertEquals("A", favoriteNames(client, "Home"));
        assertEquals("B,X", favoriteNames(client, "Work"));
    }

    @Test
    public void aClientWithoutALoadedStoreReportsNoVersionAndRefusesTheOperations() {
        assertEquals(-1, client.getFavoriteFileFormatVersion(),
            "no favorites file is loaded");
        assertFalse(client.isFavoriteFileFormatSupported());
        assertEquals(0, client.getStarredFavorites().length);

        assertFalse(client.moveGroup("Work", 0));
        assertFalse(client.moveFavoriteToGroup("Home", "A", "Work", 0));
        assertFalse(client.moveStarredFavorite("Home", "A", 0));
        assertFalse(client.addGroup("Work"));
    }
}
