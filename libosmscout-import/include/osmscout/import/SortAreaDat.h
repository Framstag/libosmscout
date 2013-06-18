#ifndef OSMSCOUT_IMPORT_SORTAREADAT_H
#define OSMSCOUT_IMPORT_SORTAREADAT_H

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

#include <osmscout/Area.h>

namespace osmscout {

  class SortAreaDataGenerator : public SortDataGenerator<Area>
  {
  private:
    void GetTopLeftCoordinate(const Area& data,
                              double& maxLat,
                              double& minLon)
    {
      bool start=true;
      for (size_t r=0; r<data.rings.size(); r++) {
        if (data.rings[r].ring==Area::outerRingId) {
          for (size_t n=1; n<data.rings[r].nodes.size(); n++) {
            if (start) {
              maxLat=data.rings[r].nodes[n].GetLat();
              minLon=data.rings[r].nodes[n].GetLon();

              start=false;
            }
            else {
              maxLat=std::max(maxLat,data.rings[r].nodes[n].GetLat());
              minLon=std::min(minLon,data.rings[r].nodes[n].GetLon());
            }
          }
        }
      }
    }

  public:
    SortAreaDataGenerator()
    : SortDataGenerator<Area>("areas.dat","areas.idmap")
    {
      AddSource(OSMRefType::osmRefWay,"wayarea.dat");
      AddSource(OSMRefType::osmRefRelation,"relarea.dat");
    }

    std::string GetDescription() const
    {
      return "Sort/copy areas";
    }
  };

}

#endif
