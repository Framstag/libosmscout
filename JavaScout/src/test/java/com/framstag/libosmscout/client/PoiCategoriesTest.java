package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.Map;

import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for the hardcoded POI category → OSM type mapping.
 */
class PoiCategoriesTest {

    @Test
    void hotelsMapsToHotelTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.HOTELS);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("tourism_hotel"));
        assertTrue(Arrays.asList(types).contains("tourism_motel"));
        assertTrue(Arrays.asList(types).contains("tourism_hostel"));
        assertTrue(Arrays.asList(types).contains("tourism_guest_house"));
    }

    @Test
    void restaurantsMapsToRestaurantTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.RESTAURANTS);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_restaurant"));
        assertTrue(Arrays.asList(types).contains("amenity_fast_food"));
    }

    @Test
    void groceryMapsToGroceryShopTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.GROCERY);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("shop_supermarket"));
        assertTrue(Arrays.asList(types).contains("shop_convenience"));
        assertTrue(Arrays.asList(types).contains("shop_grocery"));
        assertTrue(Arrays.asList(types).contains("shop_greengrocer"));
        assertTrue(Arrays.asList(types).contains("shop_butcher"));
        assertTrue(Arrays.asList(types).contains("shop_bakery"));
        assertTrue(Arrays.asList(types).contains("shop_deli"));
        assertTrue(Arrays.asList(types).contains("shop_cheese"));
        assertTrue(Arrays.asList(types).contains("shop_dairy"));
        assertTrue(Arrays.asList(types).contains("shop_seafood"));
        assertTrue(Arrays.asList(types).contains("shop_frozen_food"));
        assertTrue(Arrays.asList(types).contains("shop_health_food"));
        assertTrue(Arrays.asList(types).contains("shop_farm"));
        assertTrue(Arrays.asList(types).contains("shop_food"));
        assertTrue(Arrays.asList(types).contains("shop_confectionery"));
        assertTrue(Arrays.asList(types).contains("shop_pastry"));
        assertTrue(Arrays.asList(types).contains("shop_chocolate"));
        assertTrue(Arrays.asList(types).contains("shop_coffee"));
        assertTrue(Arrays.asList(types).contains("shop_tea"));
        assertTrue(Arrays.asList(types).contains("shop_spices"));
        assertTrue(Arrays.asList(types).contains("shop_alcohol"));
        assertTrue(Arrays.asList(types).contains("shop_beverages"));
        assertTrue(Arrays.asList(types).contains("shop_wine"));
        assertTrue(Arrays.asList(types).contains("shop_ice_cream"));
        assertFalse(Arrays.asList(types).contains("shop"),
            "generic shop type must not be used for grocery search");
    }

    @Test
    void unknownCategoryReturnsNull() {
        assertNull(PoiCategories.getTypeNames("unknown-category"));
    }

    @Test
    void categoryTypesContainsAllCategories() {
        Map<String, String[]> all = PoiCategories.getCategoryTypes();
        assertEquals(3, all.size());
        assertTrue(all.containsKey(PoiCategories.HOTELS));
        assertTrue(all.containsKey(PoiCategories.RESTAURANTS));
        assertTrue(all.containsKey(PoiCategories.GROCERY));
    }

    @Test
    void getTypeNamesReturnsDefensiveCopy() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.HOTELS);
        types[0] = "mutated";
        String[] again = PoiCategories.getTypeNames(PoiCategories.HOTELS);
        assertEquals("tourism_hotel", again[0]);
    }
}
