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
#include <list>
#include <memory>
#include <unordered_map>

#include <osmscout/import/Import.h>

#include <osmscout/DataFile.h>
#include <osmscout/ObjectRef.h>

#include <osmscout/util/FileWriter.h>

namespace osmscout {

  template <class N>
  class SortDataGenerator : public ImportModule
  {
  private:
    typedef std::unordered_map<Id,Id> IdMap;

    struct Source
    {
      std::string filename;
      FileScanner scanner;
    };

    struct CellEntry
    {
      uint8_t                              type;
      Id                                   id;
      typename std::list<Source>::iterator source;
      FileOffset                           fileOffset;
      Id                                   sortId;

      inline CellEntry(uint8_t type,
                       Id id,
                       const typename std::list<Source>::iterator source,
                       FileOffset fileOffset,
                       Id sortId)
      : type(type),
        id(id),
        source(source),
        fileOffset(fileOffset),
        sortId(sortId)
      {
        // no code
      }

      inline bool operator<(const CellEntry& other) const
      {
        return sortId<other.sortId;
      }
    };

  public:
    class ProcessingFilter
    {
    public:
      virtual ~ProcessingFilter();

      virtual bool BeforeProcessingStart(const ImportParameter& /*parameter*/,
                                         Progress& /*progress*/,
                                         const TypeConfig& /*typeConfig*/)
      {
        return true;
      }

      virtual bool Process(Progress& progress,
                           const FileOffset& offset,
                           N& data,
                           bool& save) = 0;

      virtual bool AfterProcessingEnd(const ImportParameter& /*parameter*/,
                                      Progress& /*progress*/,
                                      const TypeConfig& /*typeConfig*/)
      {
        return true;
      }
    };

    typedef std::shared_ptr<ProcessingFilter> ProcessingFilterRef;

  private:
    std::list<Source>              sources;
    std::string                    dataFilename;
    std::string                    mapFilename;
    std::list<ProcessingFilterRef> filters;

  private:
    bool Renumber(const TypeConfig& typeConfig,
                  const ImportParameter& parameter,
                  Progress& progress);

    bool Copy(const TypeConfig& typeConfig,
              const ImportParameter& parameter,
              Progress& progress);

  protected:
    virtual void GetTopLeftCoordinate(const N& data,
                                      GeoCoord& coord) = 0;

    SortDataGenerator(const std::string& mapFilename,
                      const std::string& tmpFilename);

    void AddSource(const std::string& filename);

    void AddFilter(const ProcessingFilterRef& filter);

