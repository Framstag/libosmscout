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

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include <osmscout/OpenGLMapData.h>
#include <osmscout/private/MapOpenGLImportExport.h>

namespace osmscout {

  class OSMSCOUT_MAP_OPENGL_API CharacterTexture {
    char32_t character;
    OpenGLTextureRef texture;
    long baselineY;
    long height;

  public:

    char32_t GetCharacter() const {
      return character;
    }

    void SetCharacter(char32_t character) {
      this->character = character;
    }

    OpenGLTextureRef GetTexture() const {
      return texture;
    }

    void SetTexture(OpenGLTextureRef texture) {
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

  typedef std::shared_ptr<CharacterTexture> CharacterTextureRef;

  class OSMSCOUT_MAP_OPENGL_API TextLoader {
  private:
    FT_Library ft;
    FT_Face face;

    long defaultFontSize;

    long maxHeight;
    int sumWidth;

    std::map<std::pair<char32_t, int>, int> characterIndices;
    std::vector<osmscout::CharacterTextureRef> characters;

    void LoadFace();

    void LoadFace(std::string);

  public:

    ~TextLoader();

    TextLoader(std::string path, long defaultSize);

    /**
     * Returns width of a texture at given index in pixel.
     */
    size_t GetWidth(int index);

    /**
    * Returns the sum width of a texture at given index in pixel.
    */
    int GetStartWidth(int index);

    /**
     * Returns the height of the texture atlas in pixel.
     */
    long GetHeight();

    /**
     * Returns the default font size.
     */
    long GetDefaultFontSize() const;

    /**
     * Sets the default font size.
     */
    void SetDefaultFontSize(long defaultFontSize);

    /**
     * Creates one texture from the character textures.
     */
    OpenGLTextureRef CreateTexture();

    /**
     * Add new characters to the texture atlas and returns its indices in the atlas.
     */
    std::vector<int> AddCharactersToTextureAtlas(std::string text, double size);
  };
}


#endif //LIBOSMSCOUT_TEXTLOADER_H
