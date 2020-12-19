#ifndef OSMSCOUT_IMPORT_GENCOORDDATA_H
#define OSMSCOUT_IMPORT_GENCOORDDATA_H

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

#include <osmscout/import/Import.h>
#include <osmscout/import/RawNode.h>

#include <osmscout/system/Compiler.h>

namespace osmscout {

  class SerialIdManager CLASS_FINAL
  {
  private:
    std::unordered_map<Id,uint8_t> idToSerialMap;

  public:
    void MarkIdAsDuplicate(Id id);
    uint8_t GetNextSerialForId(Id id);
    size_t Size() const;
  };

  template<typename I, typename E>
  class PageManager
  {
  private:
    std::map<I,std::vector<E>> pages;
    uint32_t                   maximumEntriesInMemory;
    uint32_t                   pageSize;
    uint32_t                   overallEntryCount;         // number of entries in file
    std::function<void(const I&,const std::vector<E>&&)> processor;
    uint32_t                   minPageId;
    uint32_t                   maxPageId;
    uint32_t                   processedPagesCount;
    uint32_t                   processedPagesEntryCount;  // number of entries in completely processed pages
    uint32_t                   handledPagesEntryCount;    // number entries in current pages

  public:
    PageManager(uint32_t maximumEntriesInMemory,
                uint32_t pageSize,
                uint32_t overallEntryCount,
                std::function<void(const I&,const std::vector<E>&&)> processor)
    : maximumEntriesInMemory(maximumEntriesInMemory),
      pageSize(pageSize),
      overallEntryCount(overallEntryCount),
      processor(processor),
      minPageId(std::numeric_limits<I>::min()/pageSize),
      maxPageId(std::numeric_limits<I>::max()/pageSize),
      processedPagesCount(0u),
      processedPagesEntryCount(0u),
      handledPagesEntryCount(0u)
    {
    }

    I GetMinPageId() const {
      return minPageId;
    }

    uint32_t GetProcessedPagesCount() const
    {
      return processedPagesCount;
    }

    uint32_t GetProcessedPagesEntryCount() const
    {
      return processedPagesEntryCount;
    }

    bool IsACurrentlyHandledPage(const I& id) const
    {
      I pageId=id/pageSize;

      return pageId>=minPageId && pageId<=maxPageId;
    }

    void AddEntry(const I& id, const E& entry)
    {
      I pageId=id/pageSize;

      pages[pageId].push_back(entry);
      handledPagesEntryCount++;

      if (handledPagesEntryCount<maximumEntriesInMemory ||
          pages.size()<=1) {
        return;
      }

      [[maybe_unused]] I oldMaxPageId=maxPageId;

      while (handledPagesEntryCount>maximumEntriesInMemory
             && pages.size()>1) {
        auto pageEntry=pages.rbegin();
        I    currentMaxPageId=pageEntry->first;

        handledPagesEntryCount-=(uint32_t)pageEntry->second.size();
        pages.erase(currentMaxPageId);
        maxPageId=currentMaxPageId-1;
      }

      assert(maxPageId<=oldMaxPageId);
    }

    bool AreAllEntriesLoaded() const
    {
      return processedPagesEntryCount+handledPagesEntryCount==overallEntryCount;
    }

    void FileCompletelyScanned()
    {
      processedPagesEntryCount+=handledPagesEntryCount;

      for (auto& pageEntry : pages) {
        processor(pageEntry.first,std::move(pageEntry.second));
        processedPagesCount++;
      }

      handledPagesEntryCount=0;
      pages.clear();
      minPageId=maxPageId+1;
      maxPageId=std::numeric_limits<OSMId>::max()/pageSize;
    }
  };

  class CoordDataGenerator CLASS_FINAL : public ImportModule
  {
  private:
    struct PageEntry
    {
      bool  isSet=false;
      Point point;
    };

  private:
    bool FindDuplicateCoordinates(const TypeConfig& typeConfig,
                                  const ImportParameter& parameter,
                                  Progress& progress,
                                  SerialIdManager& serialIdManager) const;

    bool DumpCurrentPage(FileWriter& writer,
                         std::vector<PageEntry>& page) const;

    bool StoreCoordinates(const TypeConfig& typeConfig,
                          const ImportParameter& parameter,
                          Progress& progress,
                          SerialIdManager& serialIdManager) const;

  public:
    void GetDescription(const ImportParameter& parameter,
                        ImportModuleDescription& description) const override;

    bool Import(const TypeConfigRef& typeConfig,
                const ImportParameter& parameter,
                Progress& progress) override;
  };
}

#endif
