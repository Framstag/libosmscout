#ifndef OSMSCOUT_NUMERICINDEX_H
#define OSMSCOUT_NUMERICINDEX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <mutex>
#include <vector>

#include <osmscout/util/Cache.h>
#include <osmscout/util/File.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

namespace osmscout {

  /**
    \ingroup Database
    Numeric index handles an index over instance of class <T> where the index criteria
    is of type <N>, where <N> has a numeric nature (usually Id).
    */
  template <class N>
  class NumericIndex
  {
  private:
    /**
      an individual index entry.
      */
    struct Entry
    {
      N          startId;
      FileOffset fileOffset;
    };

    struct Page
    {
      std::vector<Entry> entries;

      inline bool IndexIsValid(size_t index) const
      {
        return index<entries.size();
      }
    };

    using PageRef         = std::shared_ptr<Page>;
    using PageCache       = Cache<N, PageRef>;
    using PageSimpleCache = std::unordered_map<N, PageRef>;

    /**
      Returns the size of a individual cache entry
      */
    struct NumericIndexCacheValueSizer : public PageCache::ValueSizer
    {
      size_t GetSize(const PageRef& value) const
      {
        return sizeof(value)+sizeof(Page)+sizeof(Entry)*value->entries.size();
      }
    };

  private:
    std::string                         filepart;             //!< Name of the index file
    std::string                         filename;             //!< Complete file name including directory

    mutable FileScanner                  scanner;             //!< FileScanner instance for file access

    size_t                               cacheSize;           //!< Maximum umber of index pages cached
    uint32_t                             pageSize;            //!< Size of one page as stated by the actual index file
    uint32_t                             levels;              //!< Number of index levels as stated by the actual index file
    std::vector<uint32_t>                pageCounts;          //!< Number of pages per level as stated by the actual index file
    char                                 *buffer;             //!< Temporary buffer for reading page data

    PageRef                              root;                //!< Reference to the root page
    size_t                               simpleCacheMaxLevel; //!< Maximum level for simple caching
    mutable std::vector<PageSimpleCache> simplePageCache;     //!< Simple map to cache all entries
    mutable std::vector<PageCache>       pageCaches;          //!< Complex cache with LRU characteristics

    mutable std::mutex                   accessMutex;         //!< Mutex to secure multi-thread access

  private:
    size_t GetPageIndex(const Page& page, N id) const;
    void ReadPage(FileOffset offset, PageRef& page) const;
    void InitializeCache();

  public:
    NumericIndex(const std::string& filename,
                 size_t cacheSize);
    virtual ~NumericIndex();

    bool Open(const std::string& path,
              bool memoryMapped);
    bool Close();

    bool IsOpen() const;

    bool GetOffset(const N& id, FileOffset& offset) const;

    template<typename IteratorIn>
    bool GetOffsets(IteratorIn begin,
                    IteratorIn end,
                    size_t size,
                    std::vector<FileOffset>& offsets) const;

    void DumpStatistics() const;
  };

  template <class N>
  NumericIndex<N>::NumericIndex(const std::string& filename,
                                size_t cacheSize)
   : filepart(filename),
     cacheSize(cacheSize),
     pageSize(0),
     levels(0),
     buffer(nullptr)
  {
    // no code
  }

  template <class N>
  NumericIndex<N>::~NumericIndex()
  {
    Close();

    delete [] buffer;
  }

  /**
    Binary search for index page for given id
    */
  template <class N>
  inline size_t NumericIndex<N>::GetPageIndex(const Page& page, N id) const
  {
    size_t size=page.entries.size();

    //std::cout << "Lookup in page: " << page.entries[0].startId << " " << id << " " << page.entries[page.entries.size()-1].startId << std::endl;

    if (size>0) {
      size_t left=0;
      size_t right=size-1;

      while (left<=right) {
        size_t mid=(left+right)/2;

        if (page.entries[mid].startId<=id &&
            (mid+1>=size || page.entries[mid+1].startId>id)) {
          return mid;
        }

        if (page.entries[mid].startId<id) {
          left=mid+1;
        }
        else {
          if (mid==0) {
            return size;
          }

          right=mid-1;
        }
      }
    }

    return size;
  }

