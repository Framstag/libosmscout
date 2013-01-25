#ifndef OSMSCOUT_IMPORT_SORTDAT_H
#define OSMSCOUT_IMPORT_SORTDAT_H

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

#include <cmath>

#include <osmscout/import/Import.h>

#include <osmscout/DataFile.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>

namespace osmscout {

  template <class N>
  class SortDataGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<Id,Id> IdMap;

    struct CellEntry
    {
      Id         id;
      FileOffset fileOffset;
      double     lat;
      double     lon;

      inline CellEntry(Id id,
                       FileOffset fileOffset,
                       double lat,
                       double lon)
      : id(id),
        fileOffset(fileOffset),
        lat(lat),
        lon(lon)
      {
        // no code
      }

      inline bool operator<(const CellEntry& other) const
      {
        if (lon==other.lon) {
          return lat>other.lat;
        }
        else {
          return lon<other.lon;
        }
      }
    };

  private:
    std::string dataFilename;
    std::string mapFilename;
    std::string tmpFilename;

  private:
    bool Renumber(const ImportParameter& parameter,
                  Progress& progress);

    bool Copy(const ImportParameter& parameter,
              Progress& progress);

  protected:
    virtual void GetTopLeftCoordinate(const N& data,
                                      double& maxLat,
                                      double& minLon) = 0;

    SortDataGenerator(const std::string& dataFilename,
                      const std::string& mapFilename,
                      const std::string& tmpFilename);

