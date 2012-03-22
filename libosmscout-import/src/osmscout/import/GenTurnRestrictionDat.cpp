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

#include <osmscout/import/GenTurnRestrictionDat.h>

#include <osmscout/TurnRestriction.h>

#include <osmscout/Util.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawRelation.h>

namespace osmscout {

  std::string TurnRestrictionDataGenerator::GetDescription() const
  {
    return "Generate 'rawrestrictions.dat'";
  }

  bool TurnRestrictionDataGenerator::Import(const ImportParameter& parameter,
                                            Progress& progress,
                                            const TypeConfig& typeConfig)
  {
    uint32_t rawRelsCount=0;
    uint32_t restrictionsWrittenCount=0;

    progress.SetAction("Generating rawrestrictions.dat");

    FileScanner scanner;
    FileWriter  writer;

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "rawrels.dat"),
                                      true,true)) {
      progress.Error("Cannot open 'rawrels.dat'");
      return false;
    }

    if (!scanner.Read(rawRelsCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "rawrestrictions.dat"))) {
      progress.Error("Cannot create 'rawrestrictions.dat'");
      return false;
    }

    writer.Write(restrictionsWrittenCount);

    for (uint32_t r=1; r<=rawRelsCount; r++) {
      progress.SetProgress(r,rawRelsCount);

      RawRelation relation;

      if (!relation.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(r)+" of "+
                       NumberToString(rawRelsCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");
        return false;
      }

      bool                  isRestriction=false;
      bool                  isTurnRestriction=false;
      TurnRestriction::Type type=TurnRestriction::Allow;
      Id                    from=0;
      Id                    via=0;
      Id                    to=0;

      for (std::vector<Tag>::const_iterator tag=relation.tags.begin();
          tag!=relation.tags.end();
          tag++) {
        if (tag->key==typeConfig.tagType) {
          if (tag->value=="restriction") {
            isRestriction=true;
          }
        }
        else if (tag->key==typeConfig.tagRestriction) {
          if (tag->value=="only_left_turn" ||
              tag->value=="only_right_turn" ||
              tag->value=="only_straight_on") {
            isTurnRestriction=true;
            type=TurnRestriction::Allow;
          }
          else if (tag->value=="no_left_turn" ||
                   tag->value=="no_right_turn" ||
                   tag->value=="no_straight_on" ||
                   tag->value=="no_u_turn") {
            isTurnRestriction=true;
            type=TurnRestriction::Forbit;
          }
        }

        // finished collection data
        if (isRestriction &&
            isTurnRestriction) {
          break;
        }
      }

      if (!(isRestriction &&
            isTurnRestriction)) {
        continue;
      }

      for (std::vector<RawRelation::Member>::const_iterator member=relation.members.begin();
           member!=relation.members.end();
           ++member) {
        if (member->type==RawRelation::memberWay &&
            member->role=="from") {
          from=member->id;
        }
        else if (member->type==RawRelation::memberNode &&
            member->role=="via") {
          via=member->id;
        }
        else if (member->type==RawRelation::memberWay &&
            member->role=="to") {
          to=member->id;
        }

        // finished collection data
        if (from!=0 &&
            via!=0 &&
            to!=0) {
          break;
        }
      }

      if (from!=0 &&
          via!=0 &&
          to!=0) {
        TurnRestriction restriction(type,
                                    from,
                                    via,
                                    to);

        restriction.Write(writer);
        restrictionsWrittenCount++;
      }
    }

    if (!scanner.Close()) {
      return false;
    }

    writer.SetPos(0);
    writer.Write(restrictionsWrittenCount);

    if (!writer.Close()) {
      return false;
    }

    progress.Info(std::string("Read "+NumberToString(rawRelsCount)+" relations, wrote "+NumberToString(restrictionsWrittenCount)+" turn restrictions"));

    return true;
  }
}

