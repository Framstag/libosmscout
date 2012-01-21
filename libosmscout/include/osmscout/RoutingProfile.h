#ifndef OSMSCOUT_ROUTINGPROFILE_H
#define OSMSCOUT_ROUTINGPROFILE_H

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

#include <vector>

#include <osmscout/StyleConfig.h>
#include <osmscout/TypeConfig.h>

namespace osmscout {

  class OSMSCOUT_API RoutingProfile
  {
  private:
    std::vector<double> costFactors;
    double              minCostFactor;
    double              maxCostFactor;
    double              turnCostFactor;

  public:
    RoutingProfile();

    void SetTurnCostFactor(double costFactor);

    void SetTypeCostFactor(TypeId type, double costFactor);

    inline double GetCostFactor(TypeId type) const
    {
      if (type>=costFactors.size()) {
        return 0;
      }
      else {
        return costFactors[type];
      }
    }

    inline bool CanUse(TypeId type) const
    {
      return type<costFactors.size() && costFactors[type]!=0.0;
    }

    inline double GetMinCostFactor() const
    {
      return minCostFactor;
    }

    inline double GetMaxCostFactor() const
    {
      return maxCostFactor;
    }

    inline double GetTurnCostFactor() const
    {
      return turnCostFactor;
    }
  };
}

#endif
