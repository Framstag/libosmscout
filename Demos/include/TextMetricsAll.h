#ifndef DEMO_LIBOSMSCOUT_TEXTMETRICSALL_H
#define DEMO_LIBOSMSCOUT_TEXTMETRICSALL_H

/*
  TextMetricsAll - a part of demo programs for libosmscout
  Copyright (C) 2026  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <cmath>
#include <string>
#include <vector>

#if defined(HAVE_LIB_FREETYPE)
  #include <ft2build.h>
  #include FT_FREETYPE_H
#endif

namespace TextMetricsAll {

  /**
   * Per-glyph ink bounding box from the FreeType reference measurement.
   * Coordinates are relative to the glyph base point (left baseline origin),
   * in screen coordinates (y grows downwards).
   */
  struct ReferenceGlyph
  {
    double x{0.0};      //!< Ink box left edge, relative to glyph base point
    double y{0.0};      //!< Ink box top edge, relative to glyph base point
    double width{0.0};  //!< Ink box width
    double height{0.0}; //!< Ink box height
    double advance{0.0}; //!< Horizontal advance
  };

  /**
   * Reference measurement of a text: label dimensions and per-glyph
   * ink bounding boxes, computed directly via FreeType.
   */
  struct ReferenceMetrics
  {
    double                      width{0.0}; //!< Total advance width of the text
    double                      height{0.0}; //!< Font height
    std::vector<ReferenceGlyph> glyphs; //!< One entry per character
  };

  /**
   * Pixel size conversion shared by all backends:
   * fontSize * fontSizeParam * dpi / 25.4
   */
  inline double ReferencePixelSize(double fontSize,
                                   double fontSizeParam,
                                   double dpi)
  {
    return fontSize * fontSizeParam * dpi / 25.4;
  }

  /**
   * Measure the per-glyph ink bounding boxes of the given text with FreeType.
   * The pixel size is set from fontSize * fontSizeParam * dpi / 25.4,
   * matching the backends' font size conversion.
   * Only available when the build has FreeType (HAVE_LIB_FREETYPE).
   *
   * @param fontFile path to a font file loadable by FreeType
   * @param text UTF-8 text to measure
   * @param fontSize style font size
   * @param fontSizeParam map parameter font size
   * @param dpi rendering dpi
   * @param metrics output reference metrics
   * @param error error description on failure
   * @return true on success, false on failure
   */
#if defined(HAVE_LIB_FREETYPE)

  inline bool MeasureReference(const std::string& fontFile,
                               const std::string& text,
                               double fontSize,
                               double fontSizeParam,
                               double dpi,
                               ReferenceMetrics& metrics,
                               std::string& error)
  {
    FT_Library library;

    if (FT_Init_FreeType(&library) != 0) {
      error = "Cannot initialize FreeType";

      return false;
    }

    FT_Face face;

    if (FT_New_Face(library, fontFile.c_str(), 0, &face) != 0) {
      error = "Cannot load font face: " + fontFile;
      FT_Done_FreeType(library);

      return false;
    }

    long px = static_cast<long>(std::lround(ReferencePixelSize(fontSize, fontSizeParam, dpi)));

    if (px < 1) {
      px = 1;
    }

    if (FT_Set_Pixel_Sizes(face, static_cast<FT_UInt>(px), static_cast<FT_UInt>(px)) != 0) {
      error = "Cannot set pixel size";
      FT_Done_Face(face);
      FT_Done_FreeType(library);

      return false;
    }

    metrics = ReferenceMetrics{};
    metrics.height = static_cast<double>(face->size->metrics.height) / 64.0;

    double penX = 0.0;

    for (size_t i = 0; i < text.length();) {
      // Decode one UTF-8 character
      FT_ULong      codepoint;
      size_t        charLen = 1;
      unsigned char c = static_cast<unsigned char>(text[i]);

      if (c < 0x80) {
        codepoint = c;
      }
      else if ((c & 0xE0) == 0xC0) {
        codepoint = c & 0x1F;
        charLen = 2;
      }
      else if ((c & 0xF0) == 0xE0) {
        codepoint = c & 0x0F;
        charLen = 3;
      }
      else if ((c & 0xF8) == 0xF0) {
        codepoint = c & 0x07;
        charLen = 4;
      }
      else {
        codepoint = '?';
        charLen = 1;
      }
      for (size_t j = 1; j < charLen && i + j < text.length(); ++j) {
        codepoint = (codepoint << 6) | (static_cast<unsigned char>(text[i + j]) & 0x3F);
      }
      i += charLen;

      FT_UInt glyphIndex = FT_Get_Char_Index(face, codepoint);

      if (glyphIndex == 0) {
        // Missing glyph: use the .notdef glyph if available
        glyphIndex = FT_Get_Char_Index(face, 0);
      }

      if (FT_Load_Glyph(face, glyphIndex, FT_LOAD_RENDER) != 0) {
        continue;
      }

      ReferenceGlyph glyph;

      // 26.6 fixed point, /64. FreeType is y-up, screen is y-down.
      glyph.x = static_cast<double>(face->glyph->metrics.horiBearingX) / 64.0;
      glyph.y = -static_cast<double>(face->glyph->metrics.horiBearingY) / 64.0;
      glyph.width = static_cast<double>(face->glyph->metrics.width) / 64.0;
      glyph.height = static_cast<double>(face->glyph->metrics.height) / 64.0;
      glyph.advance = static_cast<double>(face->glyph->advance.x) / 64.0;

      metrics.glyphs.push_back(glyph);
      penX += glyph.advance;
    }

    metrics.width = penX;

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return true;
  }

  /**
   * Get the family name stored inside the given font file (FreeType face
   * family_name). Needed by backends that resolve fonts by family name via
   * fontconfig (Cairo backend): the font file name is not the family name,
   * and asking for a guessed name silently substitutes another font on
   * systems without that family installed.
   *
   * Only available when the build has FreeType (HAVE_LIB_FREETYPE).
   *
   * @param fontFile path to a font file loadable by FreeType
   * @param family the family name from the font file
   * @param error error description on failure
   * @return true on success, false on failure
   */
  inline bool ReferenceFontFamily(const std::string& fontFile,
                                  std::string& family,
                                  std::string& error)
  {
    FT_Library library;

    if (FT_Init_FreeType(&library) != 0) {
      error = "Cannot initialize FreeType";

      return false;
    }

    FT_Face face;

    if (FT_New_Face(library, fontFile.c_str(), 0, &face) != 0) {
      error = "Cannot load font face: " + fontFile;
      FT_Done_FreeType(library);

      return false;
    }

    family = face->family_name;

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return true;
  }

#endif // HAVE_LIB_FREETYPE
}

#endif //DEMO_LIBOSMSCOUT_TEXTMETRICSALL_H
