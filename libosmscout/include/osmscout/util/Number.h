#ifndef OSMSCOUT_NUMBER_H
#define OSMSCOUT_NUMBER_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/private/CoreImportExport.h>

#include <stddef.h>

namespace osmscout {

  extern OSMSCOUT_API bool EncodeNumber(unsigned long number,
                                        size_t bufferLength,
                                        char* buffer,
                                        size_t& bytes);


  template<typename N>
  bool DecodeNumber(const char* buffer, N& number, size_t& bytes)
  {
    N mult=0;

    number=0;
    bytes=1;

    // TODO: Assure that we do not read past the end of the buffer
    while (true) {
      N add=((*buffer) & 0x7f) << mult;

      number=number | add;

      if (((*buffer) & 0x80)==0) {
        return true;
      }

      bytes++;
      buffer++;
      mult+=7;
    }

    return true;
  }
}

#endif
