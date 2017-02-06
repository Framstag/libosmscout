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

  const char* MergeAreaDataGenerator::AREAS_TMP="areas.tmp";

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
    uint32_t    wayDataCount=0;
    uint32_t    relDataCount=0;
    uint32_t    dataWritten=0;

    try {
      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  AREAS_TMP));

      writer.Write(dataWritten);

      progress.SetAction("Copying areas from file 'wayarea.tmp'");

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   WayAreaDataGenerator::WAYAREA_TMP),
                   FileScanner::Sequential,
                   parameter.GetAreaDataMemoryMaped());

      scanner.Read(wayDataCount);

      for (uint32_t current=1; current<=wayDataCount; current++) {
        uint8_t type;
        OSMId   id;
        Area    data;

        progress.SetProgress(current,wayDataCount);

        scanner.Read(type);
        scanner.Read(id);
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

      scanner.Read(relDataCount);

      for (uint32_t current=1; current<=relDataCount; current++) {
        uint8_t type;
        OSMId   id;
        Area    data;

        progress.SetProgress(current,relDataCount);

        scanner.Read(type);
        scanner.Read(id);
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

      progress.Info("Merged "+NumberToString(wayDataCount)+ " ways and "+NumberToString(relDataCount)+" relations to "+
                    NumberToString(dataWritten)+" areas");
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
    if (!MergeAreas(parameter,
                    progress,
                    *typeConfig)) {
      return false;
    }

    return true;
  }
}
