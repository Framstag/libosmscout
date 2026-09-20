package com.framstag.libosmscout.client;

import java.io.IOException;
import java.io.InputStream;
import java.net.URI;
import java.net.http.HttpClient;
import java.net.http.HttpRequest;
import java.net.http.HttpResponse;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.time.LocalDateTime;
import java.time.ZoneId;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.CRC32;

/**
 * Manager for discovering, downloading, and managing the world basemap.
 * <p>
 * The basemap is a special world-wide database (borders, country names,
 * coastlines) hosted on the map provider's server at {@code {baseUri}/basemap/}.
 * It is not listed in the standard map JSON listing. It is delivered like a
 * regional database: the server publishes an availability manifest naming the
 * database format versions it offers, and each version is a directory of a
 * metadata file plus the data files that metadata names.
 * <p>
 * All HTTP operations run in Java to avoid the OpenJDK 17.0.2 JVM crash when
 * {@code java.net.http.HttpClient} methods are called from native code.
 */
public class BasemapManager {

    /** Well-known basemap path segment on the provider server. */
    public static final String BASEMAP_PATH = "basemap";

    /** Subdirectory name under the maps directory where basemap is stored. */
    public static final String BASEMAP_DIR_NAME = "basemap";

    /** The availability manifest the server publishes in the basemap area. */
    public static final String MANIFEST_FILE = "index.json";

    /** The database metadata file, which is also kept locally after a download. */
    public static final String METADATA_FILE = "db.json";

    /** The manifest schema version this client understands. */
    private static final int MANIFEST_SCHEMA = 1;

    /** The metadata schema version this client understands. */
    private static final int METADATA_SCHEMA = 1;

    /** Suffix of a file that is still being downloaded. */
    private static final String PARTIAL_SUFFIX = ".download";

    /** The map provider whose server we probe for the basemap. */
    private final MapProvider provider;

    /** Directory where downloaded maps (including basemap) are stored. */
    private final Path mapsDirectory;

    /** Active downloads, by handle, so a download can be cancelled. */
    private final Map<String, ActiveDownload> activeDownloads = new ConcurrentHashMap<>();

    /**
     * One downloadable basemap version: a database of a given database format
     * version, described by the availability manifest.
     */
    public static class BasemapVersion {

        /** The database format version of this basemap. */
        private final int typeConfigVersion;

        /** When this version last changed on the server (ISO 8601 UTC), may be null. */
        private final String changedAt;

        /**
         * @param typeConfigVersion database format version of this basemap
         * @param changedAt         when this version last changed on the server
         */
        BasemapVersion(int typeConfigVersion, String changedAt) {
            this.typeConfigVersion = typeConfigVersion;
            this.changedAt = changedAt;
        }

        /** @return the database format version of this basemap */
        public int getTypeConfigVersion() {
            return typeConfigVersion;
        }

        /** @return when this version last changed on the server, or null */
        public String getChangedAt() {
            return changedAt;
        }

        /** @return a display label for this version */
        public String getLabel() {
            return "Version " + typeConfigVersion;
        }

        @Override
        public String toString() {
            return "BasemapVersion{version=" + typeConfigVersion + ", changedAt=" + changedAt + "}";
        }
    }

    /**
     * @param provider      the map provider whose server to probe
     * @param mapsDirectory the local directory where maps (including basemap) are stored
     */
    public BasemapManager(MapProvider provider, Path mapsDirectory) {
        this.provider = provider;
        this.mapsDirectory = mapsDirectory;
    }

    /**
     * @return the base URL of the basemap area on the provider server
     */
    public String getBasemapBaseUrl() {
        return provider.getUri() + "/" + BASEMAP_PATH + "/";
    }

    /**
     * @return the URL of the basemap availability manifest
     */
    public String getBasemapManifestUrl() {
        return getBasemapBaseUrl() + MANIFEST_FILE;
    }

