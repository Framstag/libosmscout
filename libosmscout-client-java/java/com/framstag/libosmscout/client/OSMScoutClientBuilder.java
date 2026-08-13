package com.framstag.libosmscout.client;

/**
 * Fluent builder for {@link OSMScoutClient}.
 */
@SuppressWarnings("restricted")
public class OSMScoutClientBuilder {

    static {
        System.loadLibrary("osmscout_client_java");
    }

    /** Path to basemap directory. */
    private String basemapLookupDirectory;
    /** Path to icon resources. */
    private String iconDirectory;
    /** Directories to search for .osmscout map data. */
    private String[] mapLookupDirectories;
    /** Screen physical DPI. */
    private double physicalDpi;
    /** Base font size in millimeters. */
    private double fontSizeMm = 2.5;
    /** Measurement system: "metrics" or "imperial". */
    private String units;
    /** Directory containing .oss stylesheet files. */
    private String stylesheetDirectory;
    /** Synthetic POI types to register in the type config at runtime. */
    private String[] customPoiTypes;
    /** Default directory for downloaded maps. */
    private String mapsDirectory;

    /** Constructor. */
    public OSMScoutClientBuilder() {
    }

    /**
     * Set path to basemap directory.
     *
     * @param basemapLookupDirectory path to basemap OSMScout data
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withBasemapLookupDirectory(String basemapLookupDirectory) {
        this.basemapLookupDirectory = basemapLookupDirectory;
        return this;
    }

    /**
     * Set path to icon directory.
     *
     * @param iconDirectory path to icon resources
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withIconDirectory(String iconDirectory) {
        this.iconDirectory = iconDirectory;
        return this;
    }

    /**
     * Set directories to search for map databases.
     *
     * @param mapLookupDirectories paths to search for .osmscout map data
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withMapLookupDirectories(String... mapLookupDirectories) {
        this.mapLookupDirectories = mapLookupDirectories;
        return this;
    }

    /**
     * Set screen physical DPI.
     *
     * @param physicalDpi screen DPI value (e.g. 130.0 for desktop, ~330 for mobile)
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withPhysicalDpi(double physicalDpi) {
        this.physicalDpi = physicalDpi;
        return this;
    }

    /**
     * Set base font size in millimeters.
     *
     * @param fontSizeMm font size in mm (default 4.5)
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withFontSizeMm(double fontSizeMm) {
        this.fontSizeMm = fontSizeMm;
        return this;
    }

    /**
     * Set measurement system.
     *
     * @param units "metrics" or "imperial"
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withUnits(String units) {
        this.units = units;
        return this;
    }

    /**
     * Set directory containing .oss stylesheet files.
     * Defaults to "stylesheets" (relative to working directory).
     *
     * @param stylesheetDirectory path to stylesheet directory
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withStyleSheetDirectory(String stylesheetDirectory) {
        this.stylesheetDirectory = stylesheetDirectory;
        return this;
    }

    /**
     * Register a synthetic POI type in the map style config at runtime.
     * Multiple calls can be chained; all types are registered before the
     * database is opened. The type must be styled in the loaded .oss
     * stylesheet to be visible.
     *
     * @param typeName synthetic type name, e.g. "_favorite"
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withCustomPoiType(String typeName) {
        if (this.customPoiTypes == null) {
            this.customPoiTypes = new String[] { typeName };
        } else {
            String[] updated = new String[this.customPoiTypes.length + 1];
            System.arraycopy(this.customPoiTypes, 0, updated, 0, this.customPoiTypes.length);
            updated[this.customPoiTypes.length] = typeName;
            this.customPoiTypes = updated;
        }
        return this;
    }

    /**
     * Set the default directory for downloaded maps.
     *
     * @param mapsDirectory path to store downloaded map data
     * @return this builder for chaining
     */
    public OSMScoutClientBuilder withMapsDirectory(String mapsDirectory) {
        this.mapsDirectory = mapsDirectory;
        return this;
    }

    /**
     * Build the {@link OSMScoutClient} instance.
     * This native method creates the underlying C++ client objects
     * (Settings, MapManager, DBThread) and initialises them.
     *
     * @return a new OSMScoutClient on success, null if already initialised
     */
    public native OSMScoutClient build();
}