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

#include <osmscout/import/Import.h>

#include <osmscout/DataFile.h>
#include <osmscout/ObjectRef.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/util/HashMap.h>

namespace osmscout {

  template <class N>
  class SortDataGenerator : public ImportModule
  {
  private:
    typedef OSMSCOUT_HASHMAP<Id,Id> IdMap;

    struct Source
    {
      OSMRefType  type;
      std::string filename;
      FileScanner scanner;
    };

    struct CellEntry
    {
      Id                                   id;
      typename std::list<Source>::iterator source;
      FileOffset                           fileOffset;
      double                               lat;
      double                               lon;

      inline CellEntry(Id id,
                       const typename std::list<Source>::iterator source,
                       FileOffset fileOffset,
                       double lat,
                       double lon)
      : id(id),
        source(source),
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

  public:
    class ProcessingFilter : public Referencable
    {
    protected:
      bool GetAndEraseTag(std::vector<Tag>& tags,
                          TagId tagId,
                          std::string& value) const
      {
        std::vector<Tag>::iterator tag=tags.begin();

        while (tag!=tags.end()) {
          if (tag->key==tagId) {
            value=tag->value;

            tags.erase(tag);

            return true;
          }

          tag++;
        }

        return false;
      }

    public:
      virtual ~ProcessingFilter();

      virtual bool BeforeProcessingStart(const ImportParameter& parameter,
                                         Progress& progress,
                                         const TypeConfig& typeConfig) = 0;
      virtual bool Process(const FileOffset& offset,
                           N& data) = 0;
      virtual bool AfterProcessingEnd() = 0;
    };

    typedef Ref<ProcessingFilter> ProcessingFilterRef;

  private:
    std::list<Source>              sources;
    std::string                    dataFilename;
    std::string                    mapFilename;
    std::list<ProcessingFilterRef> filters;

  private:
    bool Renumber(const ImportParameter& parameter,
                  Progress& progress);

    bool Copy(const ImportParameter& parameter,
              Progress& progress);

  protected:
    virtual void GetTopLeftCoordinate(const N& data,
                                      double& maxLat,
                                      double& minLon) = 0;

    SortDataGenerator(const std::string& mapFilename,
                      const std::string& tmpFilename);

    void AddSource(OSMRefType type,
                   const std::string& filename);

    void AddFilter(const ProcessingFilterRef& filter);

  public:
    bool Import(const ImportParameter& parameter,
                Progress& progress,
                const TypeConfig& typeConfig);
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
  void SortDataGenerator<N>::AddSource(OSMRefType type,
                                       const std::string& filename)
  {
    Source source;

    source.type=type;
    source.filename=filename;

    sources.push_back(source);
  }

  template <class N>
  void SortDataGenerator<N>::AddFilter(const ProcessingFilterRef& filter)
  {
    filters.push_back(filter);
  }

  template <class N>
  bool SortDataGenerator<N>::Renumber(const ImportParameter& parameter,
                                      Progress& progress)
  {
    FileWriter dataWriter;
    FileWriter mapWriter;
    uint32_t   overallDataCount=0;
    uint32_t   dataCopyiedCount=0;
    double     zoomLevel=pow(2.0,(double)parameter.GetSortTileMag());
    size_t     cellCount=zoomLevel*zoomLevel;
    size_t     minIndex=0;
    size_t     maxIndex=cellCount-1;

    progress.SetAction("Sorting data");

    for (typename std::list<Source>::iterator source=sources.begin();
            source!=sources.end();
            ++source) {
      uint32_t dataCount;

      if (!source->scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                source->filename),
                                FileScanner::Sequential,
                                parameter.GetWayDataMemoryMaped())) {
        progress.Error(std::string("Cannot open '")+source->scanner.GetFilename()+"'");
        return false;
      }

