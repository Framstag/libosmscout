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

#include <osmscout/import/GenCoordDat.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_STD_EXECUTION)
  #include <execution>
#endif
#include <limits>
#include <map>

#include <osmscout/CoordDataFile.h>

#include <osmscout/util/ProcessingQueue.h>

#include <osmscout/import/Preprocess.h>
#include <osmscout/import/RawCoord.h>

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

  static void SortCoordPage(std::vector<Id>& page)
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

  /**
   We currently assume that coordinates are ordered by increasing id
   So if we have to nodes with the same coordinate we can expect them
   to have the same serial, as long as above is true and nodes
   for a coordinate are either all part of the import file - or all are left out.
  */
  static void ProcessIds(const std::vector<Id>& ids,
                         SerialIdManager& serialIdManager)
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

  using IdPage = std::vector<Id>;

  class IdPageSortWorker
  {
  private:
    osmscout::ProcessingQueue<IdPage>& inQueue;
    std::thread                        thread;
    osmscout::ProcessingQueue<IdPage>& outQueue;

  private:
    void ProcessorLoop()
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
    }

  public:
    explicit IdPageSortWorker(osmscout::ProcessingQueue<IdPage>& inQueue,
                              osmscout::ProcessingQueue<IdPage>& outQueue)
    : inQueue(inQueue),
      thread(&IdPageSortWorker::ProcessorLoop,this),
      outQueue(outQueue)
    {
    }

    void Wait() {
      thread.join();
    }
  };

  class SerialIdWorker
  {
  private:
    osmscout::ProcessingQueue<IdPage>& queue;
    std::thread                        thread;
    SerialIdManager&                   serialIdManager;

  private:
    void ProcessorLoop()
    {
      while (true) {
        std::optional<IdPage> value=queue.PopTask();

        if (!value) {
          break;
        }

        IdPage page=std::move(value.value());

        ProcessIds(page,serialIdManager);
      }
    }

  public:
    explicit SerialIdWorker(osmscout::ProcessingQueue<IdPage>& queue,
                            SerialIdManager& serialIdManager)
      : queue(queue),
        thread(&SerialIdWorker::ProcessorLoop,this),
        serialIdManager(serialIdManager)
    {
    }

    void Wait() {
      thread.join();
    }
  };

  bool CoordDataGenerator::FindDuplicateCoordinates(const TypeConfig& typeConfig,
                                                    const ImportParameter& parameter,
                                                    Progress& progress,
                                                    SerialIdManager& serialIdManager) const
  {
    progress.SetAction("Searching for duplicate coordinates");

    FileScanner scanner;
    ProcessingQueue<IdPage> idPageQueue1(1000);
    ProcessingQueue<IdPage> idPageQueue2(1000);
    IdPageSortWorker        idSortWorker(idPageQueue1,
                                         idPageQueue2);
    SerialIdWorker          serialIdWorker(idPageQueue2,
                                           serialIdManager);

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
                                     [&idPageQueue1](const Id& /*id*/,
                                                     IdPage&& page) {
                                       idPageQueue1.PushTask(page);
                                     });

      while (!pageManager.AreAllEntriesLoaded()) {
        std::map<Id,std::vector<Id>> coordPages;

        progress.Info("Searching for coordinates with page id >= "+std::to_string(pageManager.GetMinPageId()));

        scanner.GotoBegin();

        coordCount=scanner.ReadUInt32();

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
                      " of "+std::to_string(coordCount)+" coords"+
                      "("+std::to_string(pageManager.GetProcessedPagesCount())+" pages)");
      }

      progress.Info("Found "+std::to_string(serialIdManager.Size())+" duplicate coordinates");

      scanner.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();

      idPageQueue1.Stop();
      idPageQueue2.Stop();

      idSortWorker.Wait();
      serialIdWorker.Wait();

      return false;
    }

    idPageQueue1.Stop();
    idPageQueue2.Stop();

    idSortWorker.Wait();
    serialIdWorker.Wait();

    return true;
  }

  struct PageEntry
  {
    bool  isSet=false;
    Point point;
  };

  static inline bool SortCoordsByOSMId(const RawCoord& a, const RawCoord& b)
  {
    return a.GetOSMId()<b.GetOSMId();
  }

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

  bool DumpCurrentPage(FileWriter& writer,
                       std::vector<PageEntry>& page)
  {
    bool somethingToStore=std::any_of(page.begin(),
                                      page.end(),
                                      [](PageEntry entry) {
                                        return entry.isSet;
                                      });

    if (!somethingToStore) {
      return false;
    }

    for (const PageEntry entry : page) {
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

  void ProcessPage(Progress& progress,
                   const std::vector<RawCoord>& page,
                   SerialIdManager& serialIdManager,
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

  bool CoordDataGenerator::StoreCoordinates(const TypeConfig& typeConfig,
                                            const ImportParameter& parameter,
                                            Progress& progress,
                                            SerialIdManager& serialIdManager) const
  {
    progress.SetAction("Storing coordinates");

    FileScanner              scanner;
    FileWriter               writer;

    try {
      std::unordered_map<OSMId,FileOffset> pageFileOffsetIndex;

      writer.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                  CoordDataFile::COORD_DAT));

      writer.WriteFileOffset(0);
      writer.Write(coordDiskPageSize);
      // We want a coord page to be page aligned on disk, too
      writer.FlushCurrentBlockWithZeros(coordSortPageSize*coordDiskSize);

      scanner.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   Preprocess::RAWCOORDS_DAT),
                   FileScanner::Sequential,
                   true);

      uint32_t coordCount=scanner.ReadUInt32();

      PageSplitter<OSMId,PageEntry> pageSplitter(coordDiskPageSize,
                                                 [&writer,&pageFileOffsetIndex](const OSMId& pageId,
                                                    std::vector<PageEntry>& page) {
                                                   FileOffset pageOffset=writer.GetPos();

                                                   if (DumpCurrentPage(writer,
                                                                       page)) {
                                                     pageFileOffsetIndex[pageId]=pageOffset;
                                                   }
                                                 });

      PageManager<OSMId,RawCoord> pageManager(parameter.GetRawCoordBlockSize(),
                                              coordSortPageSize,
                                              coordCount,
                                              [&progress,&serialIdManager,&pageSplitter](const OSMId& /*pageId*/,
                                                                                         std::vector<RawCoord>&& page) {
                                                SortCoordPage(page);
                                                ProcessPage(progress,page,serialIdManager,pageSplitter);
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
        pageSplitter.FileCompletelyScanned();

        progress.Info("Loaded "+std::to_string(pageManager.GetProcessedPagesEntryCount())+
                      " of "+std::to_string(coordCount)+" coords"+
                      "("+std::to_string(pageManager.GetProcessedPagesCount())+" pages)");

      }

      FileOffset indexStartOffset=writer.GetPos();

      progress.SetAction("Written "+std::to_string(pageFileOffsetIndex.size())+" pages to disk, using "+std::to_string(pageSplitter.GetProcessedPagesCount())+" page dumps");
      progress.SetAction("Writing "+std::to_string(pageFileOffsetIndex.size())+" pages index to disk");

      writer.Write((uint32_t)pageFileOffsetIndex.size());

      for (const auto entry : pageFileOffsetIndex) {
        writer.Write(entry.first);
        writer.Write(entry.second);
      }

      scanner.Close();

      writer.GotoBegin();
      writer.WriteFileOffset(indexStartOffset);
      writer.Close();
    }
    catch (IOException& e) {
      progress.Error(e.GetDescription());
      scanner.CloseFailsafe();
      writer.CloseFailsafe();

      return false;
    }

    return true;
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
