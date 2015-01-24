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
#include <osmscout/util/HashSet.h>

#include <osmscout/DataFile.h>

namespace osmscout {

  std::string MergeAreaDataGenerator::GetDescription() const
  {
    return "Merge area data files";
  }

  bool MergeAreaDataGenerator::MergeAreas(const ImportParameter& parameter,
                                          Progress& progress,
                                          const TypeConfig& typeConfig)
  {
    FileScanner scanner;
    FileWriter  writer;
    uint32_t    dataCount=0;
    uint32_t    dataWritten=0;

    if (!writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "areas.tmp"))) {
      progress.Error("Cannot create '" + writer.GetFilename() + "'");
      return false;
    }

    writer.Write(dataWritten);

    progress.SetAction("Copying areas from file 'wayarea.tmp'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "wayarea.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=dataCount; current++) {
      uint8_t type;
      Id      id;
      Area    data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !data.Read(typeConfig,
                     scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!writer.Write(type) ||
          !writer.Write(id) ||
          !data.Write(typeConfig,
                      writer)) {
        progress.Error(std::string("Error while writing data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " from file '"+
                       scanner.GetFilename()+"' to '"+writer.GetFilename()+"'");
      }

      dataWritten++;
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    progress.SetAction("Copying areas from file 'relarea.tmp'");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "relarea.tmp"),
                                      FileScanner::Sequential,
                                      parameter.GetAreaDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    for (size_t current=1; current<=dataCount; current++) {
      uint8_t type;
      Id      id;
      Area    data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(type) ||
          !scanner.Read(id) ||
          !data.Read(typeConfig,
                     scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!writer.Write(type) ||
          !writer.Write(id) ||
          !data.Write(typeConfig,
                      writer)) {
        progress.Error(std::string("Error while writing data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " from file '"+
                       scanner.GetFilename()+"' to '"+writer.GetFilename()+"'");
      }

      dataWritten++;
    }

    if (!scanner.Close()) {
      progress.Error(std::string("Error while closing file '")+
                     scanner.GetFilename()+"'");
      return false;
    }

    if (!writer.SetPos(0)) {
      return false;
    }

    if (!writer.Write(dataWritten)) {
      return false;
    }

    return writer.Close();
  }

  bool MergeAreaDataGenerator::Import(const TypeConfigRef& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    if (!MergeAreas(parameter,
                    progress,
                    typeConfig)) {
      return false;
    }

    return true;
  }
}
