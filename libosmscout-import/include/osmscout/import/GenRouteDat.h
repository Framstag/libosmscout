#ifndef OSMSCOUT_IMPORT_GENROUTEDAT_H
#define OSMSCOUT_IMPORT_GENROUTEDAT_H

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

#include <osmscout/NumericIndex.h>
#include <osmscout/Way.h>

#include <osmscout/import/Import.h>

namespace osmscout {

  class RouteDataGenerator : public ImportModule
  {
  private:
    enum RestrictionType
    {
      allow,
      forbit
    };

    struct Restriction
    {
      RestrictionType type;
      Id              from;
      Id              to;
    };

  private:
    bool ReadRestrictionRelations(const ImportParameter& parameter,
                                  Progress& progress,
                                  const TypeConfig& typeConfig,
                                  std::map<Id,std::vector<Restriction> >& restrictions);

    bool CanTurn(const std::vector<Restriction>& restrictions,
                 Id from,
                 Id to) const;

    bool ReadJunctions(const ImportParameter& parameter,
                       Progress& progress,
                       const TypeConfig& typeConfig,
                       std::set<Id>& junctions);

    bool ReadWayEndpoints(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          const std::set<Id>& junctions,
                          std::map<Id,std::list<Id> >& endPointWayMap);

    bool LoadWays(Progress& progress,
                  FileScanner& scanner,
                  NumericIndex<Id,Way>& wayIndex,
                  const std::set<Id>& ids,
                  std::list<WayRef>& ways);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
