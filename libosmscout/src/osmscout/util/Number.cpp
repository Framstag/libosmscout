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

#include <osmscout/util/Number.h>

namespace osmscout {

  bool EncodeNumber(unsigned long number,
                    size_t bufferLength,
                    char* buffer,
                    size_t& bytes)
  {
    if (number==0) {
      if (bufferLength==0) {
        return false;
      }

      buffer[0]=0;
      bytes=1;
      return true;
    }
    else {
      bytes=0;

      while (number!=0) {
        char          byte;
        unsigned long rest;

        byte=number & 0xff;
        rest=number >> 8;

        if (rest!=0 || (byte & 0x80)!=0) {
          // If we have something to encode or the high bit is set
          // we need an additional byte and can only decode 7 bit
          byte=byte & 0x7f; // Mask out the lower 7 bytes
          byte=byte| 0x80;  // set the 8th byte to signal that more bytes will follow
          number=number >> 7;

          if (bufferLength==0) {
            return false;
          }

          buffer[bytes]=byte;
          bytes++;
          bufferLength--;
        }
        else {
          if (bufferLength==0) {
            return false;
          }

          number=0; // we are finished!

          buffer[bytes]=byte;
          bytes++;
          bufferLength--;
        }
      }
    }

    return true;
  }
}
