#ifndef OSMSCOUT_CITYSTREETINDEX_H
#define OSMSCOUT_CITYSTREETINDEX_H

/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <list>
#include <set>

#include <osmscout/City.h>
#include <osmscout/Street.h>
#include <osmscout/TypeConfig.h>

struct Urban
{
  Id                                   id;
  std::map<std::string,std::list<Id> > ways;
  std::map<std::string,std::list<Id> > areas;
};

class CityStreetIndex
{
private:
  std::string         path;
  std::list<City>     cities;
  std::map<Id,size_t> urbanOffsets;
  mutable Urban       urban;
  mutable bool        urbanLoaded;

private:
  bool LoadUrban(Id id) const;

public:
  CityStreetIndex();
  virtual ~CityStreetIndex();

  bool LoadCityStreetIndex(const std::string& path);

  bool GetMatchingCities(const std::string& name,
                         std::list<City>& cities,
                         size_t limit, bool& limitReached) const;
  bool GetMatchingStreets(Id urbanId, const std::string& name,
                          std::list<Street>& streets,
                          size_t limit, bool& limitReached) const;

  void DumpStatistics();
};

#endif
