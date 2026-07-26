package com.framstag.libosmscout;

import javafx.application.Application;

/**
 * JavaScout — JavaFX application for libosmscout.
 * <p>
 * Run with:
 * <pre>{@code
 * mvn javafx:run
 * javascout.sh
 * }</pre>
 * <p>
 * Maps are loaded from the default download directory
 * ({@code ~/.config/javascout/maps/} on Linux). No local maps directory
 * argument is required anymore — users download maps from a configured
 * provider inside the app. If no maps exist yet, the map view starts empty.
 * <p>
 * Additional options:
 * <pre>{@code
 * --stylesheet-dir /path/to/stylesheets
 * --icon-dir /path/to/icons
 * --map-provider <provider-name>
 * }</pre>
 */
public class JavaScout {

    /**
     * Application entry point.
     *
     * @param args optional: --stylesheet-dir <path>, --icon-dir <path>, --map-provider <name>
     */
    public static void main(String[] args) {
        Config config = new Config();
        String stylesheetDir = null;
        String iconDir = null;
        String mapProvider = null;

        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "--stylesheet-dir":
                    if (i + 1 < args.length) {
                        stylesheetDir = args[++i];
                    }
                    break;
                case "--icon-dir":
                    if (i + 1 < args.length) {
                        iconDir = args[++i];
                    }
                    break;
                case "--map-provider":
                    if (i + 1 < args.length) {
                        mapProvider = args[++i];
                    }
                    break;
                default:
                    // Ignore unknown non-flag arguments
                    break;
            }
        }

        // The only maps directory is the default download directory.
        String databaseDirectory = config.getMapsDirectory();

        // Stylesheet directory: CLI arg > config > default
        if (stylesheetDir != null) {
            JavaScoutApp.stylesheetDirectory = stylesheetDir;
            config.setStylesheetDirectory(stylesheetDir);
        } else {
            String saved = config.getStylesheetDirectory();
            if (saved != null) {
                JavaScoutApp.stylesheetDirectory = saved;
            }
            // If neither CLI nor config has it, leave null — MainController will use default
        }

        // Icon directory: CLI arg > config > default
        String resolvedIconDir;
        if (iconDir != null) {
            resolvedIconDir = iconDir;
            config.setIconDirectory(iconDir);
        } else {
            resolvedIconDir = config.getIconDirectory();
        }
        // Persist the resolved icon directory (saves the default on first launch too)
        config.setIconDirectory(resolvedIconDir);
        // Pass via system property so JavaFX Application instance can read it reliably
        System.setProperty("javascout.iconDirectory", resolvedIconDir);
        JavaScoutApp.iconDirectory = resolvedIconDir;

        Log.info("[JavaScout] maps directory: " + databaseDirectory);
        Log.info("[JavaScout] stylesheet directory: " + JavaScoutApp.stylesheetDirectory);
        Log.info("[JavaScout] icon directory: " + resolvedIconDir);
        Log.info("[JavaScout] config file: " + config.getConfigFilePath());

        // Map provider: CLI arg > config
        if (mapProvider != null) {
            config.setMapProvider(mapProvider);
        }
        JavaScoutApp.appConfig = config;

        // Filter out known flags and their values for JavaFX
        java.util.List<String> fxArgs = new java.util.ArrayList<>();
        for (int i = 0; i < args.length; i++) {
            switch (args[i]) {
                case "--stylesheet-dir":
                case "--icon-dir":
                case "--map-provider":
                    i++; // skip value
                    break;
                default:
                    // Ignore unknown non-flag arguments as well
                    break;
            }
        }

        // Launch JavaFX
        Application.launch(JavaScoutApp.class, fxArgs.toArray(new String[0]));
    }
}
