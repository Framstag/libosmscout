/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscoutimport/GenCoordDat.h>

#include <osmscoutimport/private/Config.h>

#if defined(HAVE_STD_EXECUTION)
  #include <execution>
#endif
#include <limits>
#include <map>

#include <osmscout/db/CoordDataFile.h>

#include <osmscout/async/ProcessingQueue.h>
#include <osmscout/async/Worker.h>

#include <osmscoutimport/Preprocess.h>
#include <osmscoutimport/RawCoord.h>

namespace osmscout {

  static const uint32_t coordSortPageSize=100000000;
  static const uint32_t coordDiskPageSize=64;
  static const uint32_t coordDiskSize=8;

  void SerialIdManager::MarkIdAsDuplicate(Id id)
  {
    idToSerialMap[id]=1;
  }

  uint8_t SerialIdManager::GetNextSerialForId(Id id)
  {
    auto entry=idToSerialMap.find(id);

    if (entry!=idToSerialMap.end()) {
      auto currentValue=entry->second;

      entry->second++;

      return currentValue;
    }

    return 1;
  }

  size_t SerialIdManager::Size() const
  {
    return idToSerialMap.size();
  }

  using IdPage = std::vector<Id>;

  class RawCoordIdReaderWorker CLASS_FINAL : public Producer<IdPage>
  {
  private:
    const TypeConfig                  &typeConfig;
    const ImportParameter             &parameter;
    Progress                          &progress;

  private:
    void ProcessingLoop() override
    {
      FileScanner scanner;

      try {
        scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     Preprocess::RAWCOORDS_DAT),
                     FileScanner::Sequential,
                     true);

        scanner.GotoBegin();

        uint32_t coordCount=scanner.ReadUInt32();

        PageManager<Id,Id> pageManager(parameter.GetRawCoordBlockSize(),
                                       coordSortPageSize,
                                       coordCount,
                                       [this](const Id& /*id*/,
                                                 IdPage&& page) {
                                         outQueue.PushTask(page);
                                       });

        while (!pageManager.AreAllEntriesLoaded()) {
          progress.Info("Searching for coordinates with page id >= "+std::to_string(pageManager.GetMinPageId()));

          scanner.GotoBegin();

          /* ignore */ scanner.ReadUInt32();

          RawCoord coord;

          for (uint32_t i=1; i<=coordCount; i++) {
            progress.SetProgress(i,coordCount);

            coord.Read(typeConfig,scanner);

            Id id=coord.GetCoord().GetId();

            if (!pageManager.IsACurrentlyHandledPage(id)) {
              continue;
            }

            // Since different coords may have the same id, it is possible that a page contains
            // multiple instances of the same id (and a page may container more then
            // 'coordSortPageSize' entries
            pageManager.AddEntry(id,id);

            // Shortcut loading of file in the last iteration
            if (pageManager.AreAllEntriesLoaded()) {
              break;
            }
          }

          pageManager.FileCompletelyScanned();

          progress.Info("Loaded "+std::to_string(pageManager.GetProcessedPagesEntryCount())+
                        " of "+std::to_string(coordCount)+" coords "+
                        "("+std::to_string(pageManager.GetProcessedPagesCount())+" pages)");
        }

        scanner.Close();

        progress.Info("File '" + scanner.GetFilename() + "' completely scanned");
      }
      catch (IOException& e) {
        progress.Error(e.GetDescription());
        scanner.CloseFailsafe();

        MarkWorkerAsFailed();
      }

