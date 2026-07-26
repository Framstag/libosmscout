package com.framstag.libosmscout;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Properties;

/**
 * Configuration file handler for JavaScout.
 * <p>
 * Reads/writes {@code config.properties} in an OS-specific config directory:
 * <ul>
 *   <li>Linux:   {@code ~/.config/javascout/config.properties}</li>
 *   <li>macOS:   {@code ~/Library/Application Support/JavaScout/config.properties}</li>
 *   <li>Windows: {@code %APPDATA%\JavaScout\config.properties}</li>
 * </ul>
 */
public class Config {

    private static final String CONFIG_DIR = "javascout";
    private static final String FILE_NAME  = "config.properties";
    private static final String MAPS_KEY  = "maps.directory";
    private static final String STYLESHEET_KEY = "stylesheets.directory";
    private static final String ICON_KEY = "icon.directory";
    private static final String MAP_LAT_KEY = "map.latitude";
    private static final String MAP_LON_KEY = "map.longitude";
    private static final String MAP_MAG_KEY = "map.magnification";
    private static final String LONG_PRESS_TIMEOUT_KEY = "longPressTimeoutMs";
    private static final String MAP_PROVIDER_KEY = "map.provider";
    private static final String FAVORITES_FILE = "favorites.json";

    private static final int DEFAULT_LONG_PRESS_TIMEOUT_MS = 500;
    private static final String DEFAULT_ICON_DIRECTORY = "libosmscout/data/icons/14x14/standard";
    private static final String LEGACY_ICON_DIRECTORY = "libosmscout/data/icons/svg/standard";

    private final Path configFile;

    /** Create Config for the default OS-specific location. */
    public Config() {
        this.configFile = getConfigDir().resolve(FILE_NAME);
    }

    /** Create Config with a custom path (for testing). */
    Config(Path configFile) {
        this.configFile = configFile;
    }

    /**
     * Returns the OS-specific config directory.
     * Creates it if it doesn't exist.
     */
    public static Path getConfigDir() {
        String os = System.getProperty("os.name").toLowerCase();
        Path base;

        if (os.contains("win")) {
            base = Paths.get(System.getenv("APPDATA"));
        } else if (os.contains("mac")) {
            base = Paths.get(System.getProperty("user.home"), "Library", "Application Support");
        } else {
            // Linux, BSD, etc.
            String xdg = System.getenv("XDG_CONFIG_HOME");
            if (xdg != null && !xdg.isEmpty()) {
                base = Paths.get(xdg);
            } else {
                base = Paths.get(System.getProperty("user.home"), ".config");
            }
        }

        return base.resolve(CONFIG_DIR);
    }

    /**
     * Read the configured maps directory from the config file.
     * Falls back to {@code <configDir>/maps} if not set.
     *
     * @return the maps directory path, or default if not set
     */
    public String getMapsDirectory() {
        String val = getProperty(MAPS_KEY);
        if (val == null || val.isEmpty()) {
            return getConfigDir().resolve("maps").toString();
        }
        return val;
    }

    /**
     * Set the maps directory in the config file.
     * Creates the config directory and file if they don't exist.
     *
     * @param path the maps directory path to persist
     */
    public void setMapsDirectory(String path) {
        setProperty(MAPS_KEY, path);
    }

    /**
     * Read the configured stylesheet directory from the config file.
     *
     * @return the stylesheet directory path, or {@code null} if not set
     */
    public String getStylesheetDirectory() {
        return getProperty(STYLESHEET_KEY);
    }

    /**
     * Set the stylesheet directory in the config file.
     */
    public void setStylesheetDirectory(String path) {
        setProperty(STYLESHEET_KEY, path);
    }

    public String getConfigFilePath() {
        return configFile.toString();
    }

    /**
     * Read the configured icon directory.
     *
     * @return the icon directory path, or the default if not set
     */
    public String getIconDirectory() {
        String val = getProperty(ICON_KEY);
        if (val == null || val.isEmpty()) {
            return DEFAULT_ICON_DIRECTORY;
        }
        // Migrate persisted SVG default to PNG default for the Cairo renderer
        if (val.equals(LEGACY_ICON_DIRECTORY)) {
            return DEFAULT_ICON_DIRECTORY;
        }
        return val;
    }

