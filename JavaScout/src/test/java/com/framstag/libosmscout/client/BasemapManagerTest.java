package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;

/**
 * Unit tests for {@link BasemapManager}: reading the availability manifest,
 * choosing the version to offer, reading the metadata of an installed basemap,
 * and reading the file inventory a download has to follow.
 */
class BasemapManagerTest {

    /** A manifest as the regeneration pass writes it. */
    private static final String SAMPLE_MANIFEST = """
        {
          "schema": 1,
          "versions": [
            {
              "typeConfigVersion": 27,
              "changedAt": "2026-09-07T10:00:00Z"
            },
            {
              "typeConfigVersion": 26,
              "changedAt": "2026-06-01T08:30:00Z"
            }
          ]
        }
        """;

    /** A metadata file as the import tool writes it. */
    private static final String SAMPLE_METADATA = """
        {
          "schema": 1,
          "typeConfigVersion": 27,
          "generatedAt": "2026-09-07T10:00:00Z",
          "source": {
            "url": "file:///config/planet_extract.osm.pbf",
            "md5": "146f59bf3b42630f89572160de6260bb"
          },
          "import": {
            "tool": "Import",
            "version": "1.1.1",
            "startStep": 0,
            "endStep": 40,
            "durationSeconds": 123.4
          },
          "output": {
            "boundingBox": {
              "minLon": -180.0,
              "minLat": -90.0,
              "maxLon": 180.0,
              "maxLat": 90.0
            },
            "files": {
              "types.dat": {
                "size": 12345678,
                "crc32": 2712847316
              },
              "water.idx": {
                "size": 4096,
                "crc32": 3735928559
              }
            }
          },
          "stats": {
            "types": 3
          }
        }
        """;

    private static BasemapManager manager() {
        return new BasemapManager(new MapProvider("test", "https://example.com", ""),
                                  Path.of("/nonexistent/path"));
    }

    // ---- the availability manifest -----------------------------------------

    @Test
    void testParseManifestReadsVersions() {
        List<BasemapManager.BasemapVersion> versions = BasemapManager.parseManifest(SAMPLE_MANIFEST);

        assertEquals(2, versions.size());
        assertEquals(27, versions.get(0).getTypeConfigVersion());
        assertEquals("2026-09-07T10:00:00Z", versions.get(0).getChangedAt());
        assertEquals(26, versions.get(1).getTypeConfigVersion());
    }

    @Test
    void testParseManifestRejectsOtherSchema() {
        assertTrue(BasemapManager.parseManifest("{\"schema\": 2, \"versions\": []}").isEmpty());
    }

    @Test
    void testParseManifestRejectsContentThatIsNotJSON() {
        assertTrue(BasemapManager.parseManifest("{ this is not json").isEmpty());
    }

    @Test
    void testParseManifestHandlesMissingContent() {
        assertTrue(BasemapManager.parseManifest(null).isEmpty());
    }

    @Test
    void testParseManifestSkipsEntriesWithoutAVersion() {
        String manifest = "{\"schema\": 1, \"versions\": [{\"changedAt\": \"2026-09-07T10:00:00Z\"}]}";

        assertTrue(BasemapManager.parseManifest(manifest).isEmpty());
    }

    @Test
    void testParseManifestWithoutVersions() {
        assertTrue(BasemapManager.parseManifest("{\"schema\": 1}").isEmpty());
    }

    @Test
    void testParseManifestIgnoresKeysInsideStrings() {
        String manifest = """
            {
              "schema": 1,
              "note": "a version entry looks like {\\"typeConfigVersion\\": 5}",
              "versions": [
                {"typeConfigVersion": 27, "changedAt": "2026-09-07T10:00:00Z"}
              ]
            }
            """;

        List<BasemapManager.BasemapVersion> versions = BasemapManager.parseManifest(manifest);

        assertEquals(1, versions.size());
        assertEquals(27, versions.get(0).getTypeConfigVersion());
    }

    // ---- version selection -------------------------------------------------

    @Test
    void testSupportedVersionIsTheOneTheListingUses() {
        assertEquals(MapDownloadManager.DATABASE_FORMAT_VERSION,
                     BasemapManager.getSupportedDatabaseFormatVersion());
    }

    @Test
    void testSelectReadableVersionsDropsNewerOnes() {
        int supported = BasemapManager.getSupportedDatabaseFormatVersion();
        List<BasemapManager.BasemapVersion> versions = List.of(
            new BasemapManager.BasemapVersion(supported + 1, "2027-01-01T00:00:00Z"),
            new BasemapManager.BasemapVersion(supported, "2026-09-07T10:00:00Z"),
            new BasemapManager.BasemapVersion(10, "2026-01-01T00:00:00Z"));

        List<BasemapManager.BasemapVersion> readable = BasemapManager.selectReadableVersions(versions);

        assertEquals(2, readable.size());
        assertEquals(supported, readable.get(0).getTypeConfigVersion());
        assertEquals(10, readable.get(1).getTypeConfigVersion());
    }

