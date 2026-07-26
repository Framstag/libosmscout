package com.framstag.libosmscout.client;

import org.junit.jupiter.api.Test;
import java.util.List;
import static org.junit.jupiter.api.Assertions.*;

/**
 * Unit tests for ObjectDescription model class.
 */
class ObjectDescriptionTest {

    @Test
    void testEmptyDescription() {
        ObjectDescription desc = new ObjectDescription(null);
        assertTrue(desc.getEntries().isEmpty(), "null list should produce empty entries");
    }

    @Test
    void testEmptyList() {
        ObjectDescription desc = new ObjectDescription(List.of());
        assertTrue(desc.getEntries().isEmpty(), "empty list should produce empty entries");
    }

    @Test
    void testSingleEntry() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.sectionKey = "General";
        entry.labelKey = "Type";
        entry.value = "building";

        ObjectDescription desc = new ObjectDescription(List.of(entry));
        assertEquals(1, desc.getEntries().size());
        assertEquals("General", desc.getEntries().get(0).sectionKey);
        assertEquals("Type", desc.getEntries().get(0).labelKey);
        assertEquals("building", desc.getEntries().get(0).value);
    }

    @Test
    void testMultipleEntries() {
        DescriptionEntry entry1 = new DescriptionEntry();
        entry1.sectionKey = "General";
        entry1.labelKey = "Type";
        entry1.value = "building";

        DescriptionEntry entry2 = new DescriptionEntry();
        entry2.sectionKey = "General";
        entry2.labelKey = "Name";
        entry2.value = "My Building";

        ObjectDescription desc = new ObjectDescription(List.of(entry1, entry2));
        assertEquals(2, desc.getEntries().size());
        assertEquals("Type", desc.getEntries().get(0).labelKey);
        assertEquals("Name", desc.getEntries().get(1).labelKey);
    }

    @Test
    void testUnmodifiableEntries() {
        DescriptionEntry entry = new DescriptionEntry();
        entry.sectionKey = "Test";
        entry.labelKey = "Key";
        entry.value = "Value";

        ObjectDescription desc = new ObjectDescription(List.of(entry));
        assertThrows(UnsupportedOperationException.class, () -> {
            desc.getEntries().add(new DescriptionEntry());
        });
    }

    @Test
    void testEntriesOrderPreserved() {
        DescriptionEntry first = new DescriptionEntry();
        first.sectionKey = "First";
        first.labelKey = "A";
        first.value = "1";

        DescriptionEntry second = new DescriptionEntry();
        second.sectionKey = "Second";
        second.labelKey = "B";
        second.value = "2";

        DescriptionEntry third = new DescriptionEntry();
        third.sectionKey = "Third";
        third.labelKey = "C";
        third.value = "3";

        ObjectDescription desc = new ObjectDescription(List.of(first, second, third));
        assertEquals(3, desc.getEntries().size());
        assertEquals("A", desc.getEntries().get(0).labelKey);
        assertEquals("B", desc.getEntries().get(1).labelKey);
        assertEquals("C", desc.getEntries().get(2).labelKey);
    }
}
