package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;
import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for DescriptionEntry model class.
 */
class DescriptionEntryTest {

    @Test
    void testDefaultConstructor() {
        DescriptionEntry entry = new DescriptionEntry();
        assertNull(entry.sectionKey);
        assertNull(entry.subsectionKey);
        assertFalse(entry.hasIndex);
        assertEquals(0, entry.index);
        assertNull(entry.labelKey);
        assertNull(entry.value);
    }

    @Test
    void testSetSectionKey() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.sectionKey = "General";
        assertEquals("General", entry.sectionKey);
    }

    @Test
    void testSetSubsectionKey() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.subsectionKey = "Lanes";
        assertEquals("Lanes", entry.subsectionKey);
    }

    @Test
    void testSetHasIndex() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.hasIndex = true;
        assertTrue(entry.hasIndex);
    }

    @Test
    void testSetIndex() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.index = 42;
        assertEquals(42, entry.index);
    }

    @Test
    void testSetLabelKey() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.labelKey = "Name";
        assertEquals("Name", entry.labelKey);
    }

    @Test
    void testSetValue() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.value = "Test Value";
        assertEquals("Test Value", entry.value);
    }

    @Test
    void testFullEntry() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.sectionKey = "ChargingStation";
        entry.subsectionKey = "Socket";
        entry.hasIndex = true;
        entry.index = 0;
        entry.labelKey = "Type";
        entry.value = "Type2";

        assertEquals("ChargingStation", entry.sectionKey);
        assertEquals("Socket", entry.subsectionKey);
        assertTrue(entry.hasIndex);
        assertEquals(0, entry.index);
        assertEquals("Type", entry.labelKey);
        assertEquals("Type2", entry.value);
    }

    @Test
    void testMultipleEntriesIndependent() {
        DescriptionEntry entry1 = new DescriptionEntry();
        entry1.sectionKey = "General";
        entry1.labelKey = "Type";
        entry1.value = "building";

        DescriptionEntry entry2 = new DescriptionEntry();
        entry2.sectionKey = "General";
        entry2.labelKey = "Name";
        entry2.value = "My Building";

        // Verify independence
        assertEquals("Type", entry1.labelKey);
        assertEquals("Name", entry2.labelKey);
        assertEquals("building", entry1.value);
        assertEquals("My Building", entry2.value);
    }
}