    /**
     * @return the local directory where the basemap is stored
     */
    public Path getBasemapDirectory() {
        return mapsDirectory.resolve(BASEMAP_DIR_NAME);
    }

    /**
     * The database format version this client can read: the same value the
     * regional map listing is requested with.
     *
     * @return the supported database format version
     */
    public static int getSupportedDatabaseFormatVersion() {
        return MapDownloadManager.DATABASE_FORMAT_VERSION;
    }

    /**
     * Probe the provider server for available basemap versions by reading the
     * availability manifest.
     * <p>
     * Only versions this client can read are reported, newest first. A missing,
     * empty, or unparsable manifest is reported as "no basemap available", not
     * as an error: the basemap is optional.
     *
     * @return list of available basemap versions, empty if none is available
     */
    public List<BasemapVersion> fetchAvailableBasemaps() {
        try {
            String urlStr = getBasemapManifestUrl();
            HttpClient client = HttpClient.newHttpClient();
            HttpRequest request = HttpRequest.newBuilder(URI.create(urlStr)).GET().build();
            HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

            if (response.statusCode() / 100 != 2) {
                System.err.println("[BasemapManager] manifest not available: HTTP " + response.statusCode());
                return List.of();
            }

            return selectReadableVersions(parseManifest(response.body()));
        } catch (IOException e) {
            System.err.println("[BasemapManager] IO error reading the basemap manifest: " + e.getMessage());
            return List.of();
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            System.err.println("[BasemapManager] Interrupted while reading the basemap manifest");
            return List.of();
        }
    }

    /**
     * The versions of a manifest this client can read, newest first.
     *
     * @param versions all versions named by the manifest
     * @return the readable versions, newest first
     */
    static List<BasemapVersion> selectReadableVersions(List<BasemapVersion> versions) {
        int supported = getSupportedDatabaseFormatVersion();
        List<BasemapVersion> readable = new ArrayList<>();

        for (BasemapVersion version : versions) {
            if (version.getTypeConfigVersion() <= supported) {
                readable.add(version);
            }
        }

        readable.sort(Comparator.comparingInt(BasemapVersion::getTypeConfigVersion).reversed());

        return readable;
    }

    /**
     * Parse an availability manifest.
     *
     * @param json the manifest content
     * @return the versions it names, empty when it is not a manifest this client understands
     */
    static List<BasemapVersion> parseManifest(String json) {
        List<BasemapVersion> versions = new ArrayList<>();

        if (json == null) {
            return versions;
        }

        Integer schema = jsonInt(json, "schema");

        if (schema == null || schema != MANIFEST_SCHEMA) {
            System.err.println("[BasemapManager] unsupported basemap manifest schema: " + schema);
            return versions;
        }

        String array = jsonArray(json, "versions");

        if (array == null) {
            return versions;
        }

        for (String entry : jsonObjects(array)) {
            Integer typeConfigVersion = jsonInt(entry, "typeConfigVersion");

            if (typeConfigVersion == null) {
                continue;
            }

            versions.add(new BasemapVersion(typeConfigVersion, jsonString(entry, "changedAt")));
        }

        return versions;
    }

    /**
     * Get the newest readable basemap version from the server.
     *
     * @return the newest version, or null if none is available
     */
    public BasemapVersion getLatestBasemap() {
        List<BasemapVersion> available = fetchAvailableBasemaps();
        return available.isEmpty() ? null : available.get(0);
    }

    /**
     * Information about an installed basemap.
     */
    public static class BasemapInfo {
        /** Total size of all files in bytes. */
        private final long sizeBytes;
        /** Number of files in the basemap directory. */
        private final int fileCount;
        /** Last modified time of the basemap directory. */
        private final LocalDateTime lastModified;
        /** Absolute path to the basemap directory. */
        private final String path;
        /** Database format version of the installed basemap. */
        private final int typeConfigVersion;
        /** When the installed basemap was generated on the server, may be null. */
        private final String changedAt;

