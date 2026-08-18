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
        assertEquals(14, all.size());
        assertTrue(all.containsKey(PoiCategories.HOTELS));
        assertTrue(all.containsKey(PoiCategories.RESTAURANTS));
        assertTrue(all.containsKey(PoiCategories.GROCERY));
        assertTrue(all.containsKey(PoiCategories.VIEWPOINT));
        assertTrue(all.containsKey(PoiCategories.MUSEUM));
        assertTrue(all.containsKey(PoiCategories.FUEL));
        assertTrue(all.containsKey(PoiCategories.CHARGING_STATION));
        assertTrue(all.containsKey(PoiCategories.ATM));
        assertTrue(all.containsKey(PoiCategories.TOURISM));
        assertTrue(all.containsKey(PoiCategories.PARKING));
        assertTrue(all.containsKey(PoiCategories.POLICE));
        assertTrue(all.containsKey(PoiCategories.HOSPITAL));
        assertTrue(all.containsKey(PoiCategories.DOCTORS));
        assertTrue(all.containsKey(PoiCategories.PUBLIC_TRANSPORT));
    }

    @Test
    void viewpointMapsToViewpointTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.VIEWPOINT);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("tourism_viewpoint"));
    }

    @Test
    void museumMapsToMuseumTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.MUSEUM);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("tourism_museum"));
        assertTrue(Arrays.asList(types).contains("tourism_museum_building"));
    }

    @Test
    void fuelMapsToFuelTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.FUEL);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_fuel"));
        assertTrue(Arrays.asList(types).contains("amenity_fuel_building"));
    }

    @Test
    void chargingStationMapsToChargingStationType() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.CHARGING_STATION);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_charging_station"));
        assertFalse(Arrays.asList(types).contains("amenity_ev_charging"),
            "amenity_ev_charging is IGNOREd in map.ost and not searchable");
    }

    @Test
    void atmMapsToAtmType() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.ATM);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_atm"));
    }

    @Test
    void tourismMapsToTourismTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.TOURISM);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("tourism_attraction"));
        assertTrue(Arrays.asList(types).contains("tourism_viewpoint"));
        assertTrue(Arrays.asList(types).contains("tourism_information"));
        assertFalse(Arrays.asList(types).contains("tourism_hotel"),
            "hotels belong to the hotels category, not the tourism umbrella");
    }

    @Test
    void parkingMapsToParkingTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.PARKING);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_parking"));
        assertTrue(Arrays.asList(types).contains("amenity_bicycle_parking"));
        assertFalse(Arrays.asList(types).contains("amenity_parking_entrance"),
            "amenity_parking_entrance is IGNOREd in map.ost and not searchable");
        assertFalse(Arrays.asList(types).contains("amenity_parking_space"),
            "amenity_parking_space is IGNOREd in map.ost and not searchable");
    }

    @Test
    void policeMapsToPoliceType() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.POLICE);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_police"));
    }

    @Test
    void hospitalMapsToHospitalTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.HOSPITAL);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_hospital"));
        assertTrue(Arrays.asList(types).contains("amenity_hospital_building"));
    }

    @Test
    void doctorsMapsToDoctorsType() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.DOCTORS);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("amenity_doctors"));
    }

    @Test
    void publicTransportMapsToPublicTransportTypes() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.PUBLIC_TRANSPORT);
        assertNotNull(types);
        assertTrue(Arrays.asList(types).contains("railway_station"));
        assertTrue(Arrays.asList(types).contains("railway_halt"));
        assertTrue(Arrays.asList(types).contains("railway_tram_stop"));
        assertTrue(Arrays.asList(types).contains("amenity_bus_station"));
        assertTrue(Arrays.asList(types).contains("public_transport_platform"));
        assertTrue(Arrays.asList(types).contains("railway_subway_entrance"));
    }

    @Test
    void getTypeNamesReturnsDefensiveCopy() {
        String[] types = PoiCategories.getTypeNames(PoiCategories.HOTELS);
        types[0] = "mutated";
        String[] again = PoiCategories.getTypeNames(PoiCategories.HOTELS);
        assertEquals("tourism_hotel", again[0]);
    }
}
