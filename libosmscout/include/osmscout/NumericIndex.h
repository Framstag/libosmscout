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

#include <vector>

#include <osmscout/TypeConfig.h>

#include <osmscout/Util.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Reference.h>
#include <osmscout/util/String.h>

namespace osmscout {

  /**
    Numeric index handles an index over instance of class <T> where the index criteria
    is of type <N>, where <N> has a numeric nature (usually Id).
    */
  template <class N, class T>
  class NumericIndex
  {
  private:
    /**
      an individual index entry.
      */
    struct Entry
    {
      Id         startId;
      FileOffset fileOffset;
    };

    struct Page : public Referencable
    {
      std::vector<Entry> entries;

      bool IndexIsValid(size_t index)
      {
        return index<entries.size();
      }
    };

    typedef LazyRef<Page> PageRef;

    typedef Cache<Id,PageRef> PageCache;

    /**
      Returns the size of a individual cache entry
      */
    struct NumericIndexCacheValueSizer : public PageCache::ValueSizer
    {
      unsigned long GetSize(const PageRef& value) const
      {
        return sizeof(value)+sizeof(Page)+sizeof(Entry)*value->entries.size();
      }
    };

  private:
    std::string                    filepart;
    std::string                    filename;
    unsigned long                  cacheSize;
    mutable FileScanner            scanner;
    uint32_t                       pageSize;
    uint32_t                       levels;
    std::vector<uint32_t>          pageCounts;
    char                           *buffer;
    PageRef                        root;
    mutable std::vector<PageCache> leafs;

  private:
    size_t GetPageIndex(const PageRef& page, Id id) const;
    bool ReadPage(FileOffset offset, PageRef& page) const;

  public:
    NumericIndex(const std::string& filename,
                 unsigned long cacheSize);
    virtual ~NumericIndex();

    bool Open(const std::string& path, bool memoryMaped);
    bool Close();

    bool GetOffset(const N& id, FileOffset& offset) const;
    bool GetOffsets(const std::vector<N>& ids, std::vector<FileOffset>& offsets) const;
    bool GetOffsets(const std::list<N>& ids, std::vector<FileOffset>& offsets) const;
    bool GetOffsets(const std::set<N>& ids, std::vector<FileOffset>& offsets) const;

    void DumpStatistics() const;
  };

  template <class N, class T>
  NumericIndex<N,T>::NumericIndex(const std::string& filename,
                                  unsigned long cacheSize)
   : filepart(filename),
     cacheSize(cacheSize),
     pageSize(0),
     levels(0),
     buffer(NULL)
  {
    // no code
  }

  template <class N, class T>
  NumericIndex<N,T>::~NumericIndex()
  {
    Close();

    delete [] buffer;
  }

  /**
    Binary search for index page for given id
    */
  template <class N, class T>
  inline size_t NumericIndex<N,T>::GetPageIndex(const PageRef& page, Id id) const
  {
    int size=page->entries.size();

    if (size>0) {
      int left=0;
      int right=size-1;
      int mid;

      while (left<=right) {
        mid=(left+right)/2;
        if (page->entries[mid].startId<=id &&
            (mid+1>=size || page->entries[mid+1].startId>id)) {
          return mid;
        }
        else if (page->entries[mid].startId<id) {
          left=mid+1;
        }
        else {
          right=mid-1;
        }
      }
    }

    return size;
  }

  template <class N, class T>
  inline bool NumericIndex<N,T>::ReadPage(FileOffset offset, PageRef& page) const
  {
    page->entries.clear();
    page->entries.reserve(pageSize);

    if (!scanner.IsOpen() &&
        !scanner.Open(filename)) {
      std::cerr << "Cannot open '" << filename << "'!" << std::endl;
      return false;
    }

    scanner.SetPos(offset);

    if (!scanner.Read(buffer,pageSize)) {
      std::cerr << "Cannot read index page from file '" << filename << "'!" << std::endl;
      return false;
    }

    size_t     currentPos=0;
    Id         sio=0;
    FileOffset poo=0;

    while (currentPos<pageSize &&
           buffer[currentPos]!=0) {
      uint32_t   data;
      size_t     bytes;
      Id         si;
      FileOffset po;
      Entry      entry;

      DecodeNumber(&buffer[currentPos],
                   data,
                   bytes);

      si=(Id)data;
      currentPos+=bytes;

      DecodeNumber(&buffer[currentPos],
                   data,
                   bytes);

      po=(FileOffset)data;
      currentPos+=bytes;

      sio+=si;
      poo+=po;

      entry.startId=sio;
      entry.fileOffset=poo;

      page->entries.push_back(entry);
    }

    return !scanner.HasError();
  }

