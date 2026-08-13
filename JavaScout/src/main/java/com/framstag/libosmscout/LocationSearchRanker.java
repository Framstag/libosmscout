package com.framstag.libosmscout;

import com.framstag.libosmscout.client.LocationEntry;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.Objects;

/**
 * Ranking and deduplication for location search results.
 * <p>
 * Mirrors OSMScout2's {@code locationRank}: {@code typeRank * distanceRank * matchRank},
 * with coordinate entries always ranked first, plus deduplication of near-identical
 * results (same object type, less than 300 m apart, more than 3000 m from the
 * search center).
 */
public final class LocationSearchRanker {

    private LocationSearchRanker() {
    }

    /**
     * Build a comparator that sorts search results by relevance.
     *
     * @param pattern   the search query (used for the match-quality boost)
     * @param centerLat map center latitude for distance ranking
     * @param centerLon map center longitude for distance ranking
     */
    public static Comparator<LocationEntry> comparator(String pattern,
                                                       double centerLat,
                                                       double centerLon) {
        return new LocationEntryComparator(pattern, centerLat, centerLon);
    }

    /**
     * Deduplicate near-identical results: same object type, less than 300 m apart
     * and more than 3000 m from the search center.
     */
    public static List<LocationEntry> deduplicate(List<LocationEntry> entries,
                                                  double centerLat,
                                                  double centerLon) {
        List<LocationEntry> result = new ArrayList<>();
        for (LocationEntry entry : entries) {
            boolean duplicate = false;
            for (LocationEntry kept : result) {
                double d = haversine(kept.lat, kept.lon, entry.lat, entry.lon);
                double dCenter = haversine(centerLat, centerLon, entry.lat, entry.lon);
                if (Objects.equals(kept.objectType, entry.objectType) &&
                        d < 300.0 && dCenter > 3000.0) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                result.add(entry);
            }
        }
        return result;
    }

    /**
     * Format a distance in meters as a kilometer string for display.
     * <p>
     * Sub-10 km distances keep one decimal place so nearby results are
     * distinguishable; larger distances round to whole kilometers.
     *
     * @param meters distance in meters
     * @return formatted value with "km" unit suffix, e.g. "0.5 km" or "12 km"
     */
    public static String formatDistanceKm(double meters) {
        double km = meters / 1000.0;
        if (km < 10.0) {
            return String.format(Locale.ROOT, "%.1f km", km);
        }
        return String.format(Locale.ROOT, "%.0f km", km);
    }

    public static double haversine(double lat1, double lon1,
                                   double lat2, double lon2) {
        double dLat = Math.toRadians(lat2 - lat1);
        double dLon = Math.toRadians(lon2 - lon1);
        double a = Math.sin(dLat / 2) * Math.sin(dLat / 2)
                 + Math.cos(Math.toRadians(lat1)) * Math.cos(Math.toRadians(lat2))
                 * Math.sin(dLon / 2) * Math.sin(dLon / 2);
        double c = 2 * Math.atan2(Math.sqrt(a), Math.sqrt(1 - a));
        return 6371000 * c;
    }

    /**
     * Comparator for sorting search results by relevance.
     * Matches OSMScout2's locationRank: typeRank * distanceRank * matchRank,
     * with coordinate entries always ranked first.
     */
    private static class LocationEntryComparator implements Comparator<LocationEntry> {

        private final String pattern;
        private final double centerLat;
        private final double centerLon;

        LocationEntryComparator(String pattern, double centerLat, double centerLon) {
            this.pattern = pattern != null ? pattern.toLowerCase(Locale.ROOT) : "";
            this.centerLat = centerLat;
            this.centerLon = centerLon;
        }

        @Override
        public int compare(LocationEntry a, LocationEntry b) {
            // Primary: match quality (match > candidate)
            boolean aMatch = "match".equals(a.matchQuality);
            boolean bMatch = "match".equals(b.matchQuality);
            if (aMatch != bMatch) {
                return aMatch ? -1 : 1;
            }
            // Secondary: relevance rank
            double rankA = computeRank(a);
            double rankB = computeRank(b);
            return Double.compare(rankB, rankA);
        }

        private double computeRank(LocationEntry entry) {
            // Coordinate/GPS results always rank first (OSMScout2 rank 1)
            if ("coordinate".equals(entry.type)) {
                return 1.0;
            }

            double typeRank = switch (entry.objectType != null ? entry.objectType : "") {
                case "boundary_country" -> 1.0;
                case "boundary_state" -> 0.93;
                case "boundary_administrative", "place_town" -> 0.9;
                case "highway_residential", "address" -> 0.8;
                case "railway_station", "railway_tram_stop",
                     "railway_subway_entrance", "highway_bus_stop" -> 0.7;
                default -> 0.5;
            };

            double distance = haversine(centerLat, centerLon, entry.lat, entry.lon);
            double distanceRank = 1.0 / Math.log((distance / 1000.0) + Math.E);

            // Match quality boost: exact label match > prefix match > fuzzy
            double matchRank = 0.5;
            if (pattern.isEmpty()) {
                matchRank = 1.0;
            } else if (entry.label != null) {
                String label = entry.label.toLowerCase(Locale.ROOT);
                if (label.equals(pattern)) {
                    matchRank = 1.0;
                } else if (label.startsWith(pattern)) {
                    matchRank = 0.75;
                }
            }

            return typeRank * distanceRank * matchRank;
        }
    }
}
