package com.framstag.libosmscout;

import org.junit.jupiter.api.Test;

import java.util.Locale;

import static org.junit.jupiter.api.Assertions.*;

/**
 * The language of the type descriptions the app asks the client for.
 * <p>
 * The language follows the locale of the platform and falls back to English, which is also the
 * language the client reports by default. Only the languages the shipped type configuration
 * carries are asked for.
 */
public class DescriptionLanguageTest {

    @Test
    public void testGermanLocaleAsksForGerman() {
        assertEquals("de", DescriptionLanguage.forLocale(Locale.GERMAN));
        assertEquals("de", DescriptionLanguage.forLocale(Locale.GERMANY));
        assertEquals("de", DescriptionLanguage.forLocale(Locale.forLanguageTag("de-AT")));
        assertEquals("de", DescriptionLanguage.forLocale(Locale.forLanguageTag("de-CH")));
    }

    @Test
    public void testEnglishLocaleAsksForEnglish() {
        assertEquals("en", DescriptionLanguage.forLocale(Locale.ENGLISH));
        assertEquals("en", DescriptionLanguage.forLocale(Locale.UK));
        assertEquals("en", DescriptionLanguage.forLocale(Locale.US));
    }

    @Test
    public void testUnsupportedLocaleFallsBackToEnglish() {
        assertEquals("en", DescriptionLanguage.forLocale(Locale.FRENCH));
        assertEquals("en", DescriptionLanguage.forLocale(Locale.JAPANESE));
        assertEquals("en", DescriptionLanguage.forLocale(Locale.forLanguageTag("cs-CZ")));
        assertEquals("en", DescriptionLanguage.forLocale(null));
    }

    @Test
    public void testOnlyTheShippedLanguagesAreSupported() {
        assertTrue(DescriptionLanguage.isSupported("de"));
        assertTrue(DescriptionLanguage.isSupported("en"));
        assertFalse(DescriptionLanguage.isSupported("fr"));
        assertFalse(DescriptionLanguage.isSupported(""));
        assertFalse(DescriptionLanguage.isSupported(null));
    }

    @Test
    public void testTheSwitchTogglesBetweenTheTwoLanguages() {
        assertEquals("en", DescriptionLanguage.other("de"));
        assertEquals("de", DescriptionLanguage.other("en"));
        assertEquals("de", DescriptionLanguage.other("fr"));
        assertEquals("de", DescriptionLanguage.other(null));
    }

    @Test
    public void testALanguageIsNamedInItself() {
        assertEquals("Deutsch", DescriptionLanguage.label("de"));
        assertEquals("English", DescriptionLanguage.label("en"));
        assertEquals("fr", DescriptionLanguage.label("fr"));
    }
}
