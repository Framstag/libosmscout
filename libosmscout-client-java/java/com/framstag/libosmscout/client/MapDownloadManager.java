package com.framstag.libosmscout.client;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CopyOnWriteArrayList;

/**
 * Manager for downloading maps from configured map providers.
 * <p>
 * Wraps native C++ {@code MapDownloadService} methods via JNI.
 * Thread-safe for concurrent access.
 */
public class MapDownloadManager {

    /** Reference to the OSMScoutClient that owns this manager. */
    private final OSMScoutClient client;
    /** List of currently active downloads. */
    private final List<ActiveDownload> activeDownloads = new CopyOnWriteArrayList<>();

    /**
     * Package-private constructor, called from OSMScoutClient.
     *
     * @param client the owning OSMScoutClient
     */
    MapDownloadManager(OSMScoutClient client) {
        this.client = client;
    }

    /**
     * Fetch the list of available maps from a provider.
     * HTTP request is done in Java to avoid JNI cross-thread issues.
     *
     * @param provider the map provider to query
     * @return list of top-level entries (directories may have children)
     */
    public List<AvailableMapEntry> fetchAvailableMaps(MapProvider provider) {
        try {
            // Build URL with FILE_FORMAT_VERSION (27)
            String urlStr = provider.getListUri()
                .replace("%1", "27")
                .replace("%2", "27")
                .replace("%3", "en");

            java.net.URI uri = java.net.URI.create(urlStr);
            java.net.http.HttpClient client = java.net.http.HttpClient.newHttpClient();
            java.net.http.HttpRequest request = java.net.http.HttpRequest.newBuilder(uri).GET().build();
            java.net.http.HttpResponse<String> response = client.send(request, java.net.http.HttpResponse.BodyHandlers.ofString());

            String json = response.body();
            return nativeParseMapList(json, provider);
        } catch (Exception e) {
            System.err.println("[MapDownloadManager] HTTP error: " + e.getMessage());
            return java.util.Collections.emptyList();
        }
    }

    /**
     * Download a map to the specified directory.
     * <p>
     * The actual HTTP download is performed in Java to avoid an OpenJDK 17.0.2
     * JVM crash when {@code java.net.http.HttpClient} methods are called from
     * native code. The C++ side is only used for directory preparation,
     * metadata writing and map registration.
     *
     * @param entry    the map entry to download
     * @param targetDir local directory to download into
     * @param listener callback for progress and completion events
     * @return a handle that can be used to cancel the download
     */
    public synchronized String downloadMap(AvailableMapEntry entry,
                                            Path targetDir,
                                            MapDownloadListener listener) {
        String handle = java.util.UUID.randomUUID().toString();
        ActiveDownload ad = new ActiveDownload(handle, entry.getName(), listener);
        activeDownloads.add(ad);

        // Create HttpClient on the calling (Java) thread. Using a thread that has
        // never been in native code avoids the OpenJDK 17.0.2 G1 barrier crash.
        java.net.http.HttpClient client = java.net.http.HttpClient.newHttpClient();

        Thread worker = new Thread(() -> {
            try {
                downloadMapInJava(ad, entry, targetDir, listener, client);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
                listener.onError(entry.getName(), "Download cancelled");
            } catch (Exception e) {
                listener.onError(entry.getName(), e.getMessage());
            } finally {
                activeDownloads.remove(ad);
            }
        }, "map-download-" + entry.getName());
        ad.setWorker(worker);
        worker.setDaemon(true);
        worker.start();

        return handle;
    }