        /**
         * @param sizeBytes         total size of all files in bytes
         * @param fileCount         number of files in the basemap directory
         * @param lastModified      last modified time of the basemap directory
         * @param path              absolute path to the basemap directory
         * @param typeConfigVersion database format version of the installed basemap
         * @param changedAt         generation time recorded in the installed metadata
         */
        BasemapInfo(long sizeBytes, int fileCount, LocalDateTime lastModified, String path,
                    int typeConfigVersion, String changedAt) {
            this.sizeBytes = sizeBytes;
            this.fileCount = fileCount;
            this.lastModified = lastModified;
            this.path = path;
            this.typeConfigVersion = typeConfigVersion;
            this.changedAt = changedAt;
        }

        /** @return total size of all files in bytes */
        public long getSizeBytes() { return sizeBytes; }
        /** @return number of files in the basemap directory */
        public int getFileCount() { return fileCount; }
        /** @return last modified time of the basemap directory */
        public LocalDateTime getLastModified() { return lastModified; }
        /** @return absolute path to the basemap directory */
        public String getPath() { return path; }
        /** @return database format version of the installed basemap */
        public int getTypeConfigVersion() { return typeConfigVersion; }
        /** @return generation time recorded in the installed metadata, or null */
        public String getChangedAt() { return changedAt; }

        /** @return human-readable size string */
        public String getSizeHuman() {
            if (sizeBytes < 1024) return sizeBytes + " B";
            if (sizeBytes < 1024 * 1024) return String.format(java.util.Locale.US, "%.0f KB", sizeBytes / 1024.0);
            if (sizeBytes < 1024 * 1024 * 1024) return String.format(java.util.Locale.US, "%.1f MB", sizeBytes / (1024.0 * 1024.0));
            return String.format(java.util.Locale.US, "%.2f GB", sizeBytes / (1024.0 * 1024.0 * 1024.0));
        }

        @Override
        public String toString() {
            return "BasemapInfo{version=" + typeConfigVersion + ", size=" + getSizeHuman() + ", files=" + fileCount + "}";
        }
    }

    /**
     * Get information about the installed basemap, including the version and
     * generation time recorded in the metadata that was downloaded with it.
     *
     * @return basemap info, or null if not installed or without metadata
     */
    public BasemapInfo getInstalledBasemapInfo() {
        Path basemapDir = getBasemapDirectory();

        if (!Files.isDirectory(basemapDir)) {
            return null;
        }

        Path metadataPath = basemapDir.resolve(METADATA_FILE);

        if (!Files.isRegularFile(metadataPath)) {
            return null;
        }

        try {
            String metadata = Files.readString(metadataPath);
            Integer typeConfigVersion = jsonInt(metadata, "typeConfigVersion");
            Integer schema = jsonInt(metadata, "schema");

            if (typeConfigVersion == null || schema == null || schema != METADATA_SCHEMA) {
                return null;
            }

            long size = Files.walk(basemapDir)
                .filter(Files::isRegularFile)
                .mapToLong(p -> {
                    try { return Files.size(p); }
                    catch (IOException e) { return 0; }
                })
                .sum();

            int fileCount = (int) Files.walk(basemapDir)
                .filter(Files::isRegularFile)
                .count();

            LocalDateTime modified = LocalDateTime.ofInstant(
                Files.getLastModifiedTime(basemapDir).toInstant(),
                ZoneId.systemDefault());

            return new BasemapInfo(size, fileCount, modified, basemapDir.toAbsolutePath().toString(),
                                   typeConfigVersion, jsonString(metadata, "generatedAt"));
        } catch (IOException e) {
            System.err.println("[BasemapManager] cannot read the installed basemap metadata: " + e.getMessage());
            return null;
        }
    }

    /**
     * Check if a basemap is installed locally.
     *
     * @return true if the basemap directory exists and carries a database
     */
    public boolean isBasemapInstalled() {
        Path basemapDir = getBasemapDirectory();

        if (!Files.isDirectory(basemapDir)) {
            return false;
        }

        return Files.isRegularFile(basemapDir.resolve("types.dat"));
    }