  template <class N>
  inline void NumericIndex<N>::ReadPage(FileOffset offset, PageRef& page) const
  {
    if (!page) {
      page=std::make_shared<Page>();
    }
    else {
      page->entries.clear();
    }

    page->entries.reserve(pageSize/4);

    scanner.SetPos(offset);

    scanner.Read(buffer,
                 pageSize);

    size_t     currentPos=0;
    N          prevId=0;
    FileOffset prefFileOffset=0;

    //std::cout << "Page: " << offset << std::endl;

    while (currentPos<pageSize &&
           buffer[currentPos]!=0) {
      unsigned int idBytes;
      unsigned int fileOffsetBytes;
      N            curId;
      FileOffset   curFileOffset;
      Entry        entry;

      idBytes=DecodeNumber(&buffer[currentPos],
                           curId);

      currentPos+=idBytes;

      fileOffsetBytes=DecodeNumber(&buffer[currentPos],
                                   curFileOffset);

      currentPos+=fileOffsetBytes;

      entry.startId=prevId+curId;
      entry.fileOffset=prefFileOffset+curFileOffset;

      //std::cout << "- " << entry.startId << " " << idBytes << " " << entry.fileOffset << " " << fileOffsetBytes << std::endl;

      prevId=entry.startId;
      prefFileOffset=entry.fileOffset;

      page->entries.push_back(entry);
    }
  }

  template <class N>
  void NumericIndex<N>::InitializeCache()
  {
    size_t currentCacheSize=cacheSize; // Available free space in cache
    size_t requiredCacheSize=0;        // Space needed for caching everything

    for (const auto count : pageCounts) {
      requiredCacheSize+=count;
    }

    if (requiredCacheSize>cacheSize) {
      log.Warn() << "Warning: Index " << filepart << " has cache size " << cacheSize<< ", but requires cache size " << requiredCacheSize << " to load index completely into cache!";
    }

    simpleCacheMaxLevel=0;
    for (size_t level=1; level<pageCounts.size(); level++) {
      size_t resultingCacheSize; // Cache size we actually use for this level

      simplePageCache.push_back(PageSimpleCache());

      if (pageCounts[level]>currentCacheSize) {
        resultingCacheSize=currentCacheSize;
        currentCacheSize=0;

        pageCaches.push_back(PageCache(resultingCacheSize));
      }
      else {
        resultingCacheSize=pageCounts[level];
        currentCacheSize-=pageCounts[level];

        simpleCacheMaxLevel=level;

        pageCaches.push_back(PageCache(0));
      }
    }
  }