  template <class N, class T>
  bool NumericIndex<N,T>::Open(const std::string& path, bool memoryMaped)
  {
    uint32_t    entries;
    FileOffset  lastLevelPageStart;
    FileOffset  indexPageCountsOffset;

    filename=AppendFileToDir(path,filepart);

    if (!scanner.Open(filename,true, memoryMaped)) {
      std::cerr << "Cannot open index file '" << filename << "'" << std::endl;
      return false;
    }

    scanner.ReadNumber(pageSize);        // Size of one index page
    scanner.ReadNumber(entries);         // Number of entries in data file

    scanner.Read(levels);                // Number of levels
    scanner.Read(lastLevelPageStart);    // Start of top level index page
    scanner.Read(indexPageCountsOffset); // Start of list of sizes of index levels

    if (scanner.HasError()) {
      std::cerr << "Error while loading header data of index file '" << filename << "'" << std::endl;
      return false;
    }

    pageCounts.resize(levels);

    scanner.SetPos(indexPageCountsOffset);
    for (size_t i=0; i<levels; i++) {
      scanner.ReadNumber(pageCounts[pageCounts.size()-1-i]);
    }

    delete buffer;
    buffer=new char[pageSize];

    std::cout << entries << " entries to index, " << levels << " levels, pageSize " << pageSize << ", cache size " << cacheSize << std::endl;

    ReadPage(lastLevelPageStart,root);

    unsigned long currentCacheSize=cacheSize;
    for (size_t i=1; i<pageCounts.size(); i++) {
      unsigned long resultingCacheSize;

      if (pageCounts[i]>currentCacheSize) {
        resultingCacheSize=currentCacheSize;
        currentCacheSize=0;
      }
      else {
        resultingCacheSize=pageCounts[i];
        currentCacheSize-=pageCounts[i];
      }

      std::cout << "Setting cache size for level " << i+1 << " with " << pageCounts[i] << " entries to " << resultingCacheSize << std::endl;

      leafs.push_back(PageCache(resultingCacheSize));
    }

    return !scanner.HasError();
  }

  template <class N, class T>
  bool NumericIndex<N,T>::Close()
  {
    if (scanner.IsOpen()) {
      return scanner.Close();
    }

    return true;
  }

  template <class N, class T>
  bool NumericIndex<N,T>::GetOffset(const N& id,
                                    FileOffset& offset) const
  {
    size_t r=GetPageIndex(root,id);

    if (!root->IndexIsValid(r)) {
      //std::cerr << "Id " << id << " not found in root index!" << std::endl;
      return false;
    }

    offset=root->entries[r].fileOffset;

    Id startId=root->entries[r].startId;
    for (size_t level=0; level+2<=levels; level++) {
      typename PageCache::CacheRef cacheRef;

      if (!leafs[level].GetEntry(startId,cacheRef)) {
        typename PageCache::CacheEntry cacheEntry(startId);

        cacheRef=leafs[level].SetEntry(cacheEntry);

        ReadPage(offset,cacheRef->value);
      }

      size_t i=GetPageIndex(cacheRef->value,id);

      if (!cacheRef->value->IndexIsValid(i)) {
        //std::cerr << "Id " << id << " not found in index level " << level+2 << "!" << std::endl;
        return false;
      }

      startId=cacheRef->value->entries[i].startId;
      offset=cacheRef->value->entries[i].fileOffset;
    }

    /*
    if (startId!=id) {
      std::cerr << "Id " << id << " not found in leaf index level!"  << " " << levels << std::endl;
    }*/

    return startId==id;
  }

  template <class N, class T>
  bool NumericIndex<N,T>::GetOffsets(const std::vector<N>& ids,
                                     std::vector<FileOffset>& offsets) const
  {
    offsets.reserve(ids.size());
    offsets.clear();

    for (typename std::vector<N>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      FileOffset offset;

      if (GetOffset(*id,
                    offset)) {
        offsets.push_back(offset);
      }
    }

    return true;
  }

  template <class N, class T>
  bool NumericIndex<N,T>::GetOffsets(const std::list<N>& ids,
                                     std::vector<FileOffset>& offsets) const
  {
    offsets.reserve(ids.size());
    offsets.clear();

    for (typename std::list<N>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      FileOffset offset;

      if (GetOffset(*id,
                    offset)) {
        offsets.push_back(offset);
      }
    }

    return true;
  }

  template <class N, class T>
  bool NumericIndex<N,T>::GetOffsets(const std::set<N>& ids,
                                     std::vector<FileOffset>& offsets) const
  {
    offsets.reserve(ids.size());
    offsets.clear();

    for (typename std::set<N>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      FileOffset offset;

      if (GetOffset(*id,
                    offset)) {
        offsets.push_back(offset);
      }
    }

    return true;
  }

  template <class N,class T>
  void NumericIndex<N,T>::DumpStatistics() const
  {
    size_t memory=0;
    size_t pages=0;

    pages+=1;
    memory+=root->entries.size()*sizeof(Entry);


    for (size_t i=0; i<leafs.size(); i++) {
      pages+=leafs[i].GetSize();
      memory+=sizeof(leafs[i])+leafs[i].GetMemory(NumericIndexCacheValueSizer());
    }

    std::cout << "Index " << filepart << ": " << pages << " pages, memory " << memory << std::endl;
  }
}

#endif
