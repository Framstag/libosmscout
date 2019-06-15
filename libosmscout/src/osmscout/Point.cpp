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

#include <osmscout/Point.h>

namespace osmscout {

  Id Point::GetId() const
  {
    Id id;

    uint64_t latValue=(uint64_t)round((coord.GetLat()+90.0)*latConversionFactor);
    uint64_t lonValue=(uint64_t)round((coord.GetLon()+180.0)*lonConversionFactor);

    id=((latValue & 0x000000ffu) <<  8u)+  // 0 => 8
       ((lonValue & 0x000000ffu) <<  0u)+  // 0 => 0
       ((latValue & 0x0000ff00u) << 16u)+  // 8 => 24
       ((lonValue & 0x0000ff00u) <<  8u)+  // 8 => 16
       ((latValue & 0x00ff0000u) << 24u)+  // 16 => 40
       ((lonValue & 0x00ff0000u) << 16u)+  // 16 => 32
       ((latValue & 0x07000000u) << 27u)+  // 24 => 51
       ((lonValue & 0x07000000u) << 24u);  // 24 => 48

    id=id << 8u;

    id|=serial;

    return id;
  }

  GeoCoord Point::GetCoordFromId(Id id)
  {
    id=id >> 8u;

    uint64_t latValue=((id >> 8u) & 0xffu) + (((id >> 24u) & 0xffu) << 8u) + (((id >> 40u) & 0xffu) << 16u) + (((id >> 51u) & 0x07u) << 24u);
    uint64_t lonValue=((id >> 0u) & 0xffu) + (((id >> 16u) & 0xffu) << 8u) + (((id >> 32u) & 0xffu) << 16u) + (((id >> 48u) & 0x07u) << 24u);

    return GeoCoord(latValue/latConversionFactor-90.0,
                    lonValue/lonConversionFactor-180.0);
  }
}
