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

  TextLoader::TextLoader(std::string path) {
    if (FT_Init_FreeType(&ft))
      std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

    sumwidth = 0;
    if(path.empty())
      LoadFace();
    else
      LoadFace(path);
  }

  std::vector<int> TextLoader::AddCharactersToTextureAtlas(std::string text) {
    std::vector<int> indices;

    std::u32string utf32=UTF8StringToU32String(text);

    for (char32_t &i : utf32) {

      if (!(characterIndices.find(i) == characterIndices.end())) {
        indices.push_back(characterIndices[i]);
        continue;
      }

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

      OpenGLTexture *texture = new osmscout::OpenGLTexture;
      //space or not space?
      if(i == 32) {
        unsigned char* spaceBitmap = new unsigned char[height*2];
        for(int i = 0; i < height*2; i++)
          spaceBitmap[i] = 0;
        texture->data = spaceBitmap;
        texture->width = 2;
        texture->height = height;
        texture->fromOriginY = 0;
        sumwidth += 2;
      }
      else{
        texture->data = bit.buffer;
        texture->width = bit.width;
        texture->height = bit.rows;
        texture->fromOriginY = face->glyph->bitmap_top;
        sumwidth += bit.width;
      }

      this->characters.push_back(texture);
      characterIndices.emplace(i, characters.size() - 1);
      indices.push_back(characters.size() - 1);
    }

    return indices;
  }

  OpenGLTexture *TextLoader::CreateTexture() {
    unsigned char *image = new unsigned char[sumwidth * this->height];

    int index = 0;
    for (int i = 0; i < this->height; i++) {
      for (unsigned int j = 0; j < characters.size(); j++) {
        int start = i * characters[j]->width;
        for (unsigned int k = start; k < start + (characters[j]->width); k++) {
          size_t start2 = this->height - (baseLineY + characters[j]->fromOriginY);
          size_t end = start2 + characters[j]->height;
          if (i >= start2 && i < end) {
            int ind = k - (start2 * characters[j]->width);
            image[index] = (characters[j]->data[ind]);
            index++;
          } else {
            image[index] = 0;
            index++;
          }
        }
      }
    }

    OpenGLTexture *result = new OpenGLTexture;
    result->width = sumwidth;
    result->height = height;
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

    FT_Set_Char_Size(face, 10 << 6, 10 << 6, 96, 96);

    FT_Select_Charmap(
        face,
        FT_ENCODING_UNICODE);

    height =  abs(face->size->metrics.descender / 64) + ( face->size->metrics.ascender  / 64);
    baseLineY = abs(face->size->metrics.descender / 64);
  }

  void TextLoader::LoadFace(std::string path) {
    std::string fontPath = path;
    const char *pathCstr = fontPath.c_str();

    if (FT_New_Face(ft, pathCstr, 0, &face)) {
      std::cout << "ERROR::FREETYPE: Failed to load font from path. Loading default font." << std::endl;
      LoadFace();
      return;
    }

    FT_Set_Char_Size(face, 10 << 6, 10 << 6, 96, 96);

    FT_Select_Charmap(
        face,
        FT_ENCODING_UNICODE);

    height =  abs(face->size->metrics.descender / 64) + ( face->size->metrics.ascender  / 64) + 1;
    baseLineY = abs(face->size->metrics.descender / 64);
  }

  int TextLoader::GetStartWidth(int index) {
    int width = 0;
    for (int i = 0; i < index; i++) {
      width += characters[i]->width;
    }

    return width;
  }

  size_t TextLoader::GetWidth(int index) {
    return characters[index]->width;
  }

  long TextLoader::GetHeight(){
    return this->height;
  }

}
