package com.framstag.libosmscout.client;

/**
 * A starred favorite location together with the name of the group that holds
 * it.
 *
 * <p>The starred favorites form one order that spans all groups, so it is not
 * enough to return the favorites alone: a caller that presents or reorders that
 * order needs to know for each entry which group the favorite lives in.</p>
 */
public class StarredFavoriteLocation {

    /** Name of the group that holds the favorite. */
    public String groupName;

    /** The starred favorite. */
    public FavoriteLocation favorite;

    /** Default constructor. */
    public StarredFavoriteLocation() {
    }

    /**
     * Construct a starred entry with the given group and favorite.
     *
     * @param groupName name of the group that holds the favorite
     * @param favorite  the starred favorite
     */
    public StarredFavoriteLocation(String groupName, FavoriteLocation favorite) {
        this.groupName = groupName;
        this.favorite = favorite;
    }
}
