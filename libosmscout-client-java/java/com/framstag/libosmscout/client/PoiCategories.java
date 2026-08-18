package com.framstag.libosmscout.client;

import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Hardcoded mapping from POI search categories to concrete OSM type names.
 * <p>
 * First iteration: the mapping is fixed in code and shared between the client
 * API and the JavaScout UI. The type names must exist in the loaded database's
 * {@code TypeConfig} (they come from the stylesheet used at import time).
 */
public final class PoiCategories {

    /** Category id for hotels. */
    public static final String HOTELS = "hotels";

    /** Category id for restaurants. */
    public static final String RESTAURANTS = "restaurants";

    /** Category id for grocery stores. */
    public static final String GROCERY = "grocery";

    private static final Map<String, String[]> CATEGORY_TYPES = createCategoryTypes();

    private PoiCategories() {
    }

    private static Map<String, String[]> createCategoryTypes() {
        Map<String, String[]> map = new LinkedHashMap<>();
        map.put(HOTELS, new String[]{
            "tourism_hotel",
            "tourism_motel",
            "tourism_hostel",
            "tourism_guest_house"
        });
        map.put(RESTAURANTS, new String[]{
            "amenity_restaurant",
            "amenity_fast_food"
        });
        // Granular food/beverage shop types (stylesheets/map.ost, values from
        // the OSM Key:shop wiki "Food, beverages" group). The generic "shop"
        // type is NOT used so that non-food shops (clothing, electronics, ...)
        // are excluded.
        map.put(GROCERY, new String[]{
            "shop_supermarket",
            "shop_convenience",
            "shop_grocery",
            "shop_greengrocer",
            "shop_butcher",
            "shop_bakery",
            "shop_deli",
            "shop_cheese",
            "shop_dairy",
            "shop_seafood",
            "shop_frozen_food",
            "shop_health_food",
            "shop_farm",
            "shop_food",
            "shop_confectionery",
            "shop_pastry",
            "shop_chocolate",
            "shop_coffee",
            "shop_tea",
            "shop_spices",
            "shop_alcohol",
            "shop_beverages",
            "shop_wine",
            "shop_ice_cream"
        });
        return map;
    }

    /**
     * Return the type names for the given category.
     *
     * @param category category id (see {@link #HOTELS}, {@link #RESTAURANTS}, {@link #GROCERY})
     * @return array of OSM type names, or null if the category is unknown
     */
    public static String[] getTypeNames(String category) {
        String[] types = CATEGORY_TYPES.get(category);
        if (types == null) {
            return null;
        }
        return types.clone();
    }

    /**
     * Return all supported category ids.
     *
     * @return unmodifiable map of category id to type names
     */
    public static Map<String, String[]> getCategoryTypes() {
        return Collections.unmodifiableMap(CATEGORY_TYPES);
    }
}
