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

#include <cassert>

namespace osmscout {

  /**
   * Encode a number into the given buffer using some variable length encoding.
   *
   * The current implementation requires the buffer to have at least space
   * for sizeof(N)*8/7 bytes:
   *
   * This are 5 bytes for a 32bit value and 10 bytes for a64bit value.
   *
   * The methods returns the number of bytes written.
   */
  template<typename N>
  unsigned int OSMSCOUT_API EncodeNumber(N number,
                                         char* buffer)
  {
    unsigned int bytes=0;

    while (number>0x7f) {
      buffer[bytes]=((number & 0x7f) | 0x80);
      number=number >> 7;
      bytes++;
    }

    buffer[bytes]=number;
    bytes++;

    return bytes;
  }


  /**
   * Decode a variable length encoded number from the buffer back to
   * the variable.
   *
   * The methods returns the number of bytes read.
   */
  template<typename N>
  unsigned int OSMSCOUT_API DecodeNumber(const char* buffer,
                                         N& number)
  {
    unsigned int shift=0;
    unsigned int bytes=1;

    number=0;

    while (true) {
      number=number+(((*buffer) & 0x7f) << shift);

      if (((*buffer) & 0x80)==0) {
        return bytes;
      }

      bytes++;
      buffer++;
      shift+=7;
    }

    return bytes;
  }
}

#endif