    /**
     * Synchronous map download orchestration done entirely in Java.
     * Called on the background worker thread.
     *
     * @param entry     the map entry to download
     * @param targetDir local directory to download into
     * @param listener  callback for progress and completion events
     * @param ad        active download record (used for cancellation)
     * @param client    Java HttpClient created on a Java thread
     * @throws Exception on any error during preparation or download
     */
    private void downloadMapInJava(ActiveDownload ad,
                                    AvailableMapEntry entry,
                                    Path targetDir,
                                    MapDownloadListener listener,
                                    java.net.http.HttpClient client) throws Exception {
        String mapName = entry.getName();
        if (entry.isDirectory()) {
            throw new IllegalArgumentException("Cannot download a directory entry: " + mapName);
        }

        // Show activity immediately in the UI.
        listener.onProgress(mapName, 0, entry.getSize());

        // Prepare directory and write metadata.json from C++ side
        if (!nativePrepareMapDirectory(entry, targetDir.toString())) {
            throw new RuntimeException("Failed to prepare map directory");
        }

        String[] fileNames = nativeGetMapFileNames();
        String serverBase = entry.getProvider().getUri() + "/" + entry.getServerDirectory();
        long totalBytes = entry.getSize();
        long downloadedBytes = 0;

        for (String fileName : fileNames) {
            if (ad.isCancelled()) {
                cleanupDirectory(targetDir);
                listener.onError(mapName, "Download cancelled");
                return;
            }

            String fileUrl = serverBase + "/" + fileName;
            Path tempFile = targetDir.resolve(fileName + ".download");
            Path finalFile = targetDir.resolve(fileName);

            boolean ok = downloadSingleFile(ad, client, fileUrl, tempFile, mapName, listener,
                                            downloadedBytes, totalBytes);
            if (!ok) {
                // Clean up and report error
                cleanupDirectory(targetDir);
                listener.onError(mapName, ad.isCancelled() ? "Download cancelled" : "Failed to download " + fileName);
                return;
            }

            java.nio.file.Files.move(tempFile, finalFile,
                java.nio.file.StandardCopyOption.REPLACE_EXISTING);

            long size = 0;
            try {
                size = java.nio.file.Files.size(finalFile);
            } catch (Exception ignored) {
            }
            downloadedBytes += size;
        }

        // Register the completed directory with the native map manager
        if (!nativeRegisterMapDirectory(targetDir.toString())) {
            cleanupDirectory(targetDir);
            listener.onError(mapName, "Failed to register map directory");
            return;
        }

        listener.onComplete(mapName, targetDir.toString());
    }

    /**
     * Download a single file using Java's HttpClient.
     *
     * @param client      HttpClient created on a Java thread
     * @param url         file URL
     * @param dest        destination path (parent must exist)
     * @param mapName     map name for progress reporting
     * @param listener    progress listener
     * @param currentBase already downloaded bytes from previous files
     * @param ad          active download record (used for cancellation)
     * @param totalBytes  expected total bytes
     * @return true on success
     */
    private boolean downloadSingleFile(ActiveDownload ad,
                                        java.net.http.HttpClient client,
                                        String url,
                                        Path dest,
                                        String mapName,
                                        MapDownloadListener listener,
                                        long currentBase,
                                        long totalBytes) {
        try {
            java.net.http.HttpRequest request = java.net.http.HttpRequest.newBuilder(java.net.URI.create(url))
                .GET()
                .build();
            java.net.http.HttpResponse<java.io.InputStream> response = client.send(
                request, java.net.http.HttpResponse.BodyHandlers.ofInputStream());

            if (response.statusCode() / 100 != 2) {
                return false;
            }

            java.nio.file.Files.createDirectories(dest.getParent());

            long fileSize = 0;
            byte[] buffer = new byte[8192];
            try (java.io.InputStream in = response.body();
                 java.io.OutputStream out = java.nio.file.Files.newOutputStream(dest)) {
                ad.setCurrentStream(in);
                int read;
                while ((read = in.read(buffer)) >= 0) {
                    if (ad.isCancelled()) {
                        throw new InterruptedException("Download cancelled");
                    }
                    out.write(buffer, 0, read);
                    fileSize += read;
                    listener.onProgress(mapName, currentBase + fileSize, totalBytes);
                }
            } finally {
                ad.setCurrentStream(null);
            }

            listener.onProgress(mapName, currentBase + fileSize, totalBytes);
            return true;
        } catch (InterruptedException e) {
            Thread.currentThread().interrupt();
            System.err.println("[MapDownloadManager] downloadSingleFile cancelled for " + url);
            return false;
        } catch (Exception e) {
            if (ad.isCancelled()) {
                System.err.println("[MapDownloadManager] downloadSingleFile cancelled for " + url);
            } else {
                System.err.println("[MapDownloadManager] downloadSingleFile failed for " + url + ": " + e.getMessage());
            }
            return false;
        }
    }

