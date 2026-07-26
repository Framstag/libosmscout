package com.framstag.libosmscout.client;

/**
 * Callback interface for map download progress and completion events.
 * Methods are invoked from a background thread — marshal to UI thread if needed.
 */
public interface MapDownloadListener {

    /**
     * Called periodically during download.
     *
     * @param mapName   human-readable map name
     * @param bytesDownloaded bytes transferred so far
     * @param totalBytes      total expected size (0 if unknown)
     */
    default void onProgress(String mapName, long bytesDownloaded, long totalBytes) {
    }

    /**
     * Called when a map download completes successfully.
     *
     * @param mapName human-readable map name
     * @param targetDir the directory where the map was downloaded
     */
    default void onComplete(String mapName, String targetDir) {
    }

    /**
     * Called when a map download fails.
     *
     * @param mapName human-readable map name
     * @param errorMessage description of the error
     */
    default void onError(String mapName, String errorMessage) {
    }
}
