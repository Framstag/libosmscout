package com.framstag.libosmscout;

import java.util.Locale;
import java.util.Set;

/**
 * The languages the type descriptions of the client are available in.
 * <p>
 * A search result carries the description of its object type in the configured language. The
 * shipped type configuration carries German and English; a language it does not carry is answered
 * by the English description, and a type it does not describe at all is reported without a
 * description, in which case the app shows the technical type name.
 * <p>
 * The language of the app is derived from the locale of the platform and falls back to English,
 * which is the language the client reports by default.
 */
public final class DescriptionLanguage {

    /** German. */
    public static final String GERMAN = "de";

    /** English; the language the client falls back to. */
    public static final String ENGLISH = "en";

    /** The languages the shipped type configuration carries. */
    private static final Set<String> SUPPORTED = Set.of(GERMAN, ENGLISH);

    private DescriptionLanguage() {
    }

    /**
     * Return the description language for a locale.
     *
     * @param locale locale of the platform, may be null
     * @return a supported language code: {@link #GERMAN} for a German locale, otherwise
     *         {@link #ENGLISH}
     */
    public static String forLocale(Locale locale) {
        if (locale == null) {
            return ENGLISH;
        }

        String language = locale.getLanguage();

        return isSupported(language) ? language : ENGLISH;
    }

    /**
     * Return whether the shipped type configuration carries descriptions in a language.
     *
     * @param languageCode language code, may be null
     * @return true when the configuration carries descriptions in that language
     */
    public static boolean isSupported(String languageCode) {
        return languageCode != null && SUPPORTED.contains(languageCode);
    }

    /**
     * Return the other supported language, for a switch between the two.
     *
     * @param languageCode the current language code
     * @return {@link #GERMAN} when the given language is not German, otherwise {@link #ENGLISH}
     */
    public static String other(String languageCode) {
        return GERMAN.equals(languageCode) ? ENGLISH : GERMAN;
    }

    /**
     * Return the label for an object type: its description when the client reported one, the
     * technical type name otherwise.
     *
     * @param description description of the type in the client's language, may be null or empty
     * @param typeName technical type name, may be null
     * @return the description when there is one, the type name otherwise, or an empty string when
     *         neither is available
     */
    public static String typeLabel(String description, String typeName) {
        if (description != null && !description.isEmpty()) {
            return description;
        }

        return typeName != null ? typeName : "";
    }

    /**
     * Return the name of a language in that language.
     *
     * @param languageCode language code, e.g. {@link #GERMAN}
     * @return the name of the language, or the code itself when it is unknown
     */
    public static String label(String languageCode) {
        if (GERMAN.equals(languageCode)) {
            return "Deutsch";
        }

        if (ENGLISH.equals(languageCode)) {
            return "English";
        }

        return languageCode;
    }
}
