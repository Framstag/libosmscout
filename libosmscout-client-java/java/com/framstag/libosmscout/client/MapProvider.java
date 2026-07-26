package com.framstag.libosmscout.client;

/**
 * Represents a map provider that can serve map listings and downloads.
 * Mirrors the C++ {@code osmscout::MapProvider} struct.
 */
public class MapProvider {

    /** Human-readable provider name (e.g. "karry.cz"). */
    private final String name;
    /** Base URI for file downloads. */
    private final String uri;
    /** URI template for fetching the map list. */
    private final String listUri;

    /**
     * @param name    human-readable provider name (e.g. "karry.cz")
     * @param uri     base URI for file downloads
     * @param listUri URI template for fetching the map list.
     *                Placeholders: %1 = fromVersion, %2 = toVersion, %3 = locale
     */
    public MapProvider(String name, String uri, String listUri) {
        this.name = name;
        this.uri = uri;
        this.listUri = listUri;
    }

    /**
     * @return human-readable provider name
     */
    public String getName() {
        return name;
    }

    /**
     * @return base URI for file downloads
     */
    public String getUri() {
        return uri;
    }

    /**
     * @return URI template for fetching the map list
     */
    public String getListUri() {
        return listUri;
    }

    @Override
    public String toString() {
        return "MapProvider{name='" + name + "'}";
    }
}
