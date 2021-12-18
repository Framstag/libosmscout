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

#include <osmscoutmapopengl/TextLoader.h>

#include <iostream>
#include <locale>

#include <osmscout/util/String.h>

namespace osmscout {

  TextLoader::TextLoader(const std::string &path, long defaultSize, double dpi) {
    if (FT_Init_FreeType(&ft)) {
      log.Error() << "ERROR::FREETYPE: Could not init FreeType Library";
      return;
    }

    sumWidth = 0;
    maxHeight = 0;
    defaultFontSize = defaultSize;
    initialized=LoadFace(path, dpi);
  }

  std::vector<int> TextLoader::AddCharactersToTextureAtlas(std::string text, double size) {
    std::vector<int> indices;

    std::u32string utf32 = UTF8StringToU32String(text);

    for (char32_t &i : utf32) {

      int size_i = (int) size * defaultFontSize;
      std::pair<char32_t, int> p = std::pair<char32_t, int>(i, size_i);
      if (!(characterIndices.find(p) == characterIndices.end())) {
        int in = characterIndices.at(p);
        indices.push_back(in);
        continue;
      }

      FT_Set_Char_Size(face, (size_i) << 6, (size_i) << 6, 96, 96);

      FT_UInt glyph_index = FT_Get_Char_Index(face, i);
      if (FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER)) {
        std::cout << "Error while loading glyph" << std::endl;
        continue;
      }

      FT_Glyph gl;
      FT_Get_Glyph(face->glyph, &gl);
      FT_Glyph_To_Bitmap(&gl, ft_render_mode_normal, 0, 1);
      FT_BitmapGlyph bitg = (FT_BitmapGlyph) gl;
      FT_Bitmap &bit = bitg->bitmap;

      long h = abs(face->size->metrics.descender / 64) + (face->size->metrics.ascender / 64) + 1;
      OpenGLTextureRef texture(new osmscout::OpenGLTexture);
      //space or not space?
      if (i == 32) {
        unsigned char *spaceBitmap = new unsigned char[h * 4];
        for (int j = 0; j < h * 4; j++)
          spaceBitmap[j] = 0;
        texture->data = spaceBitmap;
        texture->width = 2;
        texture->height = h;
        texture->fromOriginY = 0;
        sumWidth += 2;
      } else {
        texture->data = new unsigned char[bit.width*bit.rows];
        for(size_t t = 0; t < bit.rows*bit.width; t++)
          texture->data[t] = bit.buffer[t];
        texture->width = bit.width;
        texture->height = bit.rows;
        texture->fromOriginY = face->glyph->bitmap_top;
        sumWidth += bit.width;
      }

      maxHeight = maxHeight < h ? h : maxHeight;

      CharacterTextureRef character(new osmscout::CharacterTexture());
      character->SetCharacter(i);
      character->SetTexture(texture);
      character->SetBaselineY(abs(face->size->metrics.descender / 64));
      character->SetHeight(h);
      characters.push_back(character);
      characterIndices.emplace(p, characters.size() - 1);
      indices.push_back(characters.size() - 1);

      FT_Done_Glyph(gl);
    }

    return indices;
  }

  OpenGLTextureRef TextLoader::CreateTexture() {

    auto*image = new unsigned char[maxHeight * sumWidth];

    for (long i = 0; i < maxHeight * sumWidth; i++) {
      image[i] = 0;
    }

    int index = 0;
    for (size_t i = 0; i < (size_t)maxHeight; i++) {
      for (auto& character : characters) {
        OpenGLTextureRef tx =character->GetTexture();
        size_t start = i * tx->width;
        for (size_t k = start; k < start + (tx->width); k++) {
          size_t start2 = maxHeight - (character->GetBaselineY() + tx->fromOriginY);
          size_t end = start2 + tx->height;
          if (i >= start2 && i < end) {
            size_t ind = k - (start2 * tx->width);
            image[index] = (tx->data[ind]);
            index++;
          } else {
            index++;
          }
        }
      }
    }

    OpenGLTextureRef result(new osmscout::OpenGLTexture());
    result->width = sumWidth;
    result->height = maxHeight;
    result->data = image;

    return result;
  }

  bool TextLoader::LoadFace(const std::string &path, double dpi) {
    if (!ExistsInFilesystem(path)) {
      log.Error() << "Font file " << path << " doesn't exists";
      return false;
    }

    std::string fontPath = path;

    if (FT_New_Face(ft, fontPath.c_str(), 0, &face)) {
      log.Error() << "ERROR::FREETYPE: Failed to load font from " << path;
      return false;
    }

    FT_Set_Char_Size(face, defaultFontSize << 6, defaultFontSize << 6, dpi, dpi);

    FT_Select_Charmap(
        face,
        FT_ENCODING_UNICODE);

    return true;
  }

  int TextLoader::GetStartWidth(int index) {
    int width = 0;
    for (int i = 0; i < index; i++) {
      width += characters[i]->GetTexture()->width;
    }

    return width;
  }

  size_t TextLoader::GetWidth(int index) {
    return characters[index]->GetTexture()->width;
  }

  long TextLoader::GetHeight() {
    return this->maxHeight;
  }

  long TextLoader::GetDefaultFontSize() const {
    return defaultFontSize;
  }

  void TextLoader::SetDefaultFontSize(long defaultFontSize) {
    this->defaultFontSize = defaultFontSize;
  }

  TextLoader::~TextLoader() {
    FT_Done_Face(face);
    FT_Done_FreeType(ft);
  }

}
