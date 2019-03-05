/*
  This source is part of the libosmscout library
  Copyright (C) 2018 Lukáš Karas

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

#include <osmscout/util/Distance.h>
#include <osmscout/system/Math.h>

namespace osmscout{

  std::string Distance::AsString() const
  {
    if (std::abs(meters) < 1500){
      return std::to_string(meters) + " m";
    }
    return std::to_string(As<Kilometer>()) + " km";
  }

  Distance Distance::Zero()
  {
    return Distance(0);
  }

  Distance Distance::Max()
  {
    return Distance(std::numeric_limits<double>::max());
  }

  Distance Distance::Min()
  {
    return Distance(std::numeric_limits<double>::min());
  }

  Distance Distance::Lowest()
  {
    return Distance(std::numeric_limits<double>::lowest());
  }

  Distance Distance::Max(const Distance &a, const Distance &b)
  {
    return Distance(std::max(a.meters, b.meters));
  }

  Distance Distance::Min(const Distance &a, const Distance &b)
  {
    return Distance(std::min(a.meters, b.meters));
  }

}
