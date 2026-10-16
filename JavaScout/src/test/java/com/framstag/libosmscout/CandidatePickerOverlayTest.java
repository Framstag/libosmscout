package com.framstag.libosmscout;

import com.framstag.libosmscout.client.DescriptionEntry;
import com.framstag.libosmscout.client.ObjectDescription;
import org.junit.jupiter.api.Test;

import java.util.List;

import static org.junit.jupiter.api.Assertions.*;

/**
 * The entry a candidate picker shows for an object found on the map.
 * <p>
 * The candidate list is formatted like a search result, so it names the object type of a candidate
 * by its description and falls back to the technical type name when the database carries no
 * description for that type.
 */
public class CandidatePickerOverlayTest {

    private static ObjectDescription candidate(String objectTypeName,
                                               String objectTypeDescription,
                                               String... nameEntries) {
        DescriptionEntry typeEntry = new DescriptionEntry();
        typeEntry.sectionKey = "General";
        typeEntry.labelKey = "Type";
        typeEntry.value = objectTypeName;

        java.util.List<DescriptionEntry> entries = new java.util.ArrayList<>();
        entries.add(typeEntry);

        for (String name : nameEntries) {
            DescriptionEntry nameEntry = new DescriptionEntry();
            nameEntry.sectionKey = "General";
            nameEntry.labelKey = "Name";
            nameEntry.value = name;
            entries.add(nameEntry);
        }

        return new ObjectDescription(entries, 51.5, 7.4, "node", objectTypeName,
                                     objectTypeDescription, 4711);
    }

    @Test
    public void testADescribedCandidateNamesItsTypeByTheDescription() {
        ObjectDescription described = candidate("amenity_bank", "Bank");

        assertEquals("   - node 4711 Bank", CandidatePickerOverlay.candidateRefLine(described));
    }

    @Test
    public void testAnUndescribedCandidateFallsBackToTheTypeName() {
        ObjectDescription undescribed = candidate("amenity_bank", "");

        assertEquals("   - node 4711 amenity_bank", CandidatePickerOverlay.candidateRefLine(undescribed));
    }

    @Test
    public void testANullDescriptionFallsBackToTheTypeName() {
        // A native library that does not set the field leaves it null.
        ObjectDescription withoutField = candidate("amenity_bank", null);

        assertEquals("   - node 4711 amenity_bank", CandidatePickerOverlay.candidateRefLine(withoutField));
    }

    @Test
    public void testAnUnnamedCandidateIsNamedByItsTypeDescription() {
        assertEquals("Bank", CandidatePickerOverlay.candidateName(candidate("amenity_bank", "Bank")));
        assertEquals("amenity_bank",
            CandidatePickerOverlay.candidateName(candidate("amenity_bank", "")));
    }

    @Test
    public void testACandidateWithANameKeepsThatName() {
        ObjectDescription named = candidate("amenity_bank", "Bank", "Sparkasse");

        assertEquals("Sparkasse", CandidatePickerOverlay.candidateName(named));
    }
}
