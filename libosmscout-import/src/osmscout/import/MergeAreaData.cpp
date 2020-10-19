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

#include <osmscout/import/MergeAreaData.h>

#include <osmscout/util/FileScanner.h>
#include <osmscout/util/FileWriter.h>

#include <osmscout/import/GenRelAreaDat.h>
#include <osmscout/import/GenWayAreaDat.h>

namespace osmscout {

  const char* const MergeAreaDataGenerator::AREAS_TMP="areas.tmp";

  void MergeAreaDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                            ImportModuleDescription& description) const
  {
    description.SetName("MergeAreaDataGenerator");
    description.SetDescription("Merge relation and way area data files into one area file");

    description.AddRequiredFile(WayAreaDataGenerator::WAYAREA_TMP);
    description.AddRequiredFile(RelAreaDataGenerator::RELAREA_TMP);

    description.AddProvidedTemporaryFile(AREAS_TMP);
  }

  bool MergeAreaDataGenerator::MergeAreas(const ImportParameter& parameter,
                                          Progress& progress,
                                          const TypeConfig& typeConfig)
  {
    FileScanner scanner;
    FileWriter  writer;

    try {
      uint32_t dataWritten=0;

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AREAS_TMP));

      writer.Write(dataWritten);

      progress.SetAction("Copying areas from file 'wayarea.tmp'");

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayAreaDataGenerator::WAYAREA_TMP),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      uint32_t wayDataCount=scanner.ReadUInt32();

      for (uint32_t current=1; current<=wayDataCount; current++) {
        Area    data;

        progress.SetProgress(current,wayDataCount);

        uint8_t type=scanner.ReadUInt8();
        OSMId   id=scanner.ReadInt64();

        data.ReadImport(typeConfig,
                        scanner);

        writer.Write(type);
        writer.Write(id);
        data.WriteImport(typeConfig,
                         writer);

        dataWritten++;
      }

      scanner.Close();

      progress.SetAction("Copying areas from file 'relarea.tmp'");

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   RelAreaDataGenerator::RELAREA_TMP),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      uint32_t relDataCount=scanner.ReadUInt32();

      for (uint32_t current=1; current<=relDataCount; current++) {
        Area    data;

        progress.SetProgress(current,relDataCount);

        uint8_t type=scanner.ReadUInt8();
        OSMId   id=scanner.ReadInt64();

        data.ReadImport(typeConfig,
                        scanner);

        writer.Write(type);
        writer.Write(id);
        data.WriteImport(typeConfig,
                         writer);

        dataWritten++;
      }

      scanner.Close();

      writer.GotoBegin();
      writer.Write(dataWritten);

      writer.Close();

      progress.Info("Merged "+std::to_string(wayDataCount)+ " ways and "+std::to_string(relDataCount)+" relations to "+
                    std::to_string(dataWritten)+" areas");
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      writer.CloseFailsafe();

      return false;
    }

    return true;
  }

  bool MergeAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    return MergeAreas(parameter,
                    progress,
                    *typeConfig);
  }
}