    /**
     * Get the size of the installed basemap directory.
     *
     * @return size in bytes, or 0 if not installed
     */
    public long getInstalledBasemapSize() {
        Path basemapDir = getBasemapDirectory();
        if (!Files.isDirectory(basemapDir)) {
            return 0;
        }
        try {
            return Files.walk(basemapDir)
                .filter(Files::isRegularFile)
                .mapToLong(p -> {
                    try { return Files.size(p); }
                    catch (IOException e) { return 0; }
                })
                .sum();
        } catch (IOException e) {
            return 0;
        }
    }

    /**
     * Download a basemap version into the basemap directory.
     * <p>
     * Downloads the metadata of the version, then every data file it names,
     * verifies each file against the checksum in that metadata, and only then
     * replaces the installed basemap. A failed download leaves the previously
     * installed basemap untouched.
     *
     * @param version  the basemap version to download
     * @param listener callback for progress and completion events
     * @return a handle that can be used to cancel the download
     */
    public String downloadBasemap(BasemapVersion version, MapDownloadListener listener) {
        String handle = java.util.UUID.randomUUID().toString();
        String mapName = "World Basemap";

        ActiveDownload active = new ActiveDownload(handle, mapName, listener);
        activeDownloads.put(handle, active);

        HttpClient client = HttpClient.newHttpClient();

        Thread worker = new Thread(() -> {
            try {
                downloadVersion(active, version, client);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                listener.onError(mapName, "Download cancelled");
            } catch (Exception e) {
                listener.onError(mapName, e.getMessage());
            } finally {
                activeDownloads.remove(handle);
            }
        }, "basemap-download");

        active.worker = worker;
        worker.setDaemon(true);
        worker.start();

        return handle;
    }

    /**
     * Download the metadata and the data files of one basemap version, verify
     * them, and replace the installed basemap.
     */
    private void downloadVersion(ActiveDownload active,
                                 BasemapVersion version,
                                 HttpClient client) throws Exception {
        String mapName = active.mapName;
        String base = getBasemapBaseUrl() + "v" + version.getTypeConfigVersion() + "/";
        Path finalDir = getBasemapDirectory();
        Path tempDir = mapsDirectory.resolve("." + BASEMAP_DIR_NAME + "-" + active.handle);

        deleteDirectory(tempDir);
        Files.createDirectories(tempDir);

        try {
            listener(active).onProgress(mapName, 0, 1);

            String metadata = fetchText(active, client, base + METADATA_FILE);
            Integer schema = jsonInt(metadata, "schema");

            if (schema == null || schema != METADATA_SCHEMA) {
                throw new IOException("unsupported basemap metadata schema: " + schema);
            }

            Integer metadataVersion = jsonInt(metadata, "typeConfigVersion");

            if (metadataVersion == null || metadataVersion != version.getTypeConfigVersion()) {
                throw new IOException("basemap metadata names version " + metadataVersion
                                      + ", expected " + version.getTypeConfigVersion());
            }

            Map<String, long[]> files = parseFileInventory(metadata);

            if (files.isEmpty()) {
                throw new IOException("basemap metadata names no data files");
            }

            long totalBytes = 0;

            for (long[] entry : files.values()) {
                totalBytes += entry[0];
            }

            long downloadedBytes = 0;

            // types.dat last: a directory that is complete except for the type
            // configuration is not recognized as a map.
            List<String> names = new ArrayList<>(files.keySet());
            names.sort(Comparator.comparing(name -> name.equals("types.dat") ? 1 : 0));

            for (String name : names) {
                checkCancelled(active);

                long[] entry = files.get(name);
                Path target = tempDir.resolve(name);

                Files.createDirectories(target.getParent() == null ? tempDir : target.getParent());

                downloadedBytes = fetchFile(active, client, base + name, target, entry[1],
                                            downloadedBytes, totalBytes);
            }

            checkCancelled(active);

            Files.writeString(tempDir.resolve(METADATA_FILE), metadata);

            install(active, tempDir, finalDir);

            listener(active).onComplete(mapName, finalDir.toAbsolutePath().toString());
        } catch (Exception e) {
            deleteDirectory(tempDir);
            throw e;
        }
    }

