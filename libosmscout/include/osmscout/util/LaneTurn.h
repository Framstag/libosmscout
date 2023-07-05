#ifndef OSMSCOUT_UTIL_LANE_TURN_H
#define OSMSCOUT_UTIL_LANE_TURN_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2023  Lukas Karas

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

#include <cstdint>
#include <string>

#include <osmscout/lib/CoreImportExport.h>

namespace osmscout {

  /**
   * \defgroup LaneTurn lane turn helpers
   *
   * Collection of utilities for dealing with route lane turn variants.
   */

  /**
   * \ingroup LaneTurn
   *
   * Common lane turn variants.
   *
   * Note: Numeric values of variants are used for db serialization,
   * do not change them without increasing db format version.
   * Just append new variants to the end.
   *
   * Note: There is no need to export Enums
   */
  enum class LaneTurn: std::uint8_t {
    Null = 0,
    None = 1,
    Left = 2,
    MergeToLeft = 3,
    SlightLeft = 4,
    SharpLeft = 5,
    Through_Left = 6,
    Through_SlightLeft = 7,
    Through_SharpLeft = 8,
    Through = 9,
    Through_Right = 10,
    Through_SlightRight = 11,
    Through_SharpRight = 12,
    Right = 13,
    MergeToRight = 14,
    SlightRight = 15,
    SharpRight = 16,
    Unknown = 17,
  };

  std::string OSMSCOUT_API LaneTurnString(LaneTurn turn);
}

#endif //OSMSCOUT_UTIL_LANE_TURN_H
