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

  class OSMSCOUT_MAP_OPENGL_API CharacterTexture {
  public:
    char32_t character;
    OpenGLTexture *texture;
    long baselineY;
    long height;

    CharacterTexture();

    CharacterTexture(char32_t character, OpenGLTexture *texture) {
      this->character = character;
      this->texture = texture;
    }

    char32_t GetCharacter() const {
      return character;
    }

    void SetCharacter(char32_t character) {
      this->character = character;
    }

    OpenGLTexture *GetTexture() const {
      return texture;
    }

    void SetTexture(OpenGLTexture *texture) {
      this->texture = texture;
    }

    long GetBaselineY() const {
      return baselineY;
    }

    void SetBaselineY(long baselineY) {
      this->baselineY = baselineY;
    }

    long GetHeight() const {
      return height;
    }

    void SetHeight(long height) {
      this->height = height;
    }

  };

  class OSMSCOUT_MAP_OPENGL_API TextLoader {
  private:
    FT_Library ft;
    FT_Face face;

    long defaultFontSize;

    long maxHeight;
    int sumWidth;

    std::map<std::pair<char32_t, int>, int> characterIndices;
    std::vector<osmscout::CharacterTexture *> characters;

    void LoadFace();

    void LoadFace(std::string);

  public:

    TextLoader(std::string path, long defaultSize);

    size_t GetWidth(int index);

    int GetStartWidth(int index);

    long GetHeight();

    long GetDefaultFontSize() const;

    void SetDefaultFontSize(long defaultFontSize);

    OpenGLTexture *CreateTexture();

    std::vector<int> AddCharactersToTextureAtlas(std::string text, double size);
  };
}


#endif //LIBOSMSCOUT_TEXTLOADER_H