      if (!source->scanner.Read(dataCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

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
      progress.Info("Reading ways in cell range "+NumberToString(minIndex)+ "-"+NumberToString(maxIndex));

      size_t                                currentEntries=0;
      std::map<size_t,std::list<CellEntry> > dataByCellMap;

      for (typename std::list<Source>::iterator source=sources.begin();
              source!=sources.end();
              ++source) {
        uint32_t dataCount;
        progress.Info("Reading data from file '"+source->scanner.GetFilename()+"'");

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
          Id  id;
          N   data;

          progress.SetProgress(current,dataCount);

          if (!source->scanner.Read(id)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(current)+" of "+
                           NumberToString(dataCount)+
                           " in file '"+
                           source->scanner.GetFilename()+"'");

            return false;
          }

          if (!data.Read(source->scanner)) {
            progress.Error(std::string("Error while reading data entry ")+
                           NumberToString(current)+" of "+
                           NumberToString(dataCount)+
                           " in file '"+
                           source->scanner.GetFilename()+"'");
            return false;
          }

          double maxLat;
          double minLon;

          GetTopLeftCoordinate(data,maxLat,minLon);

          size_t cellY=(size_t)((maxLat+90.0)/zoomLevel);
          size_t cellX=(size_t)((minLon+180.0)/zoomLevel);
          size_t cellIndex=cellY*zoomLevel+cellX;

          if (cellIndex>=minIndex &&
              cellIndex<=maxIndex) {
            dataByCellMap[cellIndex].push_back(CellEntry(id,
                                                         source,
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

          if (!entry->source->scanner.SetPos(entry->fileOffset)) {
            progress.Error(std::string("Error while setting current position in file '")+
                           entry->source->scanner.GetFilename()+"'");

            return false;
          }

          if (!data.Read(entry->source->scanner))  {
            progress.Error(std::string("Error while reading data entry at offset ")+
                           NumberToString(entry->fileOffset)+
                           " in file '"+
                           entry->source->scanner.GetFilename()+"'");

            return false;
          }

          FileOffset fileOffset;

          if (!dataWriter.GetPos(fileOffset)) {
            progress.Error(std::string("Error while reading current fileOffset in file '")+
                           dataWriter.GetFilename()+"'");
            return false;
          }

          for (typename std::list<ProcessingFilterRef>::iterator f=filters.begin();
              f!=filters.end();
              ++f) {
            ProcessingFilterRef filter(*f);

            if (!filter->Process(fileOffset,
                                 data)) {
              progress.Error(std::string("Error while processing data entry to file '")+
                             dataWriter.GetFilename()+"'");

              return false;
            }
          }

          if (!data.Write(dataWriter)) {
            progress.Error(std::string("Error while writing data entry to file '")+
                           dataWriter.GetFilename()+"'");
            return false;
          }

          mapWriter.Write(entry->id);
          mapWriter.Write((uint8_t)entry->source->type);
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

    assert(overallDataCount==dataCopyiedCount);

    for (typename std::list<Source>::iterator source=sources.begin();
            source!=sources.end();
            ++source) {
      if (!source->scanner.Close()) {
        progress.Error(std::string("Error while  closing '")+source->scanner.GetFilename()+"'");
        return false;
      }
    }

    return dataWriter.Close() &&
           mapWriter.Close();
  }

  template <class N>
  bool SortDataGenerator<N>::Copy(const ImportParameter& parameter,
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

    for (typename std::list<Source>::iterator source=sources.begin();
            source!=sources.end();
            ++source) {
      uint32_t dataCount=0;

      progress.Info("Copying from file '"+source->scanner.GetFilename()+"'");

      if (!source->scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                                source->filename),
                                                FileScanner::Sequential,
                                                parameter.GetWayDataMemoryMaped())) {
        progress.Error(std::string("Cannot open '")+source->scanner.GetFilename()+"'");
        return false;
      }

      if (!source->scanner.Read(dataCount)) {
        progress.Error("Error while reading number of data entries in file");
        return false;
      }

      overallDataCount+=dataCount;

      for (size_t current=1; current<=dataCount; current++) {
        Id id;
        N  data;

        progress.SetProgress(current,dataCount);

        if (!source->scanner.Read(id)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(dataCount)+
                         " in file '"+
                         source->scanner.GetFilename()+"'");

          return false;
        }

        if (!data.Read(source->scanner)) {
          progress.Error(std::string("Error while reading data entry ")+
                         NumberToString(current)+" of "+
                         NumberToString(dataCount)+
                         " in file '"+
                         source->scanner.GetFilename()+"'");

          return false;
        }

        FileOffset fileOffset;

        if (!dataWriter.GetPos(fileOffset)) {
          progress.Error(std::string("Error while reading current fileOffset in file '")+
                         dataWriter.GetFilename()+"'");
          return false;
        }

        for (typename std::list<ProcessingFilterRef>::iterator f=filters.begin();
            f!=filters.end();
            ++f) {
          ProcessingFilterRef filter(*f);

          if (!filter->Process(fileOffset,
                               data)) {
            progress.Error(std::string("Error while processing data entry to file '")+
                           dataWriter.GetFilename()+"'");

            return false;
          }
        }

        if (!data.Write(dataWriter)) {
          progress.Error(std::string("Error while writing data entry to file '")+
                         dataWriter.GetFilename()+"'");

          return false;
        }

        mapWriter.Write(id);
        mapWriter.Write((uint8_t)source->type);
        mapWriter.WriteFileOffset(fileOffset);
      }

      if (!source->scanner.Close()) {
        progress.Error(std::string("Error while closing file '")+
                       source->scanner.GetFilename()+"'");
        return false;
      }
    }

    dataWriter.SetPos(0);
    dataWriter.Write(overallDataCount);

    mapWriter.SetPos(0);
    mapWriter.Write(overallDataCount);

    return dataWriter.Close() &&
           mapWriter.Close();
  }

  template <class N>
  bool SortDataGenerator<N>::Import(const ImportParameter& parameter,
                                    Progress& progress,
                                    const TypeConfig& typeConfig)
  {
    for (typename std::list<ProcessingFilterRef>::iterator f=filters.begin();
        f!=filters.end();
        ++f) {
      ProcessingFilterRef filter(*f);

      if (!filter->BeforeProcessingStart(parameter,
                                         progress,
                                         typeConfig)) {
        progress.Error("Cannot initialize processor filter");

        return false;
      }
    }

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

    for (typename std::list<ProcessingFilterRef>::iterator f=filters.begin();
        f!=filters.end();
        ++f) {
      ProcessingFilterRef filter(*f);

      if (!filter->AfterProcessingEnd()) {
        progress.Error("Cannot deinitialize processor filter");

        return false;
      }
    }

    return true;
  }
}

#endif
