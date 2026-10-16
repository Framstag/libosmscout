package com.framstag.libosmscout;

import com.framstag.libosmscout.client.DescriptionEntry;
import com.framstag.libosmscout.client.ObjectDescription;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * The title of the dialog that describes an object the user selected on the map.
 * <p>
 * The dialog names the object's type by its description in the language the client was configured
 * with, falls back to the technical type name when the database carries no description for that
 * type, and keeps a generic title for an object whose type is unknown.
 */
public class DescriptionOverlayTest {

    private static ObjectDescription described(String objectTypeName, String objectTypeDescription) {
        DescriptionEntry typeEntry = new DescriptionEntry();
        typeEntry.sectionKey = "General";
        typeEntry.labelKey = "Type";
        typeEntry.value = objectTypeName;

        return new ObjectDescription(List.of(typeEntry), 51.5, 7.4, "node", objectTypeName,
                                     objectTypeDescription, 4711);
    }

    @Test
    public void testTheTitleNamesTheTypeByItsDescription() {
        assertEquals("Bank", DescriptionOverlay.dialogTitle(described("amenity_bank", "Bank")));
    }

    @Test
    public void testAMissingDescriptionFallsBackToTheTypeName() {
        assertEquals("amenity_bank",
            DescriptionOverlay.dialogTitle(described("amenity_bank", "")));
    }

    @Test
    public void testANullDescriptionFallsBackToTheTypeName() {
        // A native library that does not set the field leaves it null.
        assertEquals("amenity_bank",
            DescriptionOverlay.dialogTitle(described("amenity_bank", null)));
    }

    @Test
    public void testAnObjectWithoutATypeKeepsTheGenericTitle() {
        assertEquals("Object Description", DescriptionOverlay.dialogTitle(described(null, null)));
        assertEquals("Object Description",
            DescriptionOverlay.dialogTitle(new ObjectDescription(List.of())));
    }
}
