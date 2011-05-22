#ifndef OSMSCOUT_GENAREAINDEX_H
#define OSMSCOUT_GENAREAINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <map>
#include <set>

#include <osmscout/Import.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/Geometry.h>

namespace osmscout {

  class AreaIndexGenerator : public ImportModule
  {
  private:
    struct AreaLeaf
    {
      FileOffset            offset;
      std::list<FileOffset> areas;
      std::list<FileOffset> relAreas;
      FileOffset            children[4];

      AreaLeaf()
      {
        offset=0;
        children[0]=0;
        children[1]=0;
        children[2]=0;
        children[3]=0;
      }
    };

  private:
    bool LoadWayBlacklist(const ImportParameter& parameter,
                          Progress& progress,
                          std::set<Id>& wayBlacklist);

    void SetOffsetOfChildren(const std::map<Coord,AreaLeaf>& leafs,
                             std::map<Coord,AreaLeaf>& newAreaLeafs);

    bool WriteIndexLevel(const ImportParameter& parameter,
                         FileWriter& writer,
                         int level,
                         std::map<Coord,AreaLeaf>& leafs);


  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