    @Test
    void testSelectReadableVersionsIsEmptyWhenNothingIsReadable() {
        int supported = BasemapManager.getSupportedDatabaseFormatVersion();
        List<BasemapManager.BasemapVersion> versions = List.of(
            new BasemapManager.BasemapVersion(supported + 1, "2027-01-01T00:00:00Z"));

        assertTrue(BasemapManager.selectReadableVersions(versions).isEmpty());
    }

    @Test
    void testSelectReadableVersionsSortsNewestFirst() {
        List<BasemapManager.BasemapVersion> versions = List.of(
            new BasemapManager.BasemapVersion(11, "2026-01-01T00:00:00Z"),
            new BasemapManager.BasemapVersion(13, "2026-03-01T00:00:00Z"),
            new BasemapManager.BasemapVersion(12, "2026-02-01T00:00:00Z"));

        List<BasemapManager.BasemapVersion> readable = BasemapManager.selectReadableVersions(versions);

        assertEquals(13, readable.get(0).getTypeConfigVersion());
        assertEquals(12, readable.get(1).getTypeConfigVersion());
        assertEquals(11, readable.get(2).getTypeConfigVersion());
    }

    @Test
    void testVersionLabel() {
        assertEquals("Version 27", new BasemapManager.BasemapVersion(27, null).getLabel());
    }

    // ---- URLs and directories ----------------------------------------------

    @Test
    void testGetBasemapDirectory() {
        BasemapManager mgr = new BasemapManager(new MapProvider("test", "https://example.com", ""),
                                                Path.of("/maps"));

        assertEquals("/maps/basemap", mgr.getBasemapDirectory().toString());
        assertEquals("https://example.com/basemap/", mgr.getBasemapBaseUrl());
        assertEquals("https://example.com/basemap/index.json", mgr.getBasemapManifestUrl());
    }

    // ---- the file inventory of a download ----------------------------------

    @Test
    void testParseFileInventory() {
        Map<String, long[]> files = BasemapManager.parseFileInventory(SAMPLE_METADATA);

        assertEquals(2, files.size());
        assertNotNull(files.get("types.dat"));
        assertEquals(12345678L, files.get("types.dat")[0]);
        assertEquals(2712847316L, files.get("types.dat")[1]);
        assertEquals(4096L, files.get("water.idx")[0]);
        assertEquals(3735928559L, files.get("water.idx")[1]);
    }

    @Test
    void testParseFileInventoryWithoutOutput() {
        assertTrue(BasemapManager.parseFileInventory("{\"schema\": 1}").isEmpty());
    }

    @Test
    void testParseFileInventoryWithoutFiles() {
        assertTrue(BasemapManager.parseFileInventory(
            "{\"schema\": 1, \"output\": {\"boundingBox\": {}}}").isEmpty());
    }

    @Test
    void testParseFileInventorySkipsEntriesWithoutChecksum() {
        String metadata = """
            {
              "schema": 1,
              "output": {
                "files": {
                  "types.dat": {"size": 10},
                  "water.idx": {"size": 20, "crc32": 30}
                }
              }
            }
            """;

        Map<String, long[]> files = BasemapManager.parseFileInventory(metadata);

        assertEquals(1, files.size());
        assertNotNull(files.get("water.idx"));
        assertNull(files.get("types.dat"));
    }

    // ---- small JSON readers -------------------------------------------------

    @Test
    void testJsonIntAndString() {
        assertEquals(27, BasemapManager.jsonInt(SAMPLE_METADATA, "typeConfigVersion"));
        assertEquals("2026-09-07T10:00:00Z", BasemapManager.jsonString(SAMPLE_METADATA, "generatedAt"));
        assertNull(BasemapManager.jsonInt(SAMPLE_METADATA, "nothingLikeThis"));
        assertNull(BasemapManager.jsonString(SAMPLE_METADATA, "typeConfigVersion"));
    }

    @Test
    void testJsonLongReadsUnsignedChecksums() {
        // CRC-32 values are unsigned 32 bit and do not fit into an int.
        Map<String, long[]> files = BasemapManager.parseFileInventory(SAMPLE_METADATA);

        assertEquals(3735928559L, files.get("water.idx")[1]);
        assertNull(BasemapManager.jsonLong(SAMPLE_METADATA, "nothingLikeThis"));
    }