    /**
     * Remove a partially downloaded directory.
     *
     * @param dir directory to clean up
     */
    private void cleanupDirectory(Path dir) {
        try {
            java.nio.file.Files.walk(dir)
                .sorted(java.util.Comparator.reverseOrder())
                .forEach(p -> {
                    try {
                        java.nio.file.Files.deleteIfExists(p);
                    } catch (Exception ignored) {
                    }
                });
        } catch (Exception e) {
            System.err.println("[MapDownloadManager] cleanup failed: " + e.getMessage());
        }
    }

    /**
     * Cancel an active download by handle.
     *
     * @param handle the handle returned by {@link #downloadMap}
     */
    public void cancelDownload(String handle) {
        for (ActiveDownload ad : activeDownloads) {
            if (ad.handle.equals(handle)) {
                ad.cancel();
                return;
            }
        }
    }

    /**
     * Get the list of installed map directories.
     *
     * @return list of directory paths
     */
    public List<String> getInstalledMaps() {
        return nativeGetInstalledMaps();
    }

    /**
     * Delete an installed map by directory path.
     *
     * @param path the directory path of the map to delete
     * @return true if deleted successfully
     */
    public boolean deleteMap(String path) {
        return nativeDeleteMap(path);
    }

    // ---- Native methods ----

    /** Native: parse JSON map list from provider.
     * @param json     JSON response from provider
     * @param provider the map provider
     * @return list of available map entries
     */
    private native List<AvailableMapEntry> nativeParseMapList(String json, MapProvider provider);

    /** Native: return the ordered list of map database file names.
     * @return file names (mandatory + optional)
     */
    private native String[] nativeGetMapFileNames();

    /** Native: prepare the target directory and write metadata.json.
     * @param entry     the map entry to download
     * @param targetDir target directory path
     * @return true on success
     */
    private native boolean nativePrepareMapDirectory(AvailableMapEntry entry,
                                                       String targetDir);

    /** Native: register a completed map directory with the map manager.
     * @param targetDir target directory path
     * @return true on success
     */
    private native boolean nativeRegisterMapDirectory(String targetDir);

    /** Native: cancel a running download.
     * @param handle cancellation handle
     */
    private native void nativeCancelDownload(String handle);

    /** Native: get list of installed map directory paths.
     * @return list of directory paths
     */
    private native List<String> nativeGetInstalledMaps();

    /** Native: delete a map by directory path.
     * @param path directory path of the map to delete
     * @return true if deleted successfully
     */
    private native boolean nativeDeleteMap(String path);

    // ---- Internal helper ----

    /** Tracks an active download. */
    private static class ActiveDownload {
        /** Unique handle for cancellation. */
        final String handle;
        /** Human-readable map name. */
        final String mapName;
        /** Listener for progress events. */
        final MapDownloadListener listener;
        /** Worker thread executing the download. */
        private Thread worker;
        /** Current HTTP response body stream; closing it unblocks a blocking read. */
        private volatile java.io.InputStream currentStream;
        /** True once cancel() has been called. */
        private volatile boolean cancelled;

        /**
         * @param handle   unique cancellation handle
         * @param mapName  human-readable map name
         * @param listener progress listener
         */
        ActiveDownload(String handle, String mapName, MapDownloadListener listener) {
            this.handle = handle;
            this.mapName = mapName;
            this.listener = listener;
        }

        /**
         * @param worker the Java thread running this download
         */
        void setWorker(Thread worker) {
            this.worker = worker;
        }

        /**
         * Register the stream currently being read, or null when none.
         *
         * @param stream the HTTP response body stream, or null
         */
        void setCurrentStream(java.io.InputStream stream) {
            this.currentStream = stream;
        }

        /** Interrupt the worker and force-close the active HTTP stream. */
        void cancel() {
            cancelled = true;
            if (worker != null) {
                worker.interrupt();
            }
            java.io.InputStream s = currentStream;
            if (s != null) {
                try {
                    s.close();
                } catch (Exception ignored) {
                }
            }
        }

        /** @return true if cancel() was called */
        boolean isCancelled() {
            return cancelled;
        }
    }
}