    /**
     * Replace the installed basemap with the downloaded one, keeping the
     * installed one when the replacement cannot be completed.
     */
    private void install(ActiveDownload active, Path tempDir, Path finalDir) throws IOException {
        Path previousDir = mapsDirectory.resolve("." + BASEMAP_DIR_NAME + "-" + active.handle + ".previous");

        deleteDirectory(previousDir);

        boolean hadPrevious = Files.isDirectory(finalDir);

        if (hadPrevious) {
            moveDirectory(finalDir, previousDir);
        }

        try {
            moveDirectory(tempDir, finalDir);
        } catch (IOException e) {
            if (hadPrevious && !Files.isDirectory(finalDir)) {
                moveDirectory(previousDir, finalDir);
            }

            throw e;
        }

        deleteDirectory(previousDir);
    }

    /**
     * Move a directory, falling back to a copy when the file system cannot move it.
     */
    private static void moveDirectory(Path source, Path target) throws IOException {
        try {
            Files.move(source, target, StandardCopyOption.ATOMIC_MOVE);
        } catch (IOException atomicFailed) {
            try {
                Files.move(source, target);
            } catch (IOException moveFailed) {
                copyDirectory(source, target);
                deleteDirectory(source);
            }
        }
    }

    /**
     * Fetch a small text resource.
     */
    private String fetchText(ActiveDownload active,
                             HttpClient client,
                             String url) throws IOException, InterruptedException {
        HttpRequest request = HttpRequest.newBuilder(URI.create(url)).GET().build();

        checkCancelled(active);

        HttpResponse<String> response = client.send(request, HttpResponse.BodyHandlers.ofString());

        if (response.statusCode() / 100 != 2) {
            throw new IOException("Server returned HTTP " + response.statusCode() + " for " + url);
        }

        return response.body();
    }

    /**
     * Fetch one data file, verifying it against the checksum the metadata names
     * for it. Returns the number of bytes downloaded so far.
     */
    private long fetchFile(ActiveDownload active,
                           HttpClient client,
                           String url,
                           Path target,
                           long expectedCrc32,
                           long downloadedBytes,
                           long totalBytes) throws IOException, InterruptedException {
        String mapName = active.mapName;
        Path partial = target.resolveSibling(target.getFileName() + PARTIAL_SUFFIX);

        HttpRequest request = HttpRequest.newBuilder(URI.create(url)).GET().build();

        checkCancelled(active);

        HttpResponse<InputStream> response = client.send(request, HttpResponse.BodyHandlers.ofInputStream());

        if (response.statusCode() / 100 != 2) {
            throw new IOException("Server returned HTTP " + response.statusCode() + " for " + url);
        }

        CRC32 crc32 = new CRC32();
        long fileBytes = 0;

        try (InputStream in = active.track(response.body());
             java.io.OutputStream out = Files.newOutputStream(partial)) {
            byte[] buffer = new byte[8192];
            int read;

            while ((read = in.read(buffer)) >= 0) {
                checkCancelled(active);

                out.write(buffer, 0, read);
                crc32.update(buffer, 0, read);
                fileBytes += read;

                listener(active).onProgress(mapName, downloadedBytes + fileBytes, totalBytes);
            }
        }

        if (crc32.getValue() != expectedCrc32) {
            Files.deleteIfExists(partial);
            throw new IOException("basemap file " + target.getFileName() + " does not match its checksum");
        }

        Files.move(partial, target, StandardCopyOption.REPLACE_EXISTING);

        return downloadedBytes + fileBytes;
    }

    private static MapDownloadListener listener(ActiveDownload active) {
        return active.listener;
    }

