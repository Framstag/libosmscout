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

    /** Category id for viewpoints. */
    public static final String VIEWPOINT = "viewpoint";

    /** Category id for museums. */
    public static final String MUSEUM = "museum";

    /** Category id for fuel stations. */
    public static final String FUEL = "fuel";

    /** Category id for electric vehicle charging stations. */
    public static final String CHARGING_STATION = "charging_station";

    /** Category id for ATMs. */
    public static final String ATM = "atm";

    /** Category id for general tourist-interest POIs. */
    public static final String TOURISM = "tourism";

    /** Category id for parking facilities. */
    public static final String PARKING = "parking";

    /** Category id for police stations. */
    public static final String POLICE = "police";

    /** Category id for hospitals. */
    public static final String HOSPITAL = "hospital";

    /** Category id for doctors offices. */
    public static final String DOCTORS = "doctors";

    /** Category id for public transport stops and stations. */
    public static final String PUBLIC_TRANSPORT = "public_transport";

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
        map.put(VIEWPOINT, new String[]{
            "tourism_viewpoint"
        });
        map.put(MUSEUM, new String[]{
            "tourism_museum",
            "tourism_museum_building"
        });
        // Only amenity_fuel / amenity_fuel_building are used; other fuel-related
        // types (e.g. amenity_ev_charging) are IGNOREd in map.ost and therefore
        // not present in imported databases.
        map.put(FUEL, new String[]{
            "amenity_fuel",
            "amenity_fuel_building"
        });
        // amenity_ev_charging exists in map.ost but is marked IGNORE, so it is
        // not stored in the database and cannot be searched.
        map.put(CHARGING_STATION, new String[]{
            "amenity_charging_station"
        });
        map.put(ATM, new String[]{
            "amenity_atm"
        });
        // Umbrella of tourist-interest types (map.ost). Hotels/lodging are NOT
        // included here - they belong to the "hotels" category.
        map.put(TOURISM, new String[]{
            "tourism_attraction",
            "tourism_attraction_building",
            "tourism_artwork",
            "tourism_aquarium",
            "tourism_zoo",
            "tourism_theme_park",
            "tourism_picnic_site",
            "tourism_camp_site",
            "tourism_caravan_site",
            "tourism_viewpoint",
            "tourism_museum",
            "tourism_information",
            "tourism_alpine_hut",
            "tourism_chalet"
        });
        // amenity_parking_entrance / amenity_parking_space are marked IGNORE in
        // map.ost and therefore not searchable.
        map.put(PARKING, new String[]{
            "amenity_parking",
            "amenity_bicycle_parking"
        });
        // Requires a database imported with a stylesheet that defines
        // amenity_police (stylesheets/map.ost). Databases imported before this
        // type existed return no police results.
        map.put(POLICE, new String[]{
            "amenity_police"
        });
        map.put(HOSPITAL, new String[]{
            "amenity_hospital",
            "amenity_hospital_building"
        });
        // Requires a database imported with a stylesheet that defines
        // amenity_doctors (stylesheets/map.ost).
        map.put(DOCTORS, new String[]{
            "amenity_doctors"
        });
        map.put(PUBLIC_TRANSPORT, new String[]{
            "railway_station",
            "railway_halt",
            "railway_tram_stop",
            "amenity_bus_station",
            "public_transport_platform",
            "railway_subway_entrance"
        });
        return map;
    }

    /**
     * Return the type names for the given category.
     *
     * @param category category id (keys of {@link #getCategoryTypes()}, e.g. {@link #HOTELS})
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
