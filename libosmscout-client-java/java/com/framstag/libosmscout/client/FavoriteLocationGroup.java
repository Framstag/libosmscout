package com.framstag.libosmscout.client;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A named group of favorite locations. Groups form a one-level
 * hierarchy. Each group has an extensible attribute map.
 */
public class FavoriteLocationGroup {

    /** Group display name. */
    public String name;

    /** List of favorites in this group. */
    public List<FavoriteLocation> favorites;

    /** Extensible attribute map for future fields. */
    public Map<String, String> attributes;

    /** Default constructor. */
    public FavoriteLocationGroup() {
        this.favorites = new ArrayList<>();
        this.attributes = new HashMap<>();
    }

    /**
     * Construct a group with the given name.
     *
     * @param name group display name
     */
    public FavoriteLocationGroup(String name) {
        this.name = name;
        this.favorites = new ArrayList<>();
        this.attributes = new HashMap<>();
    }
}
