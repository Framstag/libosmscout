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

#include <osmscout/RoutingProfile.h>

#include <cassert>

namespace osmscout {

  RoutingProfile::RoutingProfile()
   : minCostFactor(0),
     turnCostFactor(0)
  {
    // no code
  }

  void RoutingProfile::SetTurnCostFactor(double costFactor)
  {
    turnCostFactor=costFactor;
  }

  void RoutingProfile::SetTypeCostFactor(TypeId type, double costFactor)
  {
    if (costFactors.size()==0) {
      minCostFactor=costFactor;
    }
    else {
      minCostFactor=std::min(minCostFactor,costFactor);
    }

    if (type>=costFactors.size()) {
      costFactors.resize(type+1,0.0);
    }

    costFactors[type]=costFactor;
  }
}

