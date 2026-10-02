package com.framstag.libosmscout.client;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.BeforeEach;
import org.junit.jupiter.api.Test;
import org.junit.jupiter.api.io.TempDir;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import static org.junit.jupiter.api.Assertions.*;

/**
 * JNI integration tests for {@link OSMScoutClient#importGpxTrack(String)}.
 * <p>
 * These tests are skipped automatically when the native library is not available
 * (e.g. when running {@code mvn test} without {@code -Dnative.lib.dir}).
 * <p>
 * The {@code client} below is a receiver, not a subject: the native implementation of
 * the import does not read its receiver - it takes an ignored self argument and parses
 * the file itself - and it does not use the process-wide client either. A client
 * without a native handle is therefore sufficient here, and the availability probe is
 * released immediately rather than being reused as a client. Whatever this class
 * builds, it releases when the class ends.
 */
public class OSMScoutClientImportGpxTest {

    private static OSMScoutClient client;

    @BeforeEach
    public void setUp() {
        TestClients.assumeNativeLibrary();

        client = TestClients.receiverWithoutNativeHandle();
    }

    @AfterEach
    public void tearDown() {
        if (client != null) {
            client.close();
            client = null;
        }
    }

    @Test
    public void testImportValidGpx(@TempDir Path tmp) throws IOException {
        Path gpx = tmp.resolve("track.gpx");
        Files.writeString(gpx, """
            <?xml version="1.0" encoding="UTF-8"?>
            <gpx version="1.1" xmlns="http://www.topografix.com/GPX/1/1">
              <trk>
                <name>Test track</name>
                <trkseg>
                  <trkpt lat="51.514227" lon="7.465279">
                    <time>2024-01-01T10:00:00Z</time>
                  </trkpt>
                  <trkpt lat="51.515" lon="7.466"/>
                </trkseg>
              </trk>
            </gpx>
            """);

        TrackPoint[] points = client.importGpxTrack(gpx.toAbsolutePath().toString());

        assertNotNull(points);
        assertEquals(2, points.length, "first track with two points");
        assertEquals(51.514227, points[0].lat, 0.00001);
        assertEquals(7.465279, points[0].lon, 0.00001);
        assertNotNull(points[0].timestamp);
        assertTrue(points[0].timestamp.startsWith("2024-01-01T10:00:00"),
            "timestamp should start with ISO-8601 date/time: " + points[0].timestamp);
        assertEquals(51.515, points[1].lat, 0.00001);
        assertEquals(7.466, points[1].lon, 0.00001);
        assertNull(points[1].timestamp);
    }

    @Test
    public void testImportFirstTrackOnly(@TempDir Path tmp) throws IOException {
        Path gpx = tmp.resolve("multi.gpx");
        Files.writeString(gpx, """
            <?xml version="1.0" encoding="UTF-8"?>
            <gpx version="1.1" xmlns="http://www.topografix.com/GPX/1/1">
              <trk><name>First</name><trkseg>
                <trkpt lat="1.0" lon="2.0"/>
              </trkseg></trk>
              <trk><name>Second</name><trkseg>
                <trkpt lat="3.0" lon="4.0"/>
              </trkseg></trk>
            </gpx>
            """);

        TrackPoint[] points = client.importGpxTrack(gpx.toAbsolutePath().toString());

        assertNotNull(points);
        assertEquals(1, points.length);
        assertEquals(1.0, points[0].lat, 0.00001);
        assertEquals(2.0, points[0].lon, 0.00001);
    }

    @Test
    public void testImportEmptyGpx(@TempDir Path tmp) throws IOException {
        Path gpx = tmp.resolve("empty.gpx");
        Files.writeString(gpx, """
            <?xml version="1.0" encoding="UTF-8"?>
            <gpx version="1.1" xmlns="http://www.topografix.com/GPX/1/1">
            </gpx>
            """);

        TrackPoint[] points = client.importGpxTrack(gpx.toAbsolutePath().toString());

        assertNotNull(points);
        assertEquals(0, points.length);
    }

    @Test
    public void testImportMissingFile() {
        TrackPoint[] points = client.importGpxTrack("/nonexistent/path/does-not-exist.gpx");

        assertNotNull(points);
        assertEquals(0, points.length);
    }

    @Test
    public void testImportNullPath() {
        TrackPoint[] points = client.importGpxTrack(null);

        assertNotNull(points);
        assertEquals(0, points.length);
    }
}
