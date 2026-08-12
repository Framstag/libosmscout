package com.framstag.libosmscout;

import com.framstag.libosmscout.client.LocationEntry;
import org.junit.jupiter.api.Test;

import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;

class LocationSearchRankerTest {

    private static LocationEntry entry(String label, String type, String objectType,
                                       String matchQuality, double lat, double lon) {
        LocationEntry e = new LocationEntry();
        e.label = label;
        e.type = type;
        e.objectType = objectType;
        e.matchQuality = matchQuality;
        e.lat = lat;
        e.lon = lon;
        return e;
    }

    @Test
    void exactMatchRanksAbovePrefixMatch() {
        LocationEntry exact = entry("berlin", "object", "place_town", "match", 52.52, 13.40);
        LocationEntry prefix = entry("berlin-neukölln", "object", "place_town", "match", 52.48, 13.44);
        Comparator<LocationEntry> c = LocationSearchRanker.comparator("berlin", 52.52, 13.40);
        assertTrue(c.compare(exact, prefix) < 0, "exact match must rank first");
    }

    @Test
    void prefixMatchRanksAboveFuzzyMatch() {
        LocationEntry prefix = entry("berlinstrasse", "object", "address", "match", 52.52, 13.40);
        LocationEntry fuzzy = entry("alt-berlin", "object", "address", "match", 52.52, 13.40);
        Comparator<LocationEntry> c = LocationSearchRanker.comparator("berlin", 52.52, 13.40);
        assertTrue(c.compare(prefix, fuzzy) < 0, "prefix match must rank above fuzzy match");
    }

    @Test
    void candidateResultsSortAfterMatches() {
        LocationEntry match = entry("Dortmund", "object", "place_town", "match", 51.51, 7.46);
        LocationEntry candidate = entry("Dortmund", "object", "place_town", "candidate", 51.51, 7.46);
        Comparator<LocationEntry> c = LocationSearchRanker.comparator("Dortmund", 51.51, 7.46);
        assertTrue(c.compare(match, candidate) < 0, "match results must sort before candidates");
    }

    @Test
    void coordinateRanksFirst() {
        LocationEntry coord = entry("51.5, 7.4", "coordinate", null, "match", 51.5, 7.4);
        LocationEntry town = entry("Dortmund", "object", "place_town", "match", 51.51, 7.46);
        Comparator<LocationEntry> c = LocationSearchRanker.comparator("Dortmund", 51.51, 7.46);
        assertTrue(c.compare(coord, town) < 0, "coordinate results must rank first");
    }

    @Test
    void deduplicateCollapsesNearIdenticalFarAwayEntries() {
        LocationEntry a = entry("Am Birkenbaum", "object", "address", "match", 51.0, 7.0);
        LocationEntry b = entry("Am Birkenbaum", "object", "address", "match", 51.001, 7.001);
        // Search center far away (> 3000 m)
        List<LocationEntry> deduped = LocationSearchRanker.deduplicate(Arrays.asList(a, b), 52.0, 8.0);
        assertEquals(1, deduped.size(), "near-identical far-away entries must collapse");
    }

    @Test
    void deduplicateKeepsDistinctNearbyEntries() {
        LocationEntry a = entry("Am Birkenbaum", "object", "address", "match", 51.51, 7.46);
        LocationEntry b = entry("Am Birkenbaum", "object", "address", "match", 51.511, 7.461);
        // Search center close (< 3000 m)
        List<LocationEntry> deduped = LocationSearchRanker.deduplicate(Arrays.asList(a, b), 51.51, 7.46);
        assertEquals(2, deduped.size(), "entries close to the search center must be kept");
    }

    @Test
    void deduplicateKeepsDifferentTypes() {
        LocationEntry a = entry("Hauptstrasse", "object", "address", "match", 51.0, 7.0);
        LocationEntry b = entry("Hauptstrasse", "object", "highway_residential", "match", 51.001, 7.001);
        List<LocationEntry> deduped = LocationSearchRanker.deduplicate(Arrays.asList(a, b), 52.0, 8.0);
        assertEquals(2, deduped.size(), "entries of different object types must be kept");
    }
}