    @Test
    void testJsonObjectIsScopedToTheNamedObject() {
        // "version" exists both inside the import block and (as part of a string)
        // elsewhere; the reader has to return the value of the named field only.
        assertEquals("1.1.1", BasemapManager.jsonString(
            BasemapManager.jsonObject(SAMPLE_METADATA, "import"), "version"));
        assertNull(BasemapManager.jsonObject(SAMPLE_METADATA, "nothingLikeThis"));
    }

    @Test
    void testJsonArrayOfObjects() {
        List<String> objects = BasemapManager.jsonObjects(
            BasemapManager.jsonArray(SAMPLE_MANIFEST, "versions"));

        assertEquals(2, objects.size());
        assertEquals(27, BasemapManager.jsonInt(objects.get(0), "typeConfigVersion"));
    }

    // ---- installed basemap -------------------------------------------------

    @Test
    void testIsBasemapInstalledWithoutDirectory() {
        assertFalse(manager().isBasemapInstalled());
    }

    @Test
    void testIsUpdateAvailableWithoutInstalledBasemap() {
        assertFalse(manager().isUpdateAvailable());
    }

    @Test
    void testDeleteBasemapWithoutInstallation() {
        assertFalse(manager().deleteBasemap());
    }

    @Test
    void testInstalledMetadataDrivesTheInstalledInfo() throws Exception {
        Path maps = Files.createTempDirectory("basemap-test");

        try {
            Path basemapDir = maps.resolve("basemap");
            Files.createDirectories(basemapDir);
            Files.writeString(basemapDir.resolve("types.dat"), "types");
            Files.writeString(basemapDir.resolve("db.json"), SAMPLE_METADATA);

            BasemapManager mgr = new BasemapManager(
                new MapProvider("test", "https://example.com", ""), maps);

            assertTrue(mgr.isBasemapInstalled());

            BasemapManager.BasemapInfo info = mgr.getInstalledBasemapInfo();

            assertNotNull(info);
            assertEquals(27, info.getTypeConfigVersion());
            assertEquals("2026-09-07T10:00:00Z", info.getChangedAt());
            assertEquals(2, info.getFileCount());
        } finally {
            deleteDirectory(maps);
        }
    }

    @Test
    void testGetInstalledBasemapInfoWithoutMetadata() throws Exception {
        Path maps = Files.createTempDirectory("basemap-test");

        try {
            Path basemapDir = maps.resolve("basemap");
            Files.createDirectories(basemapDir);
            Files.writeString(basemapDir.resolve("types.dat"), "types");

            BasemapManager mgr = new BasemapManager(
                new MapProvider("test", "https://example.com", ""), maps);

            // Without downloaded metadata there is no version to compare, so the
            // installation is not usable for an update check either.
            assertNull(mgr.getInstalledBasemapInfo());
            assertFalse(mgr.isUpdateAvailable());
        } finally {
            deleteDirectory(maps);
        }
    }

    // ---- update comparison --------------------------------------------------

    @Test
    void testIsNewerForANewerVersion() {
        assertTrue(BasemapManager.isNewer(27, "2026-09-07T10:00:00Z", 26, "2026-09-07T10:00:00Z"));
    }

    @Test
    void testIsNewerForAnOlderVersion() {
        assertFalse(BasemapManager.isNewer(26, "2026-09-07T10:00:00Z", 27, "2026-09-07T10:00:00Z"));
    }

    @Test
    void testIsNewerForANewerChangeTime() {
        assertTrue(BasemapManager.isNewer(27, "2026-09-08T10:00:00Z", 27, "2026-09-07T10:00:00Z"));
    }

    @Test
    void testIsNewerForTheSameChangeTime() {
        assertFalse(BasemapManager.isNewer(27, "2026-09-07T10:00:00Z", 27, "2026-09-07T10:00:00Z"));
    }

    @Test
    void testIsNewerWithoutAServerChangeTime() {
        assertFalse(BasemapManager.isNewer(27, null, 27, "2026-09-07T10:00:00Z"));
    }

    @Test
    void testIsNewerWithoutAnInstalledChangeTime() {
        assertTrue(BasemapManager.isNewer(27, "2026-09-07T10:00:00Z", 27, null));
    }

    // ---- helpers ------------------------------------------------------------

    private static void deleteDirectory(Path dir) {
        try {
            Files.walk(dir)
                .sorted(java.util.Comparator.reverseOrder())
                .forEach(p -> {
                    try { Files.deleteIfExists(p); }
                    catch (Exception ignored) {}
                });
        } catch (Exception ignored) {}
    }
}