    private static void checkCancelled(ActiveDownload active) throws InterruptedException {
        if (active.cancelled) {
            throw new InterruptedException("Download cancelled");
        }
    }

    /**
     * Cancel an active basemap download.
     * <p>
     * The running transfer is stopped and the partial installation is removed;
     * a basemap installed before the download is left usable.
     *
     * @param handle the handle returned by {@link #downloadBasemap}
     */
    public void cancelDownload(String handle) {
        ActiveDownload active = activeDownloads.get(handle);

        if (active == null) {
            return;
        }

        active.cancel();
    }

    /**
     * Delete the installed basemap directory.
     *
     * @return true if deleted successfully, false if not installed or error
     */
    public boolean deleteBasemap() {
        Path basemapDir = getBasemapDirectory();
        if (!Files.exists(basemapDir)) {
            return false;
        }
        return deleteDirectory(basemapDir);
    }

    /**
     * Check if a basemap update is available by comparing the metadata of the
     * installed basemap with the newest version the server offers.
     *
     * @return true if a newer basemap exists on the server
     */
    public boolean isUpdateAvailable() {
        BasemapInfo installed = getInstalledBasemapInfo();

        if (installed == null) {
            return false;
        }

        BasemapVersion latest = getLatestBasemap();

        if (latest == null) {
            return false;
        }

        return isNewer(latest.getTypeConfigVersion(), latest.getChangedAt(),
                       installed.getTypeConfigVersion(), installed.getChangedAt());
    }

    /**
     * Whether an offered version is newer than the installed one.
     *
     * @param offeredVersion      database format version the server offers
     * @param offeredChangedAt    change time the server reports, may be null
     * @param installedVersion    database format version installed locally
     * @param installedChangedAt  change time of the installed metadata, may be null
     * @return true when the offered version is newer
     */
    static boolean isNewer(int offeredVersion, String offeredChangedAt,
                           int installedVersion, String installedChangedAt) {
        if (offeredVersion != installedVersion) {
            return offeredVersion > installedVersion;
        }

        if (offeredChangedAt == null) {
            return false;
        }

        if (installedChangedAt == null) {
            return true;
        }

        // Both are ISO 8601 UTC timestamps of the same shape, so they compare
        // as text in the same order as in time.
        return offeredChangedAt.compareTo(installedChangedAt) > 0;
    }

    /**
     * The data files named by a basemap metadata file, as name to
     * {@code [size, crc32]}.
     *
     * @param metadata the metadata content
     * @return the file inventory, empty when it cannot be read
     */
    static Map<String, long[]> parseFileInventory(String metadata) {
        Map<String, long[]> files = new java.util.LinkedHashMap<>();

        String output = jsonObject(metadata, "output");

        if (output == null) {
            return files;
        }

        String inventory = jsonObject(output, "files");

        if (inventory == null) {
            return files;
        }

        Matcher matcher = FILE_ENTRY.matcher(inventory);

        while (matcher.find()) {
            String name = matcher.group(1);
            String entry = matcher.group(2);

            Integer size = jsonInt(entry, "size");
            Long crc32 = jsonLong(entry, "crc32");

            if (size == null || crc32 == null || size < 0) {
                continue;
            }

            files.put(name, new long[] {size, crc32});
        }

        return files;
    }

    /** One {@code "name": { ... }} entry of the file inventory. */
    private static final Pattern FILE_ENTRY =
        Pattern.compile("\"([^\"\\\\]+)\"\\s*:\\s*\\{([^{}]*)\\}", Pattern.DOTALL);

    // --- minimal JSON access -------------------------------------------------
    //
    // The client has no JSON dependency, and the two documents it reads are
    // machine-generated with a known shape, so a small value reader is enough.
    // It skips string contents while looking for structure, so a key inside a
    // string cannot be mistaken for a key.

