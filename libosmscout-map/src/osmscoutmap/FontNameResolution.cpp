/*
  This source is part of the libosmscout-map library
  Copyright (C) 2026  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <osmscoutmap/FontNameResolution.h>

#include <filesystem>
#include <string>
#include <system_error>

#include <osmscoutmap/MapFeatures.h>

#include <osmscout/log/Logger.h>

#if defined(OSMSCOUT_MAP_HAVE_LIB_FREETYPE)
  #include <ft2build.h>
  #include FT_FREETYPE_H
#endif

namespace osmscout {

  bool FontNameResolution::CanReadFamilyFromFile()
  {
#if defined(OSMSCOUT_MAP_HAVE_LIB_FREETYPE)
    return true;
#else
    return false;
#endif
  }

  bool FontNameResolution::ReadFamilyFromFile(const std::string& fontFile,
                                              std::string& family,
                                              std::string& error)
  {
    family.clear();
    error.clear();

#if defined(OSMSCOUT_MAP_HAVE_LIB_FREETYPE)
    FT_Library library;

    if (FT_Init_FreeType(&library)!=0) {
      error="cannot initialize FreeType";

      return false;
    }

    FT_Face face;

    if (FT_New_Face(library,
                    fontFile.c_str(),
                    0,
                    &face)!=0) {
      error="cannot load a font face from the file";
      FT_Done_FreeType(library);

      return false;
    }

    if (face->family_name==nullptr) {
      error="the font face reports no family name";
      FT_Done_Face(face);
      FT_Done_FreeType(library);

      return false;
    }

    family=face->family_name;

    FT_Done_Face(face);
    FT_Done_FreeType(library);

    return !family.empty();
#else
    (void)fontFile;

    error="this build cannot read a font file";

    return false;
#endif
  }

  FontNameResolution::Result FontNameResolution::Resolve(const std::string& configuredName)
  {
    Result result;

    result.fontName=configuredName;

    if (configuredName.empty()) {
      return result;
    }

    std::error_code errorCode;

    if (!std::filesystem::is_regular_file(configuredName,
                                          errorCode)) {
      // A font family name, which the text stack of the backend resolves itself
      return result;
    }

    // A font file: name the face it holds, so that an interface resolving by family serves that
    // face instead of falling back to whatever the host provides
    result.fontFile=configuredName;

    if (!CanReadFamilyFromFile()) {
      // Nothing to read the family with. The file is still the face to serve, so only the name
      // stays as it was configured.
      log.Warn() << "The configured font file '" << configuredName
                 << "' is used as configured, this build cannot read the family it holds";

      return result;
    }

    std::string family;
    std::string error;

    if (!ReadFamilyFromFile(configuredName,
                            family,
                            error)) {
      log.Error() << "Cannot read the family of the configured font file '" << configuredName
                  << "': " << error;

      return result;
    }

    result.fontName=family;

    return result;
  }
} // namespace osmscout
