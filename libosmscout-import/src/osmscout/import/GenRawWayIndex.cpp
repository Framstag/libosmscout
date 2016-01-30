/*
  This source is part of the libosmscout library
  Copyright (C) 2014  Tim Teulings

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

#include <osmscout/import/GenRawWayIndex.h>

#include <osmscout/import/Preprocess.h>

namespace osmscout {

  const char* RawWayIndexGenerator::RAWWAY_IDX="rawway.idx";

  RawWayIndexGenerator::RawWayIndexGenerator()
   : NumericIndexGenerator<OSMId,RawWay>("Generating 'rawway.idx'",
                                         Preprocess::RAWWAYS_DAT,
                                         RAWWAY_IDX)
  {
    // no code
  }

  void RawWayIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                             ImportModuleDescription& description) const
  {
    description.SetName("RawWayIndexGenerator");
    description.SetDescription("Generate id lookup index on raw way data file");

    description.AddRequiredFile(Preprocess::RAWWAYS_DAT);

    description.AddProvidedTemporaryFile(RAWWAY_IDX);
  }
}