    /**
     * Value of a field, as text, at any depth of the document.
     *
     * @return the raw value (object or array including its braces), or null
     */
    static String jsonValue(String json, String key) {
        int start = valueStart(json, key);

        if (start < 0) {
            return null;
        }

        char first = json.charAt(start);

        if (first == '{' || first == '[') {
            int end = matchingBracket(json, start);

            return end < 0 ? null : json.substring(start, end + 1);
        }

        if (first == '"') {
            int end = closingQuote(json, start);

            return end < 0 ? null : json.substring(start, end + 1);
        }

        int end = start;

        while (end < json.length() && ",}\n\r".indexOf(json.charAt(end)) < 0) {
            end++;
        }

        return json.substring(start, end).trim();
    }

    /**
     * Value of a field as an object, without the braces.
     */
    static String jsonObject(String json, String key) {
        String value = jsonValue(json, key);

        if (value == null || !value.startsWith("{") || !value.endsWith("}")) {
            return null;
        }

        return value.substring(1, value.length() - 1);
    }

    /**
     * Value of a field as an array, without the brackets.
     */
    static String jsonArray(String json, String key) {
        String value = jsonValue(json, key);

        if (value == null || !value.startsWith("[") || !value.endsWith("]")) {
            return null;
        }

        return value.substring(1, value.length() - 1);
    }

    /**
     * Value of a field as text, with the quotes removed and escapes resolved.
     */
    static String jsonString(String json, String key) {
        String value = jsonValue(json, key);

        if (value == null || value.length() < 2 || !value.startsWith("\"")) {
            return null;
        }

        return unescape(value.substring(1, value.length() - 1));
    }

    /**
     * Value of a field as an integer, or null when it is not a number.
     */
    static Integer jsonInt(String json, String key) {
        Long value = jsonLong(json, key);

        if (value == null || value < Integer.MIN_VALUE || value > Integer.MAX_VALUE) {
            return null;
        }

        return value.intValue();
    }

    /**
     * Value of a field as a whole number, or null when it is not one. CRC-32
     * values are unsigned 32 bit numbers and do not fit into an int.
     */
    static Long jsonLong(String json, String key) {
        String value = jsonValue(json, key);

        if (value == null) {
            return null;
        }

        try {
            return Long.parseLong(value.trim());
        } catch (NumberFormatException e) {
            return null;
        }
    }

    /**
     * The top-level objects of an array of objects.
     */
    static List<String> jsonObjects(String array) {
        List<String> objects = new ArrayList<>();
        int index = 0;

        while (index < array.length()) {
            int open = array.indexOf('{', index);

            if (open < 0) {
                break;
            }

            int close = matchingBracket(array, open);

            if (close < 0) {
                break;
            }

            objects.add(array.substring(open + 1, close));
            index = close + 1;
        }

        return objects;
    }

    /**
     * Index of the first character of a field's value, or -1.
     */
    private static int valueStart(String json, String key) {
        if (json == null) {
            return -1;
        }

        String needle = "\"" + key + "\"";
        int index = 0;

        while (index < json.length()) {
            int found = json.indexOf(needle, index);

            if (found < 0) {
                return -1;
            }

            // A key is preceded by a structural character, and followed by a
            // colon; anything else is a key that merely ends with this name.
            char before = found == 0 ? '{' : json.charAt(found - 1);

            if (before != '{' && before != ',' && !Character.isWhitespace(before)) {
                index = found + needle.length();
                continue;
            }

            int after = found + needle.length();

            while (after < json.length() && Character.isWhitespace(json.charAt(after))) {
                after++;
            }

            if (after >= json.length() || json.charAt(after) != ':') {
                index = found + needle.length();
                continue;
            }

            after++;

            while (after < json.length() && Character.isWhitespace(json.charAt(after))) {
                after++;
            }

            return after;
        }

        return -1;
    }

