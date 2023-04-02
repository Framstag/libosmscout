/*
  This source is part of the libosmscout library
  Copyright (C) 2023 Lukas Karas

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

#include <osmscout/util/LaneTurn.h>

#include <string>

namespace osmscout {

std::string LaneTurnString(LaneTurn turn)
{
  switch (turn) {
    case LaneTurn::None:
      return "none";
    case LaneTurn::Left:
      return "left";
    case LaneTurn::MergeToLeft:
      return "merge_to_left";
    case LaneTurn::SlightLeft:
      return "slight_left";
    case LaneTurn::SharpLeft:
      return "sharp_left";
    case LaneTurn::Through_Left:
      return "through;left";
    case LaneTurn::Through_SlightLeft:
      return "through;slight_left";
    case LaneTurn::Through_SharpLeft:
      return "through;sharp_left";
    case LaneTurn::Through:
      return "through";
    case LaneTurn::Through_Right:
      return "through;right";
    case LaneTurn::Through_SlightRight:
      return "through;slight_right";
    case LaneTurn::Through_SharpRight:
      return "through;sharp_right";
    case LaneTurn::Right:
      return "right";
    case LaneTurn::MergeToRight:
      return "merge_to_right";
    case LaneTurn::SlightRight:
      return "slight_right";
    case LaneTurn::SharpRight:
      return "sharp_right";
    case LaneTurn::Unknown:
      return "unknown";
    default:
      return "unknown";
  }
}

}
