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

  uint64_t InterleaveNumbers(uint32_t a,
                             uint32_t b)
  {
    uint64_t number=0;

    for (size_t i=0; i<32; i++) {
      size_t bit=31-i;

      number=number << 1;
      number=number+((a >> bit) & 0x01);

      number=number << 1;
      number=number+((b >> bit) & 0x01);
    }

    return number;
  }
}
