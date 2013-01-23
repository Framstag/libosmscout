/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/import/SortWayDat.h>

#include <osmscout/DataFile.h>

#include <osmscout/util/StopClock.h>

#include <osmscout/Way.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  std::string SortWayDataGenerator::GetDescription() const
  {
    return "Sort/copy ways";
  }

  bool SortWayDataGenerator::RenumberWays(const ImportParameter& parameter,
                                          Progress& progress)
  {
    FileScanner scanner;
    FileWriter  wayWriter;
    FileWriter  mapWriter;
    uint32_t    waysCount;
    uint32_t    waysCopyiedCount=0;
    double      zoomLevel=pow(2.0,(double)parameter.GetRenumberMag());
    size_t      cellCount=zoomLevel*zoomLevel;
    size_t      minIndex=0;
    size_t      maxIndex=cellCount-1;

    progress.SetAction("Sorting ways");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.tmp"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.tmp'");
      return false;
    }

    if (!scanner.Read(waysCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "ways.dat"))) {
      progress.Error("Cannot create 'ways.dat'");
      return false;
    }

    wayWriter.Write(waysCount);

    if (!mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "way.idmap"))) {
      progress.Error("Cannot create 'way.idmap'");
      return false;
    }

    mapWriter.Write(waysCount);

    while (true) {
      progress.Info("Reading ways in cell range "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex)+" from 'ways.tmp'");

      if (!scanner.GotoBegin()) {
        progress.Error(std::string("Error while setting current position in file '")+
                       scanner.GetFilename()+"'");
      }

      if (!scanner.Read(waysCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      uint32_t                              current=1;
      size_t                                currentEntries=0;
      std::map<size_t,std::list<WayEntry> > waysByCell;

      while (current<=waysCount) {
        Id  wayId;
        Way way;

        progress.SetProgress(current,waysCount);

        if (!scanner.Read(wayId)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(waysCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");

          return false;
        }

        if (!way.Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(waysCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        double minLat=way.nodes[0].GetLat();
        double minLon=way.nodes[0].GetLon();

        for (size_t n=1; n<way.nodes.size(); n++) {
          minLat=std::min(minLat,way.nodes[n].GetLat());
          minLon=std::min(minLon,way.nodes[n].GetLon());
        }

        size_t cellY=(size_t)(minLat+90.0/zoomLevel);
        size_t cellX=(size_t)(minLon+180.0/zoomLevel);
        size_t cellIndex=cellY*zoomLevel+cellX;

        if (cellIndex>=minIndex &&
            cellIndex<=maxIndex) {
          waysByCell[cellIndex].push_back(WayEntry(wayId,way.GetFileOffset()));
          currentEntries++;
        }

        // Reduce cell interval,deleting all already stored nodes beyond the new
        // cell range end.
        if (currentEntries>parameter.GetRenumberBlockSize()) {
          size_t                                          count=0;
          std::map<size_t,std::list<WayEntry> >::iterator cutOff=waysByCell.end();

          for (std::map<size_t,std::list<WayEntry> >::iterator iter=waysByCell.begin();
              iter!=waysByCell.end();
              ++iter) {
            if (count<=parameter.GetRenumberBlockSize() &&
                count+iter->second.size()>parameter.GetRenumberBlockSize()) {
              cutOff=iter;
              break;
            }
            else {
              count+=iter->second.size();
              maxIndex=iter->first;
            }
          }

          assert(cutOff!=waysByCell.end());

          currentEntries=count;
          waysByCell.erase(cutOff,waysByCell.end());

          progress.Debug("Reducing cell range to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
        }

        current++;
      }

      if (maxIndex<cellCount-1) {
        progress.Info("Cell range was reduced to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
      }

      progress.Info("Copy renumbered ways to 'ways.dat");

      size_t copyCount=0;
      for (std::map<size_t,std::list<WayEntry> >::iterator iter=waysByCell.begin();
          iter!=waysByCell.end();
          ++iter) {

        iter->second.sort();

        for (std::list<WayEntry>::const_iterator entry=iter->second.begin();
            entry!=iter->second.end();
            ++entry) {
          progress.SetProgress(copyCount,currentEntries);

          Way way;

          if (!scanner.SetPos(entry->fileOffset)) {
            progress.Error(std::string("Error while setting current position in file '")+
                           scanner.GetFilename()+"'");

            return false;
          }

          if (!way.Read(scanner))  {
            progress.Error(std::string("Error while reading data entry at offset ")+
                           NumberToString(entry->fileOffset)+
                           " in file '"+
                           scanner.GetFilename()+"'");

            return false;
          }

          FileOffset fileOffset;

          if (!wayWriter.GetPos(fileOffset)) {
            progress.Error(std::string("Error while reading current fileOffset in file '")+
                           wayWriter.GetFilename()+"'");
            return false;
          }

          if (!way.Write(wayWriter)) {
            progress.Error(std::string("Error while writing data entry to file '")+
                           scanner.GetFilename()+"'");
          }

          mapWriter.Write(entry->id);
          mapWriter.WriteFileOffset(fileOffset);

          copyCount++;
          waysCopyiedCount++;
        }
      }

      if (currentEntries==0) {
        progress.Info("No more entries found");
        break;
      }

      if (maxIndex==cellCount-1) {
        // We are finished
        break;
      }

      minIndex=maxIndex+1;
      maxIndex=cellCount-1;
    }

    assert(waysCount==waysCopyiedCount);

    return scanner.Close() &&
           wayWriter.Close() &&
            mapWriter.Close();
  }

  bool SortWayDataGenerator::CopyWays(const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileScanner scanner;
    FileWriter  wayWriter;
    FileWriter  mapWriter;
    uint32_t    waysCount;

    progress.SetAction("Copy ways");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      "ways.tmp"),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error("Cannot open 'ways.tmp'");
      return false;
    }

    if (!scanner.Read(waysCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "ways.dat"))) {
      progress.Error("Cannot create 'ways.dat'");
      return false;
    }

    wayWriter.Write(waysCount);

    if (!mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     "way.idmap"))) {
      progress.Error("Cannot create 'way.idmap'");
      return false;
    }

    mapWriter.Write(waysCount);

    for (size_t current=1; current<=waysCount; current++) {
      Id  wayId;
      Way way;

      progress.SetProgress(current,waysCount);

      if (!scanner.Read(wayId)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(waysCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!way.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(waysCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      FileOffset fileOffset;

      if (!wayWriter.GetPos(fileOffset)) {
        progress.Error(std::string("Error while reading current fileOffset in file '")+
                       wayWriter.GetFilename()+"'");
        return false;
      }

      if (!way.Write(wayWriter)) {
        progress.Error(std::string("Error while writing data entry to file '")+
                       scanner.GetFilename()+"'");

        return false;
      }

      mapWriter.Write(wayId);
      mapWriter.WriteFileOffset(fileOffset);
    }

    return scanner.Close() &&
           wayWriter.Close() &&
           mapWriter.Close();
  }

  bool SortWayDataGenerator::Import(const ImportParameter& parameter,
                                    Progress& progress,
                                    const TypeConfig& typeConfig)
  {
    if (parameter.GetRenumberIds()) {
      if (!RenumberWays(parameter,
                        progress)) {
        return false;
      }
    }
    else {
      if (!CopyWays(parameter,
                    progress)) {
        return false;
      }
    }

    return true;
  }
}