    /**
     * Index of the bracket closing the one at the given position, skipping
     * string contents.
     */
    private static int matchingBracket(String json, int open) {
        char openChar = json.charAt(open);
        char closeChar = openChar == '{' ? '}' : ']';
        int depth = 0;

        for (int i = open; i < json.length(); i++) {
            char c = json.charAt(i);

            if (c == '"') {
                int end = closingQuote(json, i);

                if (end < 0) {
                    return -1;
                }

                i = end;
                continue;
            }

            if (c == openChar) {
                depth++;
            } else if (c == closeChar) {
                depth--;

                if (depth == 0) {
                    return i;
                }
            }
        }

        return -1;
    }

    /**
     * Index of the quote closing the string that starts at the given position.
     */
    private static int closingQuote(String json, int start) {
        for (int i = start + 1; i < json.length(); i++) {
            char c = json.charAt(i);

            if (c == '\\') {
                i++;
                continue;
            }

            if (c == '"') {
                return i;
            }
        }

        return -1;
    }

    private static String unescape(String value) {
        StringBuilder result = new StringBuilder(value.length());

        for (int i = 0; i < value.length(); i++) {
            char c = value.charAt(i);

            if (c != '\\' || i + 1 >= value.length()) {
                result.append(c);
                continue;
            }

            char next = value.charAt(++i);

            switch (next) {
                case 'n': result.append('\n'); break;
                case 'r': result.append('\r'); break;
                case 't': result.append('\t'); break;
                case 'b': result.append('\b'); break;
                case 'f': result.append('\f'); break;
                case 'u':
                    if (i + 4 < value.length()) {
                        result.append((char) Integer.parseInt(value.substring(i + 1, i + 5), 16));
                        i += 4;
                    }
                    break;
                default: result.append(next);
            }
        }

        return result.toString();
    }

    /**
     * Recursively delete a directory.
     *
     * @param dir directory to delete
     * @return true if deleted successfully, false on error
     */
    private static boolean deleteDirectory(Path dir) {
        if (!Files.exists(dir)) {
            return true;
        }

        try {
            Files.walk(dir)
                .sorted(Comparator.reverseOrder())
                .forEach(p -> {
                    try { Files.deleteIfExists(p); }
                    catch (IOException ignored) {}
                });
            return true;
        } catch (IOException e) {
            return false;
        }
    }

    /**
     * Recursively copy a directory tree. Used as fallback when the file system
     * cannot move a directory.
     *
     * @param source source directory
     * @param target target directory (must not exist)
     * @throws IOException on I/O error
     */
    private static void copyDirectory(Path source, Path target) throws IOException {
        Files.createDirectories(target);
        try (java.util.stream.Stream<Path> stream = Files.walk(source)) {
            stream.forEach(sourcePath -> {
                try {
                    Path relative = source.relativize(sourcePath);
                    Path targetPath = target.resolve(relative);
                    if (Files.isDirectory(sourcePath)) {
                        Files.createDirectories(targetPath);
                    } else {
                        Files.copy(sourcePath, targetPath, StandardCopyOption.REPLACE_EXISTING);
                    }
                } catch (IOException e) {
                    throw new java.io.UncheckedIOException(e);
                }
            });
        }
    }

    /**
     * One running download, so that it can be cancelled.
     */
    private static class ActiveDownload {
        private final String handle;
        private final String mapName;
        private final MapDownloadListener listener;
        private volatile Thread worker;
        private volatile InputStream stream;
        private volatile boolean cancelled;

        ActiveDownload(String handle, String mapName, MapDownloadListener listener) {
            this.handle = handle;
            this.mapName = mapName;
            this.listener = listener;
        }

        /**
         * Register the stream of the current transfer, so cancelling closes it.
         */
        synchronized InputStream track(InputStream in) {
            stream = in;

            return in;
        }

        /**
         * Stop the transfer of this download.
         */
        void cancel() {
            cancelled = true;

            InputStream in;

            synchronized (this) {
                in = stream;
                stream = null;
            }

            if (in != null) {
                try {
                    in.close();
                } catch (IOException ignored) {
                    // the download is being cancelled, a failing close is noise
                }
            }

            Thread current = worker;

            if (current != null) {
                current.interrupt();
            }
        }
    }
}
