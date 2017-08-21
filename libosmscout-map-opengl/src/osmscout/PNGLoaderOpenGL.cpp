/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings
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

#include <osmscout/PNGLoaderOpenGL.h>

#include <cstdio>
#include <cstdlib>

#include <png.h>

namespace osmscout {

  static bool IsLowerByteSet(unsigned char *bytes)
  {
    return bytes[0]!=0;
  }

  osmscout::OpenGLTextureRef LoadPNGOpenGL(const std::string& filename){
    std::FILE       *file;
    png_structp     png_ptr;
    png_infop       info_ptr;
    png_uint_32     width, height;
    int             bit_depth,color_type;
    int             channels,intent;
    double          screen_gamma;
    png_uint_32     rowbytes;
    png_bytepp      row_pointers=NULL;
    unsigned char   *image_data=NULL;
    unsigned char   *data;
    bool            littleEndian;

    int endian=1;

    littleEndian=IsLowerByteSet((unsigned char*)&endian);

    // open the file
    file=std::fopen(filename.c_str(),"rb");

    if (file==NULL) {
      return NULL;
    }

    // could pass pointers to user-defined error handlers instead of NULLs:

    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
    if (!png_ptr) {
      std::fclose(file);
      return NULL;   // out of memory
    }

    info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
      png_destroy_read_struct(&png_ptr,NULL,NULL);
      std::fclose(file);
      return NULL;   // out of memory
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
      std::fclose(file);
      return NULL;
    }

    png_init_io(png_ptr,file);
    png_read_info(png_ptr,info_ptr);

    png_get_IHDR(png_ptr,info_ptr,&width,&height,&bit_depth,&color_type,
                 NULL,NULL,NULL);

    if (setjmp(png_jmpbuf(png_ptr))) {
      png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
      std::fclose(file);
      return NULL;
    }

    // We always want RGB or RGBA
    if (color_type==PNG_COLOR_TYPE_PALETTE) {
      png_set_expand(png_ptr);
    }

    if (color_type==PNG_COLOR_TYPE_GRAY && bit_depth<8) {
      png_set_expand(png_ptr);
    }

    if (png_get_valid(png_ptr,info_ptr,PNG_INFO_tRNS)) {
      png_set_expand(png_ptr);
    }

    if (bit_depth==16) {
      png_set_strip_16(png_ptr);
    }

    if (color_type==PNG_COLOR_TYPE_GRAY ||
        color_type==PNG_COLOR_TYPE_GRAY_ALPHA) {
      png_set_gray_to_rgb(png_ptr);
    }

    screen_gamma=2.2; // TODO: Make it configurable

    if (png_get_sRGB(png_ptr,info_ptr,&intent)) {
      png_set_gamma(png_ptr,screen_gamma,0.45455);
    }
    else {
      double image_gamma;
      if (png_get_gAMA(png_ptr,info_ptr,&image_gamma)) {
        png_set_gamma(png_ptr,screen_gamma,image_gamma);
      }
      else {
        png_set_gamma(png_ptr,screen_gamma,0.45455);
      }
    }

    png_read_update_info(png_ptr, info_ptr);

    rowbytes=png_get_rowbytes(png_ptr,info_ptr);
    channels=(int)png_get_channels(png_ptr,info_ptr);

    if ((image_data=(unsigned char *)malloc(rowbytes*height)) == NULL) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      std::fclose(file);
      return NULL;
    }

    if ((row_pointers=(png_bytepp)malloc(height*sizeof(png_bytep))) == NULL) {
      png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
      free(image_data);
      std::fclose(file);
      return NULL;
    }

    for (size_t i=0;  i<height; ++i) {
      row_pointers[i]=image_data+i*rowbytes;
    }

    png_read_image(png_ptr,row_pointers);

    free(row_pointers);
    row_pointers=NULL;

    png_read_end(png_ptr,NULL);

    int stride = width*4;
    data=(unsigned char *)malloc(stride*height);

    size_t off=0;
    size_t s=0;
    while (s<width*height*channels) {
      unsigned int alpha;
      unsigned char red;
      unsigned char green;
      unsigned char blue;

      red=image_data[s];
      s++;
      green=image_data[s];
      s++;
      blue=image_data[s];
      s++;

      if (channels==4) {
        alpha=image_data[s];
        s++;
      }
      else {
        alpha=255;
      }

      red=red*alpha/256;
      green=green*alpha/256;
      blue=blue*alpha/256;

      if (littleEndian) {
      data[off]=red;
      off++;
      data[off]=green;
      off++;
      data[off]=blue;
      off++;
      data[off]=alpha;
      off++;
      }
      else {
        data[off]=alpha;
        off++;
        data[off]=red;
        off++;
        data[off]=green;
        off++;
        data[off]=blue;
        off++;
      }
    }

    png_destroy_read_struct(&png_ptr,&info_ptr,NULL);
    free(image_data);
    std::fclose(file);

    osmscout::OpenGLTextureRef texture(new osmscout::OpenGLTexture());
    texture->width = width;
    texture->height = height;
    texture->data = data;

    return texture;
  }

}
