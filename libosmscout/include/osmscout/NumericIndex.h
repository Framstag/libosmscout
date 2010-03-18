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

#include <cmath>
#include <list>
#include <set>

// Index
#include <osmscout/Cache.h>
#include <osmscout/FileScanner.h>
#include <osmscout/TypeConfig.h>

// Index generation
#include <osmscout/FileWriter.h>
#include <osmscout/Import.h>
#include <osmscout/Progress.h>

#include <osmscout/Util.h>

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
      size_t startId;
      size_t fileOffset;
    };

    typedef Cache<FileOffset,std::vector<IndexEntry> > PageCache;

    /**
      Returns the size of a individual cache entry
      */
    struct NumericIndexCacheValueSizer : public PageCache::ValueSizer
    {
      size_t GetSize(const std::vector<IndexEntry>& value) const
      {
        return sizeof(IndexEntry)*value.size();
      }
    };

  private:
    std::string                    filepart;
    std::string                    filename;
    mutable FileScanner            scanner;
    size_t                         levels;
    size_t                         levelSize;
    std::vector<IndexEntry>        root;
    mutable std::vector<PageCache> leafs;

  public:
    NumericIndex(const std::string& filename);
    virtual ~NumericIndex();

    bool Load(const std::string& path);

    bool GetOffsets(const std::vector<N>& ids, std::vector<FileOffset>& offsets) const;

    void DumpStatistics() const;
  };

  template <class N, class T>
  NumericIndex<N,T>::NumericIndex(const std::string& filename)
   : filepart(filename),
     levels(0),
     levelSize(0)
  {
    // no code
  }

  template <class N, class T>
  NumericIndex<N,T>::~NumericIndex()
  {
    // no code
  }

  template <class N, class T>
  bool NumericIndex<N,T>::Load(const std::string& path)
  {
    size_t      entries;
    size_t      lastLevelPageStart;

    filename=path+"/"+filepart;

    if (!scanner.Open(filename)) {
      return false;
    }

    scanner.ReadNumber(levels);
    scanner.ReadNumber(levelSize);
    scanner.Read(entries);
    scanner.Read(lastLevelPageStart);

    std::cout << filepart <<": " << levels << " " << levelSize << " " << entries << " " << lastLevelPageStart << std::endl;

    // Calculate the number of entries in the first level
    size_t levelEntries=entries;

    for (size_t l=1;l<levels; l++) {
      if (levelEntries%levelSize!=0) {
        levelEntries=levelEntries/levelSize+1;
      }
      else {
        levelEntries=levelEntries/levelSize;
      }
    }

    size_t sio=0;
    size_t poo=0;

    std::cout << levelEntries << " entries in first level" << std::endl;

    scanner.SetPos(lastLevelPageStart);

    for (size_t i=0; i<levelEntries; i++) {
      IndexEntry entry;
      size_t     si;
      size_t     po;

      scanner.ReadNumber(si);
      scanner.ReadNumber(po);

      sio+=si;
      poo+=po;

      entry.startId=sio;
      entry.fileOffset=poo;

      root.push_back(entry);
    }

    for (size_t i=1; i<=levels-1; i++) {
      leafs.push_back(PageCache(1000000));
    }

    return !scanner.HasError() && scanner.Close();
  }

  template <class N, class T>
  bool NumericIndex<N,T>::GetOffsets(const std::vector<N>& ids,
                                     std::vector<FileOffset>& offsets) const
  {
    offsets.reserve(ids.size());
    offsets.clear();

    if (ids.size()==0) {
      return true;
    }

    for (typename std::vector<N>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      size_t r=0;

      while (r+1<root.size() && root[r+1].startId<=*id) {
        r++;
      }

      if (r<root.size()) {
        size_t     startId=root[r].startId;
        FileOffset offset=root[r].fileOffset;

        for (size_t level=0; level<levels-1; level++) {
          class PageCache::CacheRef cacheRef;

          if (!leafs[level].GetEntry(startId,cacheRef)) {
            class PageCache::CacheEntry cacheEntry(startId);

            cacheRef=leafs[level].SetEntry(cacheEntry);

            if (!scanner.IsOpen() &&
                !scanner.Open(filename)) {
              std::cerr << "Cannot open '" << filename << "'!" << std::endl;
              return false;
            }

            scanner.SetPos(offset);

            cacheRef->value.reserve(levelSize);

            IndexEntry entry;

            entry.startId=0;
            entry.fileOffset=0;

            for (size_t j=0; j<levelSize; j++) {
              size_t cidx;
              size_t coff;

              scanner.ReadNumber(cidx);
              scanner.ReadNumber(coff);

              if (scanner.HasError()) {
                // This is a hack, for the last page we simply ran behind the end of file and
                // create an error condition. We should add an IsEOF method to the FileScanner
                // and thus avoid creating EOFconditions at all. However until that time
                // we simply close the file (and reopen it later) to clear the error flag.
                scanner.Close();
                break;
              }

              entry.startId+=cidx;
              entry.fileOffset+=coff;

              cacheRef->value.push_back(entry);
            }
          }

          size_t i=0;
          while (i+1<cacheRef->value.size() &&
                 cacheRef->value[i+1].startId<=*id) {
            i++;
          }

          if (i<cacheRef->value.size()) {
            startId=cacheRef->value[i].startId;
            offset=cacheRef->value[i].fileOffset;
            //std::cout << "id " << *id <<" => " << i << " " << startId << " " << offset << std::endl;
          }
          else {
            //std::cerr << "Id " << *id << " not found in sub page index!" << std::endl;
          }
        }

        if (*id==startId) {
          offsets.push_back(offset);
        }
        else {
          //std::cerr << "Id " << *id << " not found in sub index!" << std::endl;
        }
        //std::cout << "=> Id " << *id <<" => " << startId << " " << offset << std::endl;
      }
      else {
        //std::cerr << "Id " << *id << " not found in root index!" << std::endl;
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
    memory+=root.size()+sizeof(IndexEntry);

    for (size_t i=0; i<leafs.size(); i++) {
      entries+=leafs[i].GetSize();
      memory+=leafs[i].GetSize()+leafs[i].GetMemory(NumericIndexCacheValueSizer());

      // TODO: Calculate size of cache
    }

    std::cout << "Index " << filepart << ": " << entries << " entries, " << memory << std::endl;
  }

  template <class N,class T>
  bool GenerateNumericIndex(const ImportParameter& parameter,
                            Progress& progress,
                            const std::string& datafile,
                            const std::string& indexfile)
  {
    //
    // Writing index2 file
    //

    progress.SetAction(std::string("Generating '")+indexfile+"'");

    FileScanner             scanner;
    FileWriter              writer;
    size_t                  dataCount=0;
    std::vector<Id>         startingIds;
    std::vector<FileOffset> pageStarts;
    FileOffset              lastLevelPageStart;

    if (!scanner.Open(datafile)) {
      progress.Error(std::string("Cannot open '")+datafile+"'");
      return false;
    }

    while (!scanner.HasError()) {
      T data;

      data.Read(scanner);

      if (!scanner.HasError()) {
        dataCount++;
      }
    }

    scanner.Close();

    if (!writer.Open(indexfile)) {
      progress.Error(std::string("Cannot create '")+indexfile+"'");
      return false;
    }

    if (!scanner.Open(datafile)) {
      progress.Error(std::string("Cannot open '")+datafile+"'");
      return false;
    }

    size_t levels=1;
    size_t tmp;
    size_t indexLevelSize=parameter.GetNumericIndexLevelSize();

    tmp=dataCount;
    while (tmp/indexLevelSize>0) {
      tmp=tmp/indexLevelSize;
      levels++;
    }

    indexLevelSize=ceil(pow(dataCount,1.0/levels));

    progress.Info(NumberToString(dataCount)+" entries will be stored in "+NumberToString(levels)+ " levels using index level size of "+NumberToString(indexLevelSize));


    writer.WriteNumber(levels); // Number of levels
    writer.WriteNumber(indexLevelSize); // Size of index page
    writer.Write(dataCount);        // Number of nodes

    writer.GetPos(lastLevelPageStart);

    writer.Write((unsigned long)0); // Write the starting position of the last page

    size_t     lastId=0;
    FileOffset lastPos=0;

    progress.Info(std::string("Level ")+NumberToString(levels)+" entries "+NumberToString(dataCount));

    for (size_t i=0; i<dataCount; i++) {
      FileOffset pos;

      scanner.GetPos(pos);

      T data;

      data.Read(scanner);

      if (scanner.HasError()) {
        return false;
      }

      if (i%indexLevelSize==0) {
        FileOffset pageStart;

        writer.GetPos(pageStart);

        writer.WriteNumber(data.id);
        writer.WriteNumber(pos);
        startingIds.push_back(data.id);
        pageStarts.push_back(pageStart);
      }
      else {
        writer.WriteNumber(data.id-lastId);
        writer.WriteNumber(pos-lastPos);
      }

      lastPos=pos;
      lastId=data.id;
    }

    levels--;

    while (levels>0) {
      std::vector<Id>         si(startingIds);
      std::vector<FileOffset> po(pageStarts);

      startingIds.clear();
      pageStarts.clear();

      progress.Info(std::string("Level ")+NumberToString(levels)+" entries "+NumberToString(si.size()));

      for (size_t i=0; i<si.size(); i++) {
        if (i%indexLevelSize==0) {
          FileOffset pageStart;

          writer.GetPos(pageStart);

          startingIds.push_back(si[i]);
          pageStarts.push_back(pageStart);
        }

        if (i%indexLevelSize==0) {
          writer.WriteNumber(si[i]);
          writer.WriteNumber(po[i]);
        }
        else {
          writer.WriteNumber(si[i]-si[i-1]);
          writer.WriteNumber(po[i]-po[i-1]);
        }
      }

      levels--;
    }

    writer.SetPos(lastLevelPageStart);

    writer.Write((unsigned long)pageStarts[0]);

    scanner.Close();

    return !writer.HasError() && writer.Close();
  }
}

#endif
