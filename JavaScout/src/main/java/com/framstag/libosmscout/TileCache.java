package com.framstag.libosmscout;

import javafx.scene.image.PixelFormat;
import javafx.scene.image.WritableImage;
import javafx.scene.image.WritablePixelFormat;

import java.nio.IntBuffer;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * LRU tile cache for rendered map tiles.
 * <p>
 * Stores rendered tiles keyed by (zoomLevel, tileX, tileY).
 * After a full render completes, the result is split into tiles and cached.
 * On subsequent renders, cached tiles are reused and only missing tiles
 * are rendered individually.
 * <p>
 * Evicts least recently used tiles when cache size exceeds limit.
 * Supports epoch-based invalidation.
 */
public class TileCache {

    /** Default maximum number of tiles in cache. */
    public static final int DEFAULT_MAX_SIZE = 200;

    /** Default tile dimension in pixels. */
    public static final int TILE_SIZE = 256;

    private static final WritablePixelFormat<IntBuffer> ARGB_FORMAT =
            PixelFormat.getIntArgbPreInstance();

    private final int maxSize;
    private final LinkedHashMap<TileKey, CachedTile> cache;

    /**
     * Create a tile cache with the given maximum size.
     *
     * @param maxSize maximum number of tiles before LRU eviction
     */
    public TileCache(int maxSize) {
        this.maxSize = maxSize;
        this.cache = new LinkedHashMap<TileKey, CachedTile>(16, 0.75f, true) {
            @Override
            protected boolean removeEldestEntry(Map.Entry<TileKey, CachedTile> eldest) {
                return size() > TileCache.this.maxSize;
            }
        };
    }

    /**
     * Create a tile cache with default max size.
     */
    public TileCache() {
        this(DEFAULT_MAX_SIZE);
    }

    /**
     * Key for a tile in the cache.
     */
    public record TileKey(int zoomLevel, int tileX, int tileY) {}

    /**
     * A cached tile with its epoch for invalidation.
     */
    private record CachedTile(WritableImage image, long epoch) {}

    /**
     * Get a cached tile image.
     *
     * @param key   tile key
     * @param epoch current epoch (tile is only returned if its epoch matches)
     * @return the cached image, or null if not found or epoch mismatch
     */
    public synchronized WritableImage get(TileKey key, long epoch) {
        CachedTile entry = cache.get(key);
        if (entry == null || entry.epoch() != epoch) {
            return null;
        }
        return entry.image();
    }

    /**
     * Store a tile in the cache.
     *
     * @param key   tile key
     * @param image rendered tile image
     * @param epoch current epoch for invalidation
     */
    public synchronized void put(TileKey key, WritableImage image, long epoch) {
        cache.put(key, new CachedTile(image, epoch));
    }

    /**
     * Check if a tile is in the cache and valid for the given epoch.
     */
    public synchronized boolean contains(TileKey key, long epoch) {
        CachedTile entry = cache.get(key);
        return entry != null && entry.epoch() == epoch;
    }

    /**
     * Remove all tiles with an epoch different from the given one.
     * Call this on epoch change to purge stale entries.
     *
     * @param validEpoch only tiles with this epoch are kept
     */
    public synchronized void retainEpoch(long validEpoch) {
        cache.values().removeIf(entry -> entry.epoch() != validEpoch);
    }

    /**
     * Remove all tiles.
     */
    public synchronized void clear() {
        cache.clear();
    }

    /**
     * Get the current number of cached tiles.
     */
    public synchronized int size() {
        return cache.size();
    }