      outQueue.Stop();
    }

  public:
    RawCoordIdReaderWorker(const TypeConfig& typeConfig,
                           const ImportParameter& parameter,
                           Progress& progress,
                           osmscout::ProcessingQueue<IdPage>& outQueue)
      : Producer(outQueue),
        typeConfig(typeConfig),
        parameter(parameter),
        progress(progress)
    {
      Start();
    }
  };

  class IdPageSortWorker CLASS_FINAL : public Pipe<IdPage,IdPage>
  {
  private:
    void SortCoordPage(std::vector<Id>& page)
    {
#if defined(HAVE_STD_EXECUTION)
      std::sort(std::execution::par_unseq,
                    page.begin(),
                    page.end());

#else
      std::sort(page.begin(),
                page.end());
#endif
    }

    void ProcessingLoop() override
    {
      while (true) {
        std::optional<IdPage> value=inQueue.PopTask();

        if (!value) {
          break;
        }

        IdPage page=std::move(value.value());

        SortCoordPage(page);

        outQueue.PushTask(page);
      }

      outQueue.Stop();
    }

  public:
    IdPageSortWorker(osmscout::ProcessingQueue<IdPage>& inQueue,
                     osmscout::ProcessingQueue<IdPage>& outQueue)
    : Pipe(inQueue,outQueue)
    {
      Start();
    }
  };

  class SerialIdWorker CLASS_FINAL : public Consumer<IdPage>
  {
  private:
    Progress                          &progress;
    SerialIdManager                   &serialIdManager;

  private:
    /**
      We currently assume that coordinates are ordered by increasing id
      So if we have to nodes with the same coordinate we can expect them
      to have the same serial, as long as above is true and nodes
      for a coordinate are either all part of the import file - or all are left out.
    */
    void ProcessIds(const std::vector<Id>& ids)
    {
      Id   lastId                      =std::numeric_limits<Id>::max();
      bool currentIdDetectedAsDuplicate=false;

      for (auto id : ids) {
        if (id==lastId) {
          if (!currentIdDetectedAsDuplicate) {
            serialIdManager.MarkIdAsDuplicate(id);
            currentIdDetectedAsDuplicate=true;
          }
        }
        else {
          currentIdDetectedAsDuplicate=false;
        }

        lastId=id;
      }
    }

    void ProcessingLoop() override
    {
      progress.Info("Detecting duplicate coordinates");

      while (true) {
        std::optional<IdPage> value=inQueue.PopTask();

        if (!value) {
          break;
        }

        IdPage page=std::move(value.value());

        ProcessIds(page);
      }

      progress.Info("Found "+std::to_string(serialIdManager.Size())+" duplicate coordinates");
    }

  public:
    SerialIdWorker(Progress& progress,
                   osmscout::ProcessingQueue<IdPage>& queue,
                   SerialIdManager& serialIdManager)
      : Consumer(queue),
        progress(progress),
        serialIdManager(serialIdManager)
    {
      Start();
    }
  };

  /**
   * Reads the 'rawcoord.dat' file and passes data to initialise a SerialIdManager instance which is returned.
   *
   * @param typeConfig
   * @param parameter
   * @param progress
   * @param serialIdManager
   * @return
   *
   * The internal processing is a follows:
   *
   *   Read `rawcoord.dat` => PageManager => queue(IdPage) => IdPageSortWorker => queue(IdPage) => SerialIdWorker => result
   */
  bool CoordDataGenerator::FindDuplicateCoordinates(const TypeConfig& typeConfig,
                                                    const ImportParameter& parameter,
                                                    Progress& progress,
                                                    SerialIdManager& serialIdManager) const
  {
    progress.SetAction("Searching for duplicate coordinates");

    ProcessingQueue<IdPage> queue1(10000);
    ProcessingQueue<IdPage> queue2(10000);

    RawCoordIdReaderWorker  rawCoordIdReaderWorker(typeConfig,
                                                   parameter,
                                                   progress,
                                                   queue1);
    IdPageSortWorker        idSortWorker(queue1,
                                         queue2);
    SerialIdWorker          serialIdWorker(progress,
                                           queue2,
                                           serialIdManager);

    rawCoordIdReaderWorker.Wait();
    idSortWorker.Wait();
    serialIdWorker.Wait();

    return rawCoordIdReaderWorker.WasSuccessful() && idSortWorker.WasSuccessful() && serialIdWorker.WasSuccessful();
  }

  using RawCoordPage = std::vector<RawCoord>;

  class RawCoordReaderWorker CLASS_FINAL : public Producer<RawCoordPage>
  {
  private:
    const TypeConfig                        &typeConfig;
    const ImportParameter                   &parameter;
    Progress                                &progress;

  private:
    void ProcessingLoop() override
    {
      FileScanner scanner;

      try {
        scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                     Preprocess::RAWCOORDS_DAT),
                     FileScanner::Sequential,
                     true);

        uint32_t coordCount=scanner.ReadUInt32();

        PageManager<OSMId,RawCoord> pageManager(parameter.GetRawCoordBlockSize(),
                                                coordSortPageSize,
                                                coordCount,
                                                [this](const OSMId& /*pageId*/,
                                                          std::vector<RawCoord>&& page) {
                                                  outQueue.PushTask(page);
                                                });

        while (!pageManager.AreAllEntriesLoaded()) {
          progress.Info("Search for coordinates with page id >= "+std::to_string(pageManager.GetMinPageId()));

          scanner.GotoBegin();

          /* ignore */ scanner.ReadUInt32();

          RawCoord coord;

          for (uint32_t i=1; i<=coordCount; i++) {
            progress.SetProgress(i,coordCount);

            coord.Read(typeConfig,scanner);

            if (!pageManager.IsACurrentlyHandledPage(coord.GetOSMId())) {
              continue;
            }

            pageManager.AddEntry(coord.GetOSMId(),coord);

            // Shortcut loading of file in the last iteration
            if (pageManager.AreAllEntriesLoaded()) {
              break;
            }
          }

          pageManager.FileCompletelyScanned();

          progress.Info("Loaded "+std::to_string(pageManager.GetProcessedPagesEntryCount())+
                        " of "+std::to_string(coordCount)+" coords "+
                        "("+std::to_string(pageManager.GetProcessedPagesCount())+" pages)");

        }

        scanner.Close();

        progress.Info("File '" + scanner.GetFilename() + "' completely scanned");
      }
      catch (IOException& e) {
        progress.Error(e.GetDescription());
        scanner.CloseFailsafe();

        MarkWorkerAsFailed();
      }

      outQueue.Stop();
    }

  public:
    RawCoordReaderWorker(const TypeConfig& typeConfig,
                         const ImportParameter& parameter,
                         Progress& progress,
                         osmscout::ProcessingQueue<RawCoordPage>& outQueue)
      : Producer(outQueue),
        typeConfig(typeConfig),
        parameter(parameter),
        progress(progress)
    {
      Start();
    }
  };

  static inline bool SortCoordsByOSMId(const RawCoord& a, const RawCoord& b)
  {
    return a.GetOSMId()<b.GetOSMId();
  }

  class RawCoordPageSortWorker CLASS_FINAL : public Pipe<RawCoordPage,RawCoordPage>
  {
  private:
    static void SortCoordPage(std::vector<RawCoord>& page)
    {
#if defined(HAVE_STD_EXECUTION)
      std::sort(std::execution::par_unseq,
                    page.begin(),
                    page.end(),
                    SortCoordsByOSMId);

#else
      std::sort(page.begin(),
                page.end(),
                SortCoordsByOSMId);
#endif
    }

    void ProcessingLoop() override
    {
      while (true) {
        std::optional<RawCoordPage> value=inQueue.PopTask();

        if (!value) {
          break;
        }

        RawCoordPage page=std::move(value.value());

        SortCoordPage(page);

        outQueue.PushTask(page);
      }

      outQueue.Stop();
    }

  public:
    RawCoordPageSortWorker(osmscout::ProcessingQueue<RawCoordPage>& inQueue,
                           osmscout::ProcessingQueue<RawCoordPage>& outQueue)
      : Pipe(inQueue,outQueue)
    {
      Start();
    }
  };

  struct PageEntry
  {
    bool  isSet=false;
    Point point;
  };

  class CoordDatFileWorker CLASS_FINAL : public Consumer<RawCoordPage>
  {
  private:
    const ImportParameter                   &parameter;
    Progress                                &progress;

    SerialIdManager                         &serialIdManager;

  private:
    bool WritePage(FileWriter& writer,
                   std::vector<PageEntry>& page)
    {
      bool somethingToStore=std::any_of(page.begin(),
                                        page.end(),
                                        [](const PageEntry& entry) {
                                          return entry.isSet;
                                        });

      if (!somethingToStore) {
        return false;
      }

      for (const PageEntry& entry : page) {
        if (entry.isSet) {
          writer.Write(entry.point.GetSerial());
          writer.WriteCoord(entry.point.GetCoord());
        }
        else {
          writer.Write((uint8_t)0);
          writer.WriteInvalidCoord();
        }
      }

      return true;
    }

    void ProcessPage(const std::vector<RawCoord>& page,
                     PageSplitter<OSMId,PageEntry>& pageSplitter)
    {
      for (const auto& osmCoord : page) {
        uint8_t serial=serialIdManager.GetNextSerialForId(osmCoord.GetCoord().GetId());

        if (serial==255) {
          progress.Error("Coordinate "+std::to_string(osmCoord.GetOSMId())+" "+osmCoord.GetCoord().GetDisplayText()+" has more than 256 nodes");
          continue;
        }

        pageSplitter.Set(osmCoord.GetOSMId(),
                         PageEntry{true,
                                   Point(serial,
                                         osmCoord.GetCoord())});
      }
    }

    void ProcessingLoop() override
    {
      FileWriter writer;

      try {
        std::unordered_map<OSMId,FileOffset> pageFileOffsetIndex;

        PageSplitter<OSMId,PageEntry> pageSplitter(coordDiskPageSize,
                                                   [this,&writer,&pageFileOffsetIndex](const OSMId& pageId,
                                                                                  std::vector<PageEntry>& page) {
                                                     FileOffset pageOffset=writer.GetPos();

                                                     if (WritePage(writer,
                                                                   page)) {
                                                       pageFileOffsetIndex[pageId]=pageOffset;
                                                     }
                                                   });

        writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    CoordDataFile::COORD_DAT));

        progress.Info("Writing file '" + writer.GetFilename() + "'");

        writer.WriteFileOffset(0);
        writer.Write(coordDiskPageSize);
        // We want a coord page to be page aligned on disk, too
        writer.FlushCurrentBlockWithZeros(coordSortPageSize*coordDiskSize);

        while (true) {
          std::optional<RawCoordPage> value=inQueue.PopTask();

          if (!value) {
            break;
          }

          RawCoordPage page=std::move(value.value());

          ProcessPage(page,pageSplitter);
        }

        // Make sure that every page is dumped

        pageSplitter.FileCompletelyScanned();

        progress.SetAction("Written {} pages to disk, using {} page dumps",
          pageFileOffsetIndex.size(),
          pageSplitter.GetProcessedPagesCount());

        progress.SetAction("Writing {} pages index to disk",pageFileOffsetIndex.size());

        // Remember index start
        FileOffset indexStartOffset=writer.GetPos();

        // Write Index

        writer.Write((uint32_t)pageFileOffsetIndex.size());
        for (const auto& entry : pageFileOffsetIndex) {
          writer.Write(entry.first);
          writer.Write(entry.second);
        }

        // Write Index offset

        writer.GotoBegin();
        writer.WriteFileOffset(indexStartOffset);

        // Close file

        writer.Close();

        progress.Info("File '" + writer.GetFilename() + "' completely written");
      }
      catch (IOException& e) {
        progress.Error(e.GetDescription());
        writer.CloseFailsafe();

        MarkWorkerAsFailed();
      }
    }

  public:
    explicit CoordDatFileWorker(const ImportParameter& parameter,
                                Progress& progress,
                                osmscout::ProcessingQueue<RawCoordPage>& inQueue,
                                SerialIdManager& serialIdManager)
      : Consumer(inQueue),
        parameter(parameter),
        progress(progress),
        serialIdManager(serialIdManager)
    {
      Start();
    }
  };

  /**
   * Reads the `rawcoord.dat` file and generates a `coord.dat` file using the information in the passed SerialIdManager
   * instance.
   *
   * @param typeConfig
   * @param parameter
   * @param progress
   * @param serialIdManager
   * @return
   *
   * Internal Processing:
   *   Read `rawcoord.dat` => PageManager => queue(RawCoordPage) => RawCoordPageSortWorker => queue(RawCoordPage) => CoordDatWorker
   */
  bool CoordDataGenerator::StoreCoordinates(const TypeConfig& typeConfig,
                                            const ImportParameter& parameter,
                                            Progress& progress,
                                            SerialIdManager& serialIdManager) const
  {
    progress.SetAction("Storing coordinates");

    ProcessingQueue<RawCoordPage> queue1(10000);
    ProcessingQueue<RawCoordPage> queue2(10000);

    RawCoordReaderWorker   rawCoordReaderWorker(typeConfig,
                                                parameter,
                                                progress,
                                                queue1);
    RawCoordPageSortWorker rawCoordPageSortWorker(queue1,
                                                  queue2);
    CoordDatFileWorker     coordDatFileWorker(parameter,
                                              progress,
                                              queue2,
                                              serialIdManager);

    rawCoordReaderWorker.Wait();
    rawCoordPageSortWorker.Wait();
    coordDatFileWorker.Wait();

    return rawCoordReaderWorker.WasSuccessful() &&
           rawCoordPageSortWorker.WasSuccessful() &&
           coordDatFileWorker.WasSuccessful();
  }

  void CoordDataGenerator::GetDescription(const ImportParameter& /*parameter*/,
                                          ImportModuleDescription& description) const
  {
    description.SetName("CoordDataGenerator");
    description.SetDescription("Generate coord data file");

    description.AddRequiredFile(Preprocess::RAWCOORDS_DAT);

    description.AddProvidedDebuggingFile(CoordDataFile::COORD_DAT);
  }

  bool CoordDataGenerator::Import(const TypeConfigRef& typeConfig,
                                  const ImportParameter& parameter,
                                  Progress& progress)
  {
    SerialIdManager serialIdManager;

    if (!FindDuplicateCoordinates(*typeConfig,
                                  parameter,
                                  progress,
                                  serialIdManager)) {
      return false;
    }

    if (!StoreCoordinates(*typeConfig,
                          parameter,
                          progress,
                          serialIdManager)) {
      return false;
    }

    return true;
  }
}
