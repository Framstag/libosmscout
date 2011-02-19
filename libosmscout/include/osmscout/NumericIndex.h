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
    struct IndexEntry
    {
      Id         startId;
      FileOffset fileOffset;
    };

    typedef Cache<Id,std::vector<IndexEntry> > PageCache;

    /**
      Returns the size of a individual cache entry
      */
    struct NumericIndexCacheValueSizer : public PageCache::ValueSizer
    {
      unsigned long GetSize(const std::vector<IndexEntry>& value) const
      {
        return sizeof(value)+sizeof(IndexEntry)*value.size();
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
    std::vector<IndexEntry>        root;
    mutable std::vector<PageCache> leafs;

  private:
    size_t GetPageIndex(const std::vector<IndexEntry>& index,
                        Id id) const;
    bool ReadPage(FileOffset offset,
                  std::vector<IndexEntry>& index) const;

  public:
    NumericIndex(const std::string& filename,
                 unsigned long cacheSize);
    virtual ~NumericIndex();

    bool Load(const std::string& path);

    bool GetOffsets(const std::vector<N>& ids, std::vector<FileOffset>& offsets) const;

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
    delete buffer;
  }

  /**
    Binary search for index page for given id
    */
  template <class N, class T>
  inline size_t NumericIndex<N,T>::GetPageIndex(const std::vector<IndexEntry>& index,
                                                Id id) const
  {
    int size=index.size();

    if (size>0) {
      int left=0;
      int right=size-1;
      int mid;

      while (left<=right) {
        mid=(left+right)/2;
        if (index[mid].startId<=id &&
            (mid+1>=size || index[mid+1].startId>id)) {
          return mid;
        }
        else if (index[mid].startId<id) {
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
  inline bool NumericIndex<N,T>::ReadPage(FileOffset offset,
                                          std::vector<IndexEntry>& index) const
  {
    FileOffset currentPos;
    Id         sio=0;
    FileOffset poo=0;

    index.clear();

    if (!scanner.IsOpen() &&
        !scanner.Open(filename)) {
      std::cerr << "Cannot open '" << filename << "'!" << std::endl;
      return false;
    }

    scanner.SetPos(offset);

    while (scanner.GetPos(currentPos) &&
           currentPos<offset+(FileOffset)pageSize) {
      IndexEntry entry;
      Id         si;
      FileOffset po;

      scanner.ReadNumber(si);

      if (si==0)  {
        return !scanner.HasError();
      }

      scanner.ReadNumber(po);

      sio+=si;
      poo+=po;

      entry.startId=sio;
      entry.fileOffset=poo;

      index.push_back(entry);
    }

    return !scanner.HasError();
  }

  template <class N, class T>
  bool NumericIndex<N,T>::Load(const std::string& path)
  {
    uint32_t    entries;
    FileOffset  lastLevelPageStart;
    FileOffset  indexPageCountsOffset;

    filename=AppendFileToDir(path,filepart);

    if (!scanner.Open(filename)) {
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

    root.reserve(pageSize);

    ReadPage(lastLevelPageStart,root);

    unsigned long currentCacheSize=cacheSize;
    for (size_t i=1; i<pageCounts.size(); i++) {
      unsigned long resultingCacheSize;

      if (currentCacheSize==0) {
        resultingCacheSize=1;
      }
      else if (pageCounts[i]>currentCacheSize) {
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

    return !scanner.HasError() && scanner.Close();
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
      size_t r=GetPageIndex(root,*id);

      if (r>=root.size()) {
        //std::cerr << "Id " << *id << " not found in root index!" << std::endl;
        continue;
      }

      Id         startId=root[r].startId;
      FileOffset offset=root[r].fileOffset;
      bool       error=false;

      for (size_t level=0; level<=levels-2 && !error; level++) {
        typename PageCache::CacheRef cacheRef;

        if (!leafs[level].GetEntry(startId,cacheRef)) {
          typename PageCache::CacheEntry cacheEntry(startId);

          cacheRef=leafs[level].SetEntry(cacheEntry);

          cacheRef->value.reserve(pageSize);

          ReadPage(offset,cacheRef->value);
        }

        size_t i=GetPageIndex(cacheRef->value,*id);

        if (i>=cacheRef->value.size()) {
          //std::cerr << "Id " << *id << " not found in index level " << level+2 << "!" << std::endl;
          error=true;
          continue;
        }

        startId=cacheRef->value[i].startId;
        offset=cacheRef->value[i].fileOffset;
      }

      if (!error &&
          startId==*id) {
        offsets.push_back(offset);
      }
    }

    return true;
  }

  template <class N,class T>
  void NumericIndex<N,T>::DumpStatistics() const
  {
    size_t memory=0;
    size_t entries=0;

    entries+=root.size();
    memory+=root.size()*sizeof(IndexEntry);

    for (size_t i=0; i<leafs.size(); i++) {
      entries+=leafs[i].GetSize();
      memory+=sizeof(leafs[i])+leafs[i].GetMemory(NumericIndexCacheValueSizer());
    }

    std::cout << "Index " << filepart << ": " << entries << " entries, memory " << memory << std::endl;
  }
}

#endif
