#ifndef OSMSCOUT_IMPORT_GENWATERINDEX_H
#define OSMSCOUT_IMPORT_GENWATERINDEX_H

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

#include <list>

#include <osmscout/import/Import.h>
#include <osmscout/import/WaterIndexProcessor.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  /**
   * Generator that calculates land, water and coast tiles based on costline data
   * and the assumption that land is always left of the coast (in line direction)
   * and water is always right.
   */
  class OSMSCOUT_IMPORT_API WaterIndexGenerator CLASS_FINAL : public ImportModule
  {
  private:
    bool LoadRawBoundaries(const ImportParameter& parameter,
                           Progress& progress,
                           std::list<WaterIndexProcessor::CoastRef>& coastlines,
                           const char* rawFile,
                           WaterIndexProcessor::CoastState leftState,
                           WaterIndexProcessor::CoastState rightState);

    bool LoadCoastlines(const ImportParameter& parameter,
                        Progress& progress,
                        std::list<WaterIndexProcessor::CoastRef>& coastlines);

    bool LoadBoundingPolygons(const ImportParameter& parameter,
                              Progress& progress,
                              std::list<WaterIndexProcessor::CoastRef>& boundingPolygons);

    void MergeCoastlines(Progress& progress,
                         std::list<WaterIndexProcessor::CoastRef>& coastlines);

    bool AssumeLand(const ImportParameter& parameter,
                    Progress& progress,
                    const TypeConfig& typeConfig,
                    const WaterIndexProcessor& processor,
                    WaterIndexProcessor::StateMap& stateMap);

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };
}

#endif
