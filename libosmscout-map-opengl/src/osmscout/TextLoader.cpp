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

#include <osmscout/TextLoader.h>

#include <iostream>
#include <locale>

namespace osmscout {

  TextLoader::TextLoader(std::string path, long defaultSize) {
    if (FT_Init_FreeType(&ft))
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    sumWidth = 0;
    maxHeight = 0;
    defaultFontSize = defaultSize;
    if (path.empty())
      LoadFace();
    else
      LoadFace(path);
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

    unsigned char *image = new unsigned char[maxHeight * sumWidth];
    for (int i = 0; i < maxHeight * sumWidth; i++) {
      image[i] = 0;
    }

    int index = 0;
    for (int i = 0; i < maxHeight; i++) {
      for (unsigned int j = 0; j < characters.size(); j++) {
        OpenGLTextureRef tx = characters[j]->GetTexture();
        int start = i * tx->width;
        for (unsigned int k = start; k < start + (tx->width); k++) {
          size_t start2 = maxHeight - (characters[j]->GetBaselineY() + tx->fromOriginY);
          size_t end = start2 + tx->height;
          if (i >= start2 && i < end) {
            int ind = k - (start2 * tx->width);
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

  void TextLoader::LoadFace() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft))
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    std::string filePath = std::string(__FILE__);

#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
    std::string dirPath = filePath.substr(0, filePath.rfind("\\"));
#else
    std::string dirPath = filePath.substr(0, filePath.rfind("/src"));
#endif

    //Liberation Sans is licensed under SIL Open Font License (OFL)
    std::string fontPath = dirPath + "/data/fonts/LiberationSans-Regular.ttf";
    const char *path = fontPath.c_str();

    if (FT_New_Face(ft, path, 0, &face))
      std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

    FT_Set_Char_Size(face, defaultFontSize << 6, defaultFontSize << 6, 96, 96);

    FT_Select_Charmap(
        face,
        FT_ENCODING_UNICODE);

  }

  void TextLoader::LoadFace(std::string path) {
    std::string fontPath = path;
    const char *pathCstr = fontPath.c_str();

    if (FT_New_Face(ft, pathCstr, 0, &face)) {
      std::cout << "ERROR::FREETYPE: Failed to load font from path. Loading default font." << std::endl;
      LoadFace();
      return;
    }

    FT_Set_Char_Size(face, defaultFontSize << 6, defaultFontSize << 6, 96, 96);

    FT_Select_Charmap(
        face,
        FT_ENCODING_UNICODE);

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
    FT_Done_Face    ( face );
    FT_Done_FreeType( ft );
  }

}
