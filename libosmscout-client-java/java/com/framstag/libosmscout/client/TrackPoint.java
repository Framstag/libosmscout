package com.framstag.libosmscout.client;

/**
 * A single track point imported from a GPX file.
 * <p>
 * Returned by {@link OSMScoutClient#importGpxTrack(String)}.
 */
public class TrackPoint {

    /** Latitude in degrees. */
    public double lat;

    /** Longitude in degrees. */
    public double lon;

    /**
     * Optional timestamp as an ISO-8601 string.
     * May be {@code null} when the source file does not provide timestamps.
     */
    public String timestamp;

    /** Default constructor. */
    public TrackPoint() {
    }

    /**
     * Construct a track point with the given coordinates.
     *
     * @param lat latitude in degrees
     * @param lon longitude in degrees
     */
    public TrackPoint(double lat, double lon) {
        this.lat = lat;
        this.lon = lon;
    }

    /**
     * Construct a track point with coordinates and timestamp.
     *
     * @param lat       latitude in degrees
     * @param lon       longitude in degrees
     * @param timestamp ISO-8601 timestamp string, or {@code null}
     */
    public TrackPoint(double lat, double lon, String timestamp) {
        this.lat = lat;
        this.lon = lon;
        this.timestamp = timestamp;
    }
}
