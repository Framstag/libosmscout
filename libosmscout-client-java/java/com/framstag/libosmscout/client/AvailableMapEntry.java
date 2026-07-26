package com.framstag.libosmscout.client;

import java.util.Collections;
import java.util.List;

/**
 * Represents a single entry in the available maps tree returned by a map provider.
 * An entry is either a directory (grouping of maps) or a leaf map that can be downloaded.
 * Mirrors the C++ {@code osmscout::AvailableMapEntry} class.
 */
public class AvailableMapEntry {

    /** True if this entry is a directory, false if it is a downloadable map. */
    private final boolean directory;
    /** Human-readable name (e.g. "Czech Republic", "Europe"). */
    private final String name;
    /** Logical path segments (e.g. ["europe", "czech-republic"]). */
    private final List<String> path;
    /** Optional description. */
    private final String description;
    /** The provider that serves this map. Null for directories. */
    private final MapProvider provider;
    /** Size in bytes. 0 for directories. */
    private final long size;
    /** Server directory path for download. Null for directories. */
    private final String serverDirectory;
    /** Map creation timestamp (epoch seconds). 0 for directories. */
    private final long creationTimestamp;
    /** Map data version. -1 for directories. */
    private final int version;
    /** Child entries (for directories). */
    private final List<AvailableMapEntry> children;

    /**
     * Constructor for directory entries.
     *
     * @param name        directory name
     * @param path        logical path segments
     * @param description optional description
     */
    public AvailableMapEntry(String name, List<String> path, String description) {
        this.directory = true;
        this.name = name;
        this.path = path != null ? Collections.unmodifiableList(path) : List.of();
        this.description = description;
        this.provider = null;
        this.size = 0;
        this.serverDirectory = null;
        this.creationTimestamp = 0;
        this.version = -1;
        this.children = List.of();
    }

    /**
     * Package-private constructor for leaf map entries.
     *
     * @param name              map name
     * @param path              logical path segments
     * @param description       optional description
     * @param provider          the map provider
     * @param size              size in bytes
     * @param serverDirectory   server directory path
     * @param creationTimestamp creation time (epoch seconds)
     * @param version           map data version
     */
    AvailableMapEntry(String name, List<String> path, String description,
                      MapProvider provider, long size,
                      String serverDirectory, long creationTimestamp, int version) {
        this.directory = false;
        this.name = name;
        this.path = path != null ? Collections.unmodifiableList(path) : List.of();
        this.description = description;
        this.provider = provider;
        this.size = size;
        this.serverDirectory = serverDirectory;
        this.creationTimestamp = creationTimestamp;
        this.version = version;
        this.children = List.of();
    }

    /**
     * @return true if this entry is a directory
     */
    public boolean isDirectory() {
        return directory;
    }

    /**
     * @return human-readable name
     */
    public String getName() {
        return name;
    }

    /**
     * @return logical path segments
     */
    public List<String> getPath() {
        return path;
    }

    /**
     * @return optional description
     */
    public String getDescription() {
        return description;
    }

    /**
     * @return the map provider, or null for directories
     */
    public MapProvider getProvider() {
        return provider;
    }

    /**
     * @return size in bytes, 0 for directories
     */
    public long getSize() {
        return size;
    }

    /**
     * @return server directory path, null for directories
     */
    public String getServerDirectory() {
        return serverDirectory;
    }

    /**
     * @return creation timestamp (epoch seconds), 0 for directories
     */
    public long getCreationTimestamp() {
        return creationTimestamp;
    }

    /**
     * @return map data version, -1 for directories
     */
    public int getVersion() {
        return version;
    }

    /**
     * Recursively search for an entry with the given name (case-insensitive).
     *
     * @param name the name to search for
     * @return first matching entry, or null if none found
     */
    public AvailableMapEntry findEntryByName(String name) {
        if (getName().equalsIgnoreCase(name)) {
            return this;
        }
        for (AvailableMapEntry child : children) {
            AvailableMapEntry found = child.findEntryByName(name);
            if (found != null) {
                return found;
            }
        }
        return null;
    }

    /**
     * @return child entries (for directories)
     */
    public List<AvailableMapEntry> getChildren() {
        return children;
    }

    @Override
    public String toString() {
        return "AvailableMapEntry{name='" + name + "', dir=" + directory + "}";
    }
}
