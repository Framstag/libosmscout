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

#include <list>
#include <memory>
#include <unordered_map>

#include <osmscout/import/Import.h>

#include <osmscout/DataFile.h>
#include <osmscout/ObjectRef.h>

#include <osmscout/util/FileWriter.h>
#include <osmscout/system/Math.h>

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

    SortDataGenerator(const std::string& dataFilename,
                      const std::string& mapFilename);

    void AddSource(const std::string& filename);

    void AddFilter(const ProcessingFilterRef& filter);

  public:
    std::list<std::string> ProvidesFiles(const ImportParameter& parameter) const;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
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
  std::list<std::string> SortDataGenerator<N>::ProvidesFiles(const ImportParameter& /*parameter*/) const
  {
    std::list<std::string> providedFiles={dataFilename, mapFilename};

    return providedFiles;
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
    size_t     zoomLevel=Pow(2,parameter.GetSortTileMag());
    size_t     cellCount=zoomLevel*zoomLevel;
    size_t     maxIndex=cellCount-1;

    progress.SetAction("Sorting data");

    try {
      uint32_t overallDataCount=0;
      uint32_t dataCopiedCount=0;
      size_t   minIndex=0;

      for (auto& source : sources) {
        uint32_t dataCount=0;

        source.scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                            source.filename),
                            FileScanner::Sequential,
                            parameter.GetWayDataMemoryMaped());

        source.scanner.Read(dataCount);

        progress.Info(std::to_string(dataCount)+" entries in file '"+source.scanner.GetFilename()+"'");

        overallDataCount+=dataCount;
      }


      dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      dataFilename));

      dataWriter.Write(overallDataCount);

      mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     mapFilename));

      mapWriter.Write(overallDataCount);

      while (true) {
        progress.Info("Reading objects in cell range "+std::to_string(minIndex)+ "-"+std::to_string(maxIndex));

        size_t                                 currentEntries=0;
        std::map<size_t,std::list<CellEntry> > dataByCellMap;

        for (typename std::list<Source>::iterator source=sources.begin();
             source!=sources.end();
             ++source) {
          uint32_t dataCount;
          progress.Info("Reading objects from file '"+source->scanner.GetFilename()+"'");

          source->scanner.GotoBegin();

          source->scanner.Read(dataCount);

          uint32_t current=1;

          while (current<=dataCount) {
            uint8_t type;
            Id      id;
            N       data;

            progress.SetProgress(current,dataCount);

            source->scanner.Read(type);
            source->scanner.Read(id);

            data.Read(typeConfig,
                      source->scanner);

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
                                                           coord.GetHash()));
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
          progress.Info("Cell range was reduced to "+std::to_string(minIndex)+ "-"+std::to_string(maxIndex));
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

            entry.source->scanner.SetPos(entry.fileOffset);

            data.Read(typeConfig,
                      entry.source->scanner);

            FileOffset fileOffset;
            bool       save=true;

            fileOffset=dataWriter.GetPos();

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

            data.Write(typeConfig,
                       dataWriter);

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
        source.scanner.Close();
      }

      progress.Info(std::to_string(dataCopiedCount)+" of " +std::to_string(overallDataCount) + " object(s) written to file '"+dataWriter.GetFilename()+"'");

      dataWriter.SetPos(0);
      dataWriter.Write(dataCopiedCount);

      mapWriter.SetPos(0);
      mapWriter.Write(dataCopiedCount);

      dataWriter.Close();
      mapWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      for (auto& source : sources) {
        source.scanner.CloseFailsafe();
      }

      dataWriter.CloseFailsafe();
      mapWriter.CloseFailsafe();

      return false;
    }

    return true;
  }

  template <class N>
  bool SortDataGenerator<N>::Copy(const TypeConfig& typeConfig,
                                  const ImportParameter& parameter,
                                  Progress& progress)
  {
    FileWriter  dataWriter;
    FileWriter  mapWriter;

    progress.SetAction("Copy data");

    try {
      uint32_t overallDataCount=0;

      dataWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                      dataFilename));

      dataWriter.Write(overallDataCount);

      mapWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     mapFilename));

      mapWriter.Write(overallDataCount);

      for (auto& source : sources) {
        uint32_t dataCount=0;

        progress.Info("Copying from file '"+source.scanner.GetFilename()+"'");

        source.scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                            source.filename),
                            FileScanner::Sequential,
                            parameter.GetWayDataMemoryMaped());

        source.scanner.Read(dataCount);

        progress.Info(std::to_string(dataCount)+" entries in file '"+source.scanner.GetFilename()+"'");

        overallDataCount+=dataCount;

        for (uint32_t current=1; current<=dataCount; current++) {
          uint8_t type=0;
          Id      id=0;
          N       data;

          progress.SetProgress(current,dataCount);

          source.scanner.Read(type);
          source.scanner.Read(id);

          data.Read(typeConfig,
                    source.scanner);

          FileOffset fileOffset;
          bool       save=true;

          fileOffset=dataWriter.GetPos();

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

          data.Write(typeConfig,
                     dataWriter);

          mapWriter.Write(id);
          mapWriter.Write(type);
          mapWriter.WriteFileOffset(fileOffset);
        }

        source.scanner.Close();
      }

      dataWriter.SetPos(0);
      dataWriter.Write(overallDataCount);

      mapWriter.SetPos(0);
      mapWriter.Write(overallDataCount);

      progress.Info(std::to_string(overallDataCount) + " object(s) written to file '"+dataWriter.GetFilename()+"'");

      dataWriter.Close();
      mapWriter.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());

      for (auto& source : sources) {
        source.scanner.CloseFailsafe();
      }

      dataWriter.CloseFailsafe();
      mapWriter.CloseFailsafe();

      return false;
    }

    return true;
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
