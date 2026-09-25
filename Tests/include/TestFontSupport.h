#ifndef OSMSCOUT_TEST_TESTFONTSUPPORT_H
#define OSMSCOUT_TEST_TESTFONTSUPPORT_H

/*
  Support for tests that measure text with the font the repository ships.

  The Cairo backend resolves a font by family name through Pango, and Pango
  chooses its font map per platform: on Windows the Win32 font map is the
  default and it ignores fontconfig, so a font that is only registered with
  fontconfig stays invisible there (MSYS2 issue 4293). A test that hardcodes a
  family name therefore measures whatever font the host happens to provide, and
  its glyph metrics move when the host font set changes.

  These helpers read the family name out of the font file the repository ships,
  register that file with fontconfig, and select the fontconfig based Pango font
  map for this process when the build has one and the environment has not
  already chosen a font map. The measured font then comes from the repository on
  every platform.
*/

#include <filesystem>
#include <string>

#if defined(HAVE_LIB_FONTCONFIG)
#include <fontconfig/fontconfig.h>
#include <pango/pangocairo.h>
#include <glib.h>
#endif

#if defined(HAVE_LIB_FREETYPE)
#include <TextMetricsAll.h>
#endif

namespace osmscout {

  /**
   * Whether a font file's family name can be read in this build.
   */
  inline bool CanResolveFamilyFromFontFile()
  {
#if defined(HAVE_LIB_FREETYPE)
    return true;
#else
    return false;
#endif
  }

  /**
   * Read the family name stored inside the given font file.
   *
   * @param fontFile path to a font file
   * @param family the family name stored in the file
   * @param error error description on failure
   * @return true on success, false on failure
   */
  inline bool FamilyFromFontFile(const std::string& fontFile,
                                 std::string& family,
                                 std::string& error)
  {
#if defined(HAVE_LIB_FREETYPE)
    return TextMetricsAll::ReferenceFontFamily(fontFile,
                                               family,
                                               error);
#else
    (void)family;
    error="this build cannot read a font file (no FreeType)";

    return false;
#endif
  }

  /**
   * Make the given font file resolvable for the Pango/cairo stack and select
   * the fontconfig based Pango font map for this process when it is available
   * and the environment has not selected one.
   *
   * A no-op in builds without fontconfig.
   */
  inline void MakeFontFileResolvable(const std::string& fontFile)
  {
#if defined(HAVE_LIB_FONTCONFIG)
    FcConfigAppFontAddFile(nullptr,
                           reinterpret_cast<const FcChar8*>(fontFile.c_str()));

    if (g_getenv("PANGOCAIRO_BACKEND")==nullptr) {
      g_setenv("PANGOCAIRO_BACKEND","fc",TRUE);

      PangoFontMap *fontMap=pango_cairo_font_map_new();

      if (fontMap!=nullptr) {
        pango_cairo_font_map_set_default(PANGO_CAIRO_FONT_MAP(fontMap));
        g_object_unref(fontMap);
      } else {
        // This build has no fontconfig based cairo font map: keep the font map
        // the platform would choose.
        g_unsetenv("PANGOCAIRO_BACKEND");
      }
    }
#else
    (void)fontFile;
#endif
  }

  /**
   * Font name to configure a backend that resolves fonts by family name with,
   * given the font file (or family name) a test was started with.
   *
   * Returns the family name stored in the file and makes that file resolvable.
   * Keeps the input unchanged when it is not a file or when this build cannot
   * read a font file.
   *
   * @param fontFileOrFamily a font file path, or a family name
   * @param error error description when a font file could not be read
   * @return the font name to use
   */
  inline std::string FontNameForFamilyBackend(const std::string& fontFileOrFamily,
                                              std::string& error)
  {
    std::error_code ec;

    if (!CanResolveFamilyFromFontFile() || !std::filesystem::is_regular_file(fontFileOrFamily,ec)) {
      // a family name, or a build that cannot read the family out of a file
      return fontFileOrFamily;
    }

    std::string family;

    if (!FamilyFromFontFile(fontFileOrFamily,
                            family,
                            error)) {
      return "";
    }

    MakeFontFileResolvable(fontFileOrFamily);

    return family;
  }
}

#endif // OSMSCOUT_TEST_TESTFONTSUPPORT_H