    /**
     * Set the icon directory in the config file.
     *
     * @param path the icon directory path to persist
     */
    public void setIconDirectory(String path) {
        setProperty(ICON_KEY, path);
    }

    /**
     * Read the saved map latitude.
     *
     * @return latitude, or {@code null} if not set
     */
    public String getMapLatitude() {
        return getProperty(MAP_LAT_KEY);
    }

    /**
     * Read the saved map longitude.
     *
     * @return longitude, or {@code null} if not set
     */
    public String getMapLongitude() {
        return getProperty(MAP_LON_KEY);
    }

    /**
     * Read the saved map magnification level.
     *
     * @return magnification level string, or {@code null} if not set
     */
    public String getMapMagnification() {
        return getProperty(MAP_MAG_KEY);
    }

    /**
     * Save the current map position to the config file.
     *
     * @param lat latitude
     * @param lon longitude
     * @param mag magnification level
     */
    public void setMapPosition(double lat, double lon, int mag) {
        setProperty(MAP_LAT_KEY, String.valueOf(lat));
        setProperty(MAP_LON_KEY, String.valueOf(lon));
        setProperty(MAP_MAG_KEY, String.valueOf(mag));
    }

    /**
     * Read the long-press timeout in milliseconds.
     *
     * @return timeout in ms, or default (500) if not set
     */
    public int getLongPressTimeoutMs() {
        String val = getProperty(LONG_PRESS_TIMEOUT_KEY);
        if (val != null) {
            try {
                return Integer.parseInt(val);
            } catch (NumberFormatException e) {
                // fall through
            }
        }
        return DEFAULT_LONG_PRESS_TIMEOUT_MS;
    }

    /**
     * Set the long-press timeout in milliseconds.
     *
     * @param ms timeout in milliseconds
     */
    public void setLongPressTimeoutMs(int ms) {
        setProperty(LONG_PRESS_TIMEOUT_KEY, String.valueOf(ms));
    }

    /**
     * Read the configured map provider name.
     *
     * @return provider name, or {@code null} if not set
     */
    public String getMapProvider() {
        return getProperty(MAP_PROVIDER_KEY);
    }

    /**
     * Set the map provider name in the config file.
     *
     * @param provider provider name to persist
     */
    public void setMapProvider(String provider) {
        setProperty(MAP_PROVIDER_KEY, provider);
    }

    /**
     * Get the absolute path to the favorites JSON file.
     *
     * @return path to favorites.json in the config directory
     */
    public String getFavoritesFilePath() {
        return getConfigDir().resolve(FAVORITES_FILE).toString();
    }

    /**
     * Read a string property from the config file.
     *
     * @param key the property key
     * @return the value, or {@code null} if not set or file missing
     */
    private String getProperty(String key) {
        if (!Files.exists(configFile)) {
            return null;
        }

        try {
            Properties props = new Properties();
            try (var in = Files.newInputStream(configFile)) {
                props.load(in);
            }
            String value = props.getProperty(key);
            return (value != null && !value.isBlank()) ? value.trim() : null;
        } catch (IOException e) {
            Log.error("Warning: failed to read config: " + e.getMessage());
            return null;
        }
    }

    /**
     * Set a string property in the config file.
     * Creates the config directory and file if they don't exist.
     *
     * @param key   the property key
     * @param value the value to persist
     */
    private void setProperty(String key, String value) {
        try {
            Files.createDirectories(configFile.getParent());

            Properties props = new Properties();
            if (Files.exists(configFile)) {
                try (var in = Files.newInputStream(configFile)) {
                    props.load(in);
                }
            }

            props.setProperty(key, value);

            try (var out = Files.newOutputStream(configFile)) {
                props.store(out, "JavaScout configuration");
            }
        } catch (IOException e) {
            Log.error("Warning: failed to write config: " + e.getMessage());
        }
    }
}