  template <class N>
  bool NumericIndex<N>::Open(const std::string& path,
                             bool memoryMapped)
  {
    uint32_t    entries;
    FileOffset  lastLevelPageStart;
    FileOffset  indexPageCountsOffset;

    filename=AppendFileToDir(path,filepart);

    try {
      scanner.Open(filename,
                   FileScanner::FastRandom,
                   memoryMapped);

      scanner.ReadNumber(pageSize);                  // Size of one index page
      scanner.ReadNumber(entries);                   // Number of entries in data file

      scanner.Read(levels);                          // Number of levels
      scanner.ReadFileOffset(lastLevelPageStart);    // Start of top level index page
      scanner.ReadFileOffset(indexPageCountsOffset); // Start of list of sizes of index levels

      if (scanner.HasError()) {
        log.Error() << "Error while loading header data of index file '" << filename << "'";
        return false;
      }

      pageCounts.resize(levels);

      scanner.SetPos(indexPageCountsOffset);
      for (size_t level=0; level<levels; level++) {
        scanner.ReadNumber(pageCounts[level]);
      }

      delete [] buffer;
      buffer=new char[pageSize];

      //std::cout << "Index " << filename << ": " << entries << " entries to index, " << levels << " levels, pageSize " << pageSize << ", cache size " << cacheSize << std::endl;

      ReadPage(lastLevelPageStart,root);

      InitializeCache();
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return !scanner.HasError();
  }

  template <class N>
  bool NumericIndex<N>::Close()
  {
    try {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  template <class N>
  bool NumericIndex<N>::IsOpen() const
  {
    return scanner.IsOpen();
  }

  /**
   * Return the file offset in the data file for the given object id.
   *
   * This method is thread-safe.
   */
  template <class N>
  bool NumericIndex<N>::GetOffset(const N& id,
                                  FileOffset& offset) const
  {
    try
    {
      std::lock_guard<std::mutex> lock(accessMutex);

      //std::cout << "Looking up " << id << " in index...." << std::endl;

      size_t                      r=GetPageIndex(*root,id);
      PageRef                     pageRef;

      if (!root->IndexIsValid(r)) {
        //std::cerr << "Id " << id << " not found in root index, " << root->entries.front().startId << "-" << root->entries.back().startId << std::endl;
        return false;
      }

      const Entry& rootEntry=root->entries[r];

      //std::cout << "Root entry index: " << r << " " << rootEntry.startId << " " << rootEntry.fileOffset << std::endl;

      offset=rootEntry.fileOffset;

      N startId=rootEntry.startId;
      for (size_t level=0; level+2<=levels; level++) {
        //std::cout << "Level " << level << "/" << levels << std::endl;
        if (level<=simpleCacheMaxLevel) {
          auto cacheRef=simplePageCache[level].find(startId);

          if (cacheRef==simplePageCache[level].end()) {
            pageRef=nullptr; // Make sure, that we allocate a new page and not reuse an old one

            ReadPage(offset,pageRef);

            simplePageCache[level].insert(std::make_pair(startId,pageRef));
          }
          else {
            pageRef=cacheRef->second;
          }
        }
        else {
          typename PageCache::CacheRef cacheRef;

          if (!pageCaches[level].GetEntry(startId,cacheRef)) {
            typename PageCache::CacheEntry cacheEntry(startId);

            cacheRef=pageCaches[level].SetEntry(cacheEntry);

            ReadPage(offset,cacheRef->value);
          }

          pageRef=cacheRef->value;
        }

        Page& page=*pageRef;

        size_t i=GetPageIndex(page,id);

        if (!page.IndexIsValid(i)) {
          //std::cerr << "Id " << id << " not found in index level " << level+2 << "!" << std::endl;
          return false;
        }

        const Entry& entry=page.entries[i];

        //std::cout << "Sub entry index: " << i << " " << entry.startId << " " << entry.fileOffset << std::endl;

        startId=entry.startId;
        offset=entry.fileOffset;
      }

      if (startId!=id) {
        //std::cerr << "Id " << id << " not found in leaf index level (" << levels << " levels)" << std::endl;
      }

      return startId==id;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  /**
   * Return the file offsets in the data file for the given object ids.
   *
   * This method is thread-safe.
   */
  template <class N>
  template<typename IteratorIn>
  bool NumericIndex<N>::GetOffsets(IteratorIn begin,
                                   IteratorIn end,
                                   size_t size,
                                   std::vector<FileOffset>& offsets) const
  {
    offsets.clear();
    offsets.reserve(size);

    for (IteratorIn idIter=begin; idIter!=end; ++idIter) {
      FileOffset offset;

      if (GetOffset(*idIter,
                    offset)) {
        offsets.push_back(offset);
      }
    }

    return true;
  }

  template <class N>
  void NumericIndex<N>::DumpStatistics() const
  {
    size_t memory=0;
    size_t pages=0;

    pages+=1;
    memory+=root->entries.size()*sizeof(Entry);


    for (size_t i=0; i<pageCaches.size(); i++) {
      pages+=pageCaches[i].GetSize();
      memory+=sizeof(pageCaches[i])+pageCaches[i].GetMemory(NumericIndexCacheValueSizer());
    }

    log.Info() << "Index " << filepart << ": " << pages << " pages, memory " << memory;
  }
}

#endif
