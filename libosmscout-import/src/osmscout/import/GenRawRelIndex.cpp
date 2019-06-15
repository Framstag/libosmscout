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

#include <osmscout/import/GenRawRelIndex.h>

#include <osmscout/import/Preprocess.h>

namespace osmscout {

  const char* const RawRelationIndexGenerator::RAWREL_IDX="rawrel.idx";

  RawRelationIndexGenerator::RawRelationIndexGenerator()
   : NumericIndexGenerator<OSMId,RawRelation>("Generating 'rawrel.idx'",
                                              Preprocess::RAWRELS_DAT,
                                              RAWREL_IDX)
  {
    // no code
  }

  void RawRelationIndexGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                                 ImportModuleDescription& description) const
  {
    description.SetName("RawRelationIndexGenerator");
    description.SetDescription("Generate id lookup index on raw relation data file");

    description.AddRequiredFile(Preprocess::RAWRELS_DAT);

    description.AddProvidedTemporaryFile(RAWREL_IDX);
  }
}

