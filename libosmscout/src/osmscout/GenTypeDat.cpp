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

#include <osmscout/GenTypeDat.h>

#include <osmscout/Util.h>

#include <osmscout/util/FileWriter.h>

namespace osmscout {

  std::string TypeDataGenerator::GetDescription() const
  {
    return "Generate 'types.dat'";
  }

  bool TypeDataGenerator::Import(const ImportParameter& parameter,
                                Progress& progress,
                                const TypeConfig& typeConfig)
  {
    //
    // Analysing distribution of nodes in the given interval size
    //

    progress.SetAction("Generate types.dat");

    FileWriter writer;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "types.dat"))) {
      progress.Error("Canot create 'types.dat'");
      return false;
    }

    writer.WriteNumber(typeConfig.GetTags().size());

    for (std::vector<TagInfo>::const_iterator tag=typeConfig.GetTags().begin();
         tag!=typeConfig.GetTags().end();
         ++tag) {
      writer.WriteNumber(tag->GetId());
      writer.Write(tag->GetName());
    }

    writer.WriteNumber(typeConfig.GetTypes().size());

    for (std::vector<TypeInfo>::const_iterator type=typeConfig.GetTypes().begin();
         type!=typeConfig.GetTypes().end();
         ++type) {
      writer.WriteNumber(type->GetId());
      writer.Write(type->GetName());
      writer.WriteNumber(type->GetTag());
      writer.Write(type->GetTagValue());
      writer.Write(type->CanBeNode());
      writer.Write(type->CanBeWay());
      writer.Write(type->CanBeArea());
      writer.Write(type->CanBeRelation());
      writer.Write(type->CanBeOverview());
      writer.Write(type->CanBeRoute());
      writer.Write(type->CanBeIndexed());
    }

    return !writer.HasError() && writer.Close();
  }
}

