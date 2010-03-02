#ifndef OSMSCOUT_CITYSTREETINDEX_H
#define OSMSCOUT_CITYSTREETINDEX_H

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

#include <list>
#include <set>

#include <osmscout/AdminRegion.h>
#include <osmscout/Location.h>
#include <osmscout/TypeConfig.h>

#include <osmscout/FileScanner.h>

namespace osmscout {
  struct Loc
  {
    std::list<Id> nodes;
    std::list<Id> ways;
    std::list<Id> areas;
  };

  class CityStreetIndex
  {
  private:
    std::string            path;

    std::list<AdminRegion> areas;
    mutable FileOffset     region;
    mutable bool           regionLoaded;
    mutable std::map<std::string,Loc> locations;

    std::string            (*hashFunction)(std::string);

  private:
    bool LoadRegion(FileScanner& scanner) const;
    bool LoadRegion(FileOffset offset) const;

  public:
    CityStreetIndex();
    virtual ~CityStreetIndex();

    bool Load(const std::string& path,
              std::string (*hashFunction) (std::string) = NULL);

    bool GetMatchingAdminRegions(const std::string& name,
                                 std::list<AdminRegion>& regions,
                                 size_t limit,
                                 bool& limitReached,
                                 bool startWith) const;

    bool GetMatchingLocations(const AdminRegion& region,
                              const std::string& name,
                              std::list<Location>& locations,
                              size_t limit,
                              bool& limitReached,
                              bool startWith) const;

    void DumpStatistics();
  };
}

#endif
