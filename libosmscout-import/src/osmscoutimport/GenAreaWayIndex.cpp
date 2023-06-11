/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

#include <osmscoutimport/GenAreaWayIndex.h>

#include <osmscout/Way.h>

#include <osmscout/db/AreaWayIndex.h>
#include <osmscout/db/WayDataFile.h>

namespace osmscout {

  AreaWayIndexGenerator::AreaWayIndexGenerator():
    AreaIndexGenerator<Way>("way",
                            "ways",
                            WayDataFile::WAYS_DAT,
                            AreaWayIndex::AREA_WAY_IDX)
  {
  }

  void AreaWayIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                              ImportModuleDescription& description) const
  {
    description.SetName("AreaWayIndexGenerator");
    description.SetDescription("Index ways for area lookup");

    description.AddRequiredFile(WayDataFile::WAYS_DAT);

    description.AddProvidedFile(AreaWayIndex::AREA_WAY_IDX);
  }

  void AreaWayIndexGenerator::WriteTypeId(const TypeConfigRef& typeConfig,
                                          const TypeInfoRef &type,
                                          FileWriter &writer) const
  {
    writer.WriteTypeId(type->GetWayId(),
                       typeConfig->GetWayTypeIdBytes());
  }

  bool AreaWayIndexGenerator::Import(const TypeConfigRef& typeConfig,
                                     const ImportParameter& parameter,
                                     Progress& progress)
  {
    return MakeAreaIndex(typeConfig,
                         parameter,
                         progress,
                         typeConfig->GetWayTypes(),
                         parameter.GetAreaWayMinMag(),
                         parameter.GetAreaWayIndexMaxLevel(),
                         parameter.GetWayDataMemoryMaped());
  }

}