  public:
    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress);
  };

  template <class N>
  SortDataGenerator<N>::ProcessingFilter::~ProcessingFilter()
  {
    // no code
  }

  template <class N>
  SortDataGenerator<N>::SortDataGenerator(const std::string& dataFilename,
                                          const std::string& mapFilename)
  : dataFilename(dataFilename),
    mapFilename(mapFilename)
  {
    // no code
  }

  template <class N>
  void SortDataGenerator<N>::AddSource(const std::string& filename)
  {
    Source source;

    source.filename=filename;

    sources.push_back(source);
  }

  template <class N>
  void SortDataGenerator<N>::AddFilter(const ProcessingFilterRef& filter)
  {
    filters.push_back(filter);
  }

  template <class N>
  bool SortDataGenerator<N>::Renumber(const TypeConfig& typeConfig,
                                      const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileWriter dataWriter;
    FileWriter mapWriter;
    uint32_t   overallDataCount=0;
    uint32_t   dataCopiedCount=0;
    size_t     zoomLevel=Pow(2,parameter.GetSortTileMag());
    size_t     cellCount=zoomLevel*zoomLevel;
    size_t     minIndex=0;
    size_t     maxIndex=cellCount-1;

    progress.SetAction("Sorting data");

    for (auto& source : sources) {
      uint32_t dataCount;

      if (!source.scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                               source.filename),
                                FileScanner::Sequential,
                                parameter.GetWayDataMemoryMaped())) {
        progress.Error(std::string("Cannot open '")+source.scanner.GetFilename()+"'");
        return false;
      }

      if (!source.scanner.Read(dataCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      progress.Info(NumberToString(dataCount)+" entries in file '"+source.scanner.GetFilename()+"'");

      overallDataCount+=dataCount;
    }


    if (!dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        dataFilename))) {
      progress.Error(std::string("Cannot create '")+dataWriter.GetFilename()+"'");
      return false;
    }

    dataWriter.Write(overallDataCount);

    if (!mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        mapFilename))) {
      progress.Error(std::string("Cannot create '")+mapWriter.GetFilename()+"'");
      return false;
    }

    mapWriter.Write(overallDataCount);

    while (true) {
      progress.Info("Reading objects in cell range "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));

      size_t                                 currentEntries=0;
      std::map<size_t,std::list<CellEntry> > dataByCellMap;

      for (typename std::list<Source>::iterator source=sources.begin();
           source!=sources.end();
           ++source) {
        uint32_t dataCount;
        progress.Info("Reading objects from file '"+source->scanner.GetFilename()+"'");

        if (!source->scanner.GotoBegin()) {
          progress.Error(std::string("Error while setting current position in file '")+
                         source->scanner.GetFilename()+"'");
        }

        if (!source->scanner.Read(dataCount)) {
          progress.Error("Error while reading number of data entries in file'"+
                         source->scanner.GetFilename()+"'");
          return false;
        }

        uint32_t current=1;

        while (current<=dataCount) {
          uint8_t type;
          Id      id;
          N       data;

          progress.SetProgress(current,dataCount);

          if (!source->scanner.Read(type) ||
              !source->scanner.Read(id) ||
              !data.Read(typeConfig,
                         source->scanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(current)+" of "+
                           NumberToString(dataCount)+
                           " in file '"+
                           source->scanner.GetFilename()+"'");
            return false;
          }

          GeoCoord coord;;

          GetTopLeftCoordinate(data,
                               coord);

          size_t cellY=(size_t)((coord.GetLat()+90.0)/180.0*zoomLevel);
          size_t cellX=(size_t)((coord.GetLon()+180.0)/360.0*zoomLevel);
          size_t cellIndex=cellY*zoomLevel+cellX;

          if (cellIndex>=minIndex &&
              cellIndex<=maxIndex) {
            dataByCellMap[cellIndex].push_back(CellEntry(type,
                                                         id,
                                                         source,
                                                         data.GetFileOffset(),
                                                         coord.ToNumber()));
            currentEntries++;
          }

          // Reduce cell interval, deleting all already stored nodes beyond the new
          // cell range end.
          if (currentEntries>parameter.GetSortBlockSize() &&
              dataByCellMap.size()>1) {
            size_t                                                    count=0;
            size_t                                                    cutLimit=parameter.GetSortBlockSize()*9/10;
            typename std::map<size_t,std::list<CellEntry> >::iterator cutOff=dataByCellMap.end();

            for (typename std::map<size_t,std::list<CellEntry> >::iterator iter=dataByCellMap.begin();
                iter!=dataByCellMap.end();
                ++iter) {
              if (count<=cutLimit &&
                  count+iter->second.size()>cutLimit) {
                cutOff=iter;
                break;
              }
              else {
                maxIndex=iter->first;
                count+=iter->second.size();
              }
            }

            assert(cutOff!=dataByCellMap.end());

            currentEntries=count;
            dataByCellMap.erase(cutOff,dataByCellMap.end());
          }

          current++;
        }
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

        for (auto& entry : iter->second) {
          progress.SetProgress(copyCount,currentEntries);

          copyCount++;

          N data;

          if (!entry.source->scanner.SetPos(entry.fileOffset)) {
            progress.Error(std::string("Error while setting current position in file '")+
                           entry.source->scanner.GetFilename()+"'");

            return false;
          }

          if (!data.Read(typeConfig,
                         entry.source->scanner))  {
            progress.Error(std::string("Error while reading data entry at offset ")+
                           NumberToString(entry.fileOffset)+
                           " in file '"+
                           entry.source->scanner.GetFilename()+"'");

            return false;
          }

          FileOffset fileOffset;
          bool       save=true;

          if (!dataWriter.GetPos(fileOffset)) {
            progress.Error(std::string("Error while reading current fileOffset in file '")+
                           dataWriter.GetFilename()+"'");
            return false;
          }

          for (const auto& filter : filters) {
            if (!filter->Process(progress,
                                 fileOffset,
                                 data,
                                 save)) {
              progress.Error(std::string("Error while processing data entry to file '")+
                             dataWriter.GetFilename()+"'");

              return false;
            }

            if (!save) {
              break;
            }
          }

          if (!save) {
            continue;
          }

          if (!data.Write(typeConfig,
                          dataWriter)) {
            progress.Error(std::string("Error while writing data entry to file '")+
                           dataWriter.GetFilename()+"'");
            return false;
          }

          mapWriter.Write(entry.id);
          mapWriter.Write(entry.type);
          mapWriter.WriteFileOffset(fileOffset);

          dataCopiedCount++;
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

    assert(overallDataCount>=dataCopiedCount);

    for (auto& source : sources) {
      if (!source.scanner.Close()) {
        progress.Error(std::string("Error while  closing '")+source.scanner.GetFilename()+"'");
        return false;
      }
    }

    progress.Info(NumberToString(dataCopiedCount)+" of " +NumberToString(overallDataCount) + " object(s) written to file '"+dataWriter.GetFilename()+"'");

    dataWriter.SetPos(0);
    dataWriter.Write(dataCopiedCount);

    mapWriter.SetPos(0);
    mapWriter.Write(dataCopiedCount);

    return dataWriter.Close() &&
           mapWriter.Close();
  }

  template <class N>
  bool SortDataGenerator<N>::Copy(const TypeConfig& typeConfig,
                                  const ImportParameter& parameter,
                                  Progress& progress)
  {
    FileWriter  dataWriter;
    FileWriter  mapWriter;
    uint32_t    overallDataCount=0;

    progress.SetAction("Copy data");

    if (!dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        dataFilename))) {
      progress.Error(std::string("Cannot create '")+dataWriter.GetFilename()+"'");
      return false;
    }

    dataWriter.Write(overallDataCount);

    if (!mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        mapFilename))) {
      progress.Error(std::string("Cannot create '")+mapWriter.GetFilename()+"'");
      return false;
    }

    mapWriter.Write(overallDataCount);

    for (auto& source : sources) {
      uint32_t dataCount=0;

      progress.Info("Copying from file '"+source.scanner.GetFilename()+"'");

      if (!source.scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                               source.filename),
                               FileScanner::Sequential,
                               parameter.GetWayDataMemoryMaped())) {
        progress.Error(std::string("Cannot open '")+source.scanner.GetFilename()+"'");
        return false;
      }

      if (!source.scanner.Read(dataCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      progress.Info(NumberToString(dataCount)+" entries in file '"+source.scanner.GetFilename()+"'");

      overallDataCount+=dataCount;

      for (uint32_t current=1; current<=dataCount; current++) {
        uint8_t type;
        Id      id;
        N       data;

        progress.SetProgress(current,dataCount);

        if (!source.scanner.Read(type) ||
            !source.scanner.Read(id) ||
            !data.Read(typeConfig,
                       source.scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(dataCount)+
                         " in file '"+
                         source.scanner.GetFilename()+"'");

          return false;
        }

        FileOffset fileOffset;
        bool       save=true;

        if (!dataWriter.GetPos(fileOffset)) {
          progress.Error(std::string("Error while reading current fileOffset in file '")+
                         dataWriter.GetFilename()+"'");
          return false;
        }

        for (auto& filter : filters) {
          if (!filter->Process(progress,
                               fileOffset,
                               data,
                               save)) {
            progress.Error(std::string("Error while processing data entry to file '")+
                           dataWriter.GetFilename()+"'");

            return false;
          }

          if (!save) {
            break;
          }
        }

        if (!save) {
          continue;
        }

        if (!data.Write(typeConfig,
                        dataWriter)) {
          progress.Error(std::string("Error while writing data entry to file '")+
                         dataWriter.GetFilename()+"'");

          return false;
        }

        mapWriter.Write(id);
        mapWriter.Write(type);
        mapWriter.WriteFileOffset(fileOffset);
      }

      if (!source.scanner.Close()) {
        progress.Error(std::string("Error while closing file '")+
                       source.scanner.GetFilename()+"'");
        return false;
      }
    }

    dataWriter.SetPos(0);
    dataWriter.Write(overallDataCount);

    mapWriter.SetPos(0);
    mapWriter.Write(overallDataCount);

    progress.Info(NumberToString(overallDataCount) + " object(s) written to file '"+dataWriter.GetFilename()+"'");

    return dataWriter.Close() &&
           mapWriter.Close();
  }

  template <class N>
  bool SortDataGenerator<N>::Import(const TypeConfigRef& typeConfig,
                                    const ImportParameter& parameter,
                                    Progress& progress)
  {
    bool error=false;

    for (auto& filter : filters) {
      if (!filter->BeforeProcessingStart(parameter,
                                         progress,
                                         *typeConfig)) {
        progress.Error("Cannot initialize processor filter");

        error=true;
      }
    }

    if (!error) {
      if (parameter.GetSortObjects()) {
        if (!Renumber(*typeConfig,
                      parameter,
                      progress)) {
          error=true;
        }
      }
      else {
        if (!Copy(*typeConfig,
                  parameter,
                  progress)) {
          error=true;
        }
      }
    }

    for (auto& filter : filters) {
      if (!filter->AfterProcessingEnd(parameter,
                                      progress,
                                      *typeConfig)) {
        progress.Error("Cannot deinitialize processor filter");

        error=true;
      }
    }

    return !error;
  }
}

#endif
