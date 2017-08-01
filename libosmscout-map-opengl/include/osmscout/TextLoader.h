#ifndef LIBOSMSCOUT_TEXTLOADER_H
#define LIBOSMSCOUT_TEXTLOADER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017  Fanny Monori

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

#include <string>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <osmscout/OpenGLMapData.h>
#include <osmscout/private/MapOpenGLImportExport.h>

namespace osmscout {

  class OSMSCOUT_MAP_OPENGL_API TextLoader {
  private:
    FT_Library ft;
    FT_Face face;

    long height;
    long baseLineY;
    int sumwidth;

    std::map<char32_t, int> characterIndices;
    std::vector<osmscout::OpenGLTexture *> characters;

    void LoadFace();

    void LoadFace(std::string);

  public:

    TextLoader(std::string path);

    int GetStartWidth(int index);

    long GetHeight();

    size_t GetWidth(int index);

    OpenGLTexture *CreateTexture();

    std::vector<int> AddCharactersToTextureAtlas(std::string text);

  };
}


#endif //LIBOSMSCOUT_TEXTLOADER_H
