#ifndef OSMSCOUT_IMPORT_SORTRELATIONDAT_H
#define OSMSCOUT_IMPORT_SORTRELATIONDAT_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/import/SortDat.h>

#include <osmscout/Relation.h>

namespace osmscout {

  class SortRelationDataGenerator : public SortDataGenerator<Relation>
  {
  private:
    void GetTopLeftCoordinate(const Relation& data,
                              double& maxLat,
                              double& minLon)
    {
      assert(!data.roles.empty());
      assert(!data.roles[0].nodes.empty());

      maxLat=data.roles[0].nodes[0].GetLat();
      minLon=data.roles[0].nodes[0].GetLon();

      for (std::vector<Relation::Role>::const_iterator role=data.roles.begin();
           role!=data.roles.end();
           ++role) {
        for (size_t i=0; i<role->nodes.size(); i++) {
          maxLat=std::max(maxLat,role->nodes[i].GetLat());
          minLon=std::min(minLon,role->nodes[i].GetLon());
        }
      }
    }


  public:
    SortRelationDataGenerator()
    : SortDataGenerator("relations.dat","relation.idmap","relations.tmp")
    {
      // no code
    }

    std::string GetDescription() const
    {
      return "Sort/copy relations";
    }
  };

}

#endif