  public:
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
  };

  template <class N>
  SortDataGenerator<N>::SortDataGenerator(const std::string& dataFilename,
                                          const std::string& mapFilename,
                                          const std::string& tmpFilename)
  : dataFilename(dataFilename),
    mapFilename(mapFilename),
    tmpFilename(tmpFilename)
  {
    // no code
  }

  template <class N>
  bool SortDataGenerator<N>::Renumber(const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileScanner scanner;
    FileWriter  dataWriter;
    FileWriter  mapWriter;
    uint32_t    dataCount;
    uint32_t    dataCopyiedCount=0;
    double      zoomLevel=pow(2.0,(double)parameter.GetSortTileMag());
    size_t      cellCount=zoomLevel*zoomLevel;
    size_t      minIndex=0;
    size_t      maxIndex=cellCount-1;

    progress.SetAction("Sorting data");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      tmpFilename),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        dataFilename))) {
      progress.Error(std::string("Cannot create '")+dataWriter.GetFilename()+"'");
      return false;
    }

    dataWriter.Write(dataCount);

    if (!mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        mapFilename))) {
      progress.Error(std::string("Cannot create '")+mapWriter.GetFilename()+"'");
      return false;
    }

    mapWriter.Write(dataCount);

    while (true) {
      progress.Info("Reading ways in cell range "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex)+" from 'ways.tmp'");

      if (!scanner.GotoBegin()) {
        progress.Error(std::string("Error while setting current position in file '")+
                       scanner.GetFilename()+"'");
      }

      if (!scanner.Read(dataCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      uint32_t                              current=1;
      size_t                                currentEntries=0;
      std::map<size_t,std::list<CellEntry> > dataByCellMap;

      while (current<=dataCount) {
        Id  id;
        N   data;

        progress.SetProgress(current,dataCount);

        if (!scanner.Read(id)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(dataCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");

          return false;
        }

        if (!data.Read(scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(dataCount)+
                         " in file '"+
                         scanner.GetFilename()+"'");
          return false;
        }

        double maxLat;
        double minLon;

        GetTopLeftCoordinate(data,maxLat,minLon);

        size_t cellY=(size_t)(maxLat+90.0/zoomLevel);
        size_t cellX=(size_t)(minLon+180.0/zoomLevel);
        size_t cellIndex=cellY*zoomLevel+cellX;

        if (cellIndex>=minIndex &&
            cellIndex<=maxIndex) {
          dataByCellMap[cellIndex].push_back(CellEntry(id,
                                                       data.GetFileOffset(),
                                                       maxLat,
                                                       minLon));
          currentEntries++;
        }

        // Reduce cell interval,deleting all already stored nodes beyond the new
        // cell range end.
        if (currentEntries>parameter.GetSortBlockSize()) {
          size_t                                                    count=0;
          typename std::map<size_t,std::list<CellEntry> >::iterator cutOff=dataByCellMap.end();

          for (typename std::map<size_t,std::list<CellEntry> >::iterator iter=dataByCellMap.begin();
              iter!=dataByCellMap.end();
              ++iter) {
            if (count<=parameter.GetSortBlockSize() &&
                count+iter->second.size()>parameter.GetSortBlockSize()) {
              cutOff=iter;
              break;
            }
            else {
              count+=iter->second.size();
              maxIndex=iter->first;
            }
          }

          assert(cutOff!=dataByCellMap.end());

          currentEntries=count;
          dataByCellMap.erase(cutOff,dataByCellMap.end());

          progress.Debug("Reducing cell range to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
        }

        current++;
      }

      if (maxIndex<cellCount-1) {
        progress.Info("Cell range was reduced to "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));
      }

      progress.Info(std::string("Copy renumbered data to '")+dataWriter.GetFilename()+"'");

      size_t copyCount=0;
      for (typename std::map<size_t,std::list<CellEntry> >::iterator iter=dataByCellMap.begin();
          iter!=dataByCellMap.end();
          ++iter) {

        iter->second.sort();

        for (typename std::list<CellEntry>::const_iterator entry=iter->second.begin();
            entry!=iter->second.end();
            ++entry) {
          progress.SetProgress(copyCount,currentEntries);

          N data;

          if (!scanner.SetPos(entry->fileOffset)) {
            progress.Error(std::string("Error while setting current position in file '")+
                           scanner.GetFilename()+"'");

            return false;
          }

          if (!data.Read(scanner))  {
            progress.Error(std::string("Error while reading data entry at offset ")+
                           NumberToString(entry->fileOffset)+
                           " in file '"+
                           scanner.GetFilename()+"'");

            return false;
          }

          FileOffset fileOffset;

          if (!dataWriter.GetPos(fileOffset)) {
            progress.Error(std::string("Error while reading current fileOffset in file '")+
                           dataWriter.GetFilename()+"'");
            return false;
          }

          if (!data.Write(dataWriter)) {
            progress.Error(std::string("Error while writing data entry to file '")+
                           scanner.GetFilename()+"'");
          }

          mapWriter.Write(entry->id);
          mapWriter.WriteFileOffset(fileOffset);

          copyCount++;
          dataCopyiedCount++;
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

    assert(dataCount==dataCopyiedCount);

    return scanner.Close() &&
           dataWriter.Close() &&
           mapWriter.Close();
  }

  template <class N>
  bool SortDataGenerator<N>::Copy(const ImportParameter& parameter,
                                  Progress& progress)
  {
    FileScanner scanner;
    FileWriter  dataWriter;
    FileWriter  mapWriter;
    uint32_t    dataCount;

    progress.SetAction("Copy data");

    if (!scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      tmpFilename),
                      FileScanner::Sequential,
                      parameter.GetWayDataMemoryMaped())) {
      progress.Error(std::string("Cannot open '")+scanner.GetFilename()+"'");
      return false;
    }

    if (!scanner.Read(dataCount)) {
      progress.Error("Error while reading number of data entries in file");
      return false;
    }

    if (!dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        dataFilename))) {
      progress.Error(std::string("Cannot create '")+dataWriter.GetFilename()+"'");
      return false;
    }

    dataWriter.Write(dataCount);

    if (!mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        mapFilename))) {
      progress.Error(std::string("Cannot create '")+mapWriter.GetFilename()+"'");
      return false;
    }

    mapWriter.Write(dataCount);

    for (size_t current=1; current<=dataCount; current++) {
      Id id;
      N  data;

      progress.SetProgress(current,dataCount);

      if (!scanner.Read(id)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      if (!data.Read(scanner)) {
        progress.Error(std::string("Error while reading data entry ")+
                       NumberToString(current)+" of "+
                       NumberToString(dataCount)+
                       " in file '"+
                       scanner.GetFilename()+"'");

        return false;
      }

      FileOffset fileOffset;

      if (!dataWriter.GetPos(fileOffset)) {
        progress.Error(std::string("Error while reading current fileOffset in file '")+
                       dataWriter.GetFilename()+"'");
        return false;
      }

      if (!data.Write(dataWriter)) {
        progress.Error(std::string("Error while writing data entry to file '")+
                       dataWriter.GetFilename()+"'");

        return false;
      }

      mapWriter.Write(id);
      mapWriter.WriteFileOffset(fileOffset);
    }

    return scanner.Close() &&
           dataWriter.Close() &&
           mapWriter.Close();
  }

  template <class N>
  bool SortDataGenerator<N>::Import(const ImportParameter& parameter,
                                    Progress& progress,
                                    const TypeConfig& typeConfig)
  {
    if (parameter.GetSortObjects()) {
      if (!Renumber(parameter,
                    progress)) {
        return false;
      }
    }
    else {
      if (!Copy(parameter,
                progress)) {
        return false;
      }
    }

    return true;
  }
}

#endif
