package com.framstag.libosmscout;

import com.framstag.libosmscout.client.LocationEntry;
import org.junit.jupiter.api.Test;

import static org.junit.jupiter.api.Assertions.*;

/**
 * The line that names the object type of a location search result.
 * <p>
 * The result carries the description of its object type in the language the client was configured
 * with. The line shows that description; when the database carries none, it falls back to the
 * technical type name, and a result without an object type renders no such line at all.
 */
public class SearchOverlayTest {

    private static LocationEntry entry(String objectTypeName, String objectTypeDescription) {
        LocationEntry entry = new LocationEntry();
        entry.label = "Am Birkenbaum";
        entry.type = "node";
        entry.objectTypeName = objectTypeName;
        entry.objectTypeDescription = objectTypeDescription;
        entry.objectFileOffset = 42;

        return entry;
    }

    @Test
    public void testTheDescriptionNamesTheObjectType() {
        String line = SearchOverlay.typeLine(entry("amenity_bank", "Bank"));

        assertEquals("   - node 42 Bank", line);
        assertFalse(line.contains("amenity_bank"), "the line must not show the technical type name");
    }

    @Test
    public void testAMissingDescriptionFallsBackToTheTypeName() {
        String line = SearchOverlay.typeLine(entry("amenity_bank", ""));

        assertEquals("   - node 42 amenity_bank", line);
    }

    @Test
    public void testANullDescriptionFallsBackToTheTypeName() {
        // A native library that does not set the field leaves it null.
        String line = SearchOverlay.typeLine(entry("amenity_bank", null));

        assertEquals("   - node 42 amenity_bank", line);
    }

    @Test
    public void testAResultWithoutAnObjectTypeRendersNoLine() {
        assertEquals("", SearchOverlay.typeLine(entry("", "")));
        assertEquals("", SearchOverlay.typeLine(entry(null, null)));
    }

    @Test
    public void testTheRefTypeFallsBackToThePlainWord() {
        LocationEntry withoutRefType = entry("highway_track", "Wirtschaftsweg");
        withoutRefType.type = null;

        assertEquals("   - object 42 Wirtschaftsweg", SearchOverlay.typeLine(withoutRefType));
    }
}
