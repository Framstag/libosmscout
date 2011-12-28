#ifndef OSMSCOUT_IMPORT_GENWAYDAT_H
#define OSMSCOUT_IMPORT_GENWAYDAT_H

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

#include <osmscout/Way.h>
#include <osmscout/NumericIndex.h>

#include <osmscout/import/Import.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

  class WayDataGenerator : public ImportModule
  {
  private:
    bool ReadRestrictionRelations(const ImportParameter& parameter,
                                  Progress& progress,
                                  const TypeConfig& typeConfig,
                                  std::map<Id,std::vector<Way::Restriction> >& restrictions);

    bool ReadWayEndpoints(const ImportParameter& parameter,
                          Progress& progress,
                          const TypeConfig& typeConfig,
                          std::map<Id,std::list<Id> >& endPointWayMap);

    bool ReadAreasIncludingEndpoints(const ImportParameter& parameter,
                                     Progress& progress,
                                     const TypeConfig& typeConfig,
                                     const std::map<Id,std::list<Id> >& endPointWayMap,
                                     std::set<Id>& endPointAreaSet);
    bool JoinWay(Progress& progress,
                 const TypeConfig& typeConfig,
                 FileScanner& scanner,
                 RawWay& rawWay,
                 std::map<Id,std::list<Id> >& endPointWayMap,
                 NumericIndex<Id,RawWay>& rawWayIndex,
                 std::set<Id>& wayBlacklist,
                 size_t& mergeCount);

  public:
    std::string GetDescription() const;
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };
}

#endif