    /**
     * Compute tile keys covering a rendered area.
     * <p>
     * Divides the pixel area (0,0)-(width,height) into tiles of {@link #TILE_SIZE}.
     * Each tile is identified by its column/row in the grid.
     *
     * @param width  total pixel width of the rendered area
     * @param height total pixel height of the rendered area
     * @param zoomLevel the zoom level at which this area was rendered
     * @return array of tile keys covering the area
     */
    public static TileKey[] computeTileGrid(int width, int height, int zoomLevel) {
        int cols = (int) Math.ceil((double) width / TILE_SIZE);
        int rows = (int) Math.ceil((double) height / TILE_SIZE);
        TileKey[] keys = new TileKey[cols * rows];
        int idx = 0;
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                keys[idx++] = new TileKey(zoomLevel, col, row);
            }
        }
        return keys;
    }

    /**
     * Split a full rendered pixel buffer into tiles and store in cache.
     * <p>
     * Called after a full render completes. Extracts each tile-sized region
     * from the pixel buffer and stores it as a {@link WritableImage}.
     *
     * @param pixels    full rendered pixel buffer (ARGB ints)
     * @param width     pixel width of the full render
     * @param height    pixel height of the full render
     * @param zoomLevel zoom level of the render
     * @param epoch     current epoch for cache invalidation
     */
    public void storeTiles(int[] pixels, int width, int height, int zoomLevel, long epoch) {
        TileKey[] keys = computeTileGrid(width, height, zoomLevel);
        for (TileKey key : keys) {
            int tileW = Math.min(TILE_SIZE, width - key.tileX() * TILE_SIZE);
            int tileH = Math.min(TILE_SIZE, height - key.tileY() * TILE_SIZE);
            if (tileW <= 0 || tileH <= 0) continue;

            WritableImage tileImage = new WritableImage(tileW, tileH);
            int[] tilePixels = new int[tileW * tileH];

            // Extract tile region from full pixel buffer
            int srcX = key.tileX() * TILE_SIZE;
            int srcY = key.tileY() * TILE_SIZE;
            for (int y = 0; y < tileH; y++) {
                System.arraycopy(pixels, (srcY + y) * width + srcX,
                                 tilePixels, y * tileW, tileW);
            }

            tileImage.getPixelWriter().setPixels(0, 0, tileW, tileH,
                                                  ARGB_FORMAT, tilePixels, 0, tileW);
            put(key, tileImage, epoch);
        }
    }

    /**
     * Compose a full image from cached tiles, filling missing tiles with null.
     * <p>
     * Returns a {@link CompositeResult} containing the composed image and
     * information about which tiles are missing.
     *
     * @param width     target pixel width
     * @param height    target pixel height
     * @param zoomLevel zoom level for tile keys
     * @param epoch     current epoch for cache validation
     * @return composite result, or null if no tiles are cached
     */
    public CompositeResult compose(int width, int height, int zoomLevel, long epoch) {
        TileKey[] keys = computeTileGrid(width, height, zoomLevel);
        WritableImage result = new WritableImage(width, height);
        var pw = result.getPixelWriter();

        boolean anyCached = false;
        int missingCount = 0;

        for (TileKey key : keys) {
            int tileW = Math.min(TILE_SIZE, width - key.tileX() * TILE_SIZE);
            int tileH = Math.min(TILE_SIZE, height - key.tileY() * TILE_SIZE);
            if (tileW <= 0 || tileH <= 0) continue;

            WritableImage tile = get(key, epoch);
            if (tile != null) {
                // Copy cached tile pixels into result
                int[] tilePixels = new int[tileW * tileH];
                var reader = tile.getPixelReader();
                reader.getPixels(0, 0, tileW, tileH, ARGB_FORMAT, tilePixels, 0, tileW);
                pw.setPixels(key.tileX() * TILE_SIZE, key.tileY() * TILE_SIZE,
                             tileW, tileH, ARGB_FORMAT, tilePixels, 0, tileW);
                anyCached = true;
            } else {
                missingCount++;
            }
        }

        if (!anyCached) {
            return null;
        }

        return new CompositeResult(result, missingCount);
    }

    /**
     * Result of composing cached tiles into a full image.
     */
    public record CompositeResult(WritableImage image, int missingTiles) {
        boolean isComplete() {
            return missingTiles == 0;
        }
    }
}
