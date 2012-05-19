#ifndef OSMSCOUT_DATAFILE_H
#define OSMSCOUT_DATAFILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <set>
#include <vector>

#include <osmscout/NumericIndex.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  template <class N>
  class DataFile
  {
  public:
    typedef Ref<N> ValueType;

  private:
    typedef NumericIndex<Id>            DataIndex;

    typedef Cache<FileOffset,ValueType> DataCache;

    struct DataCacheValueSizer : public DataCache::ValueSizer
    {
      unsigned long GetSize(const ValueType& value) const
      {
        return sizeof(value);
      }
    };

  private:
    bool                isOpen;        //! If true,the data file is opened
    std::string         datafile;      //! Basename part fo the data file name
    std::string         datafilename;  //! complete filename for data file
    mutable DataCache   cache;         //! Entry cache
    mutable FileScanner scanner;       //! File stream to the data file
    DataIndex           index;         //! Index

  public:
    DataFile(const std::string& datafile,
             const std::string& indexfile,
             unsigned long dataCacheSize,
             unsigned long indexCacheSize);

    virtual ~DataFile();

    bool Open(const std::string& path, bool memoryMapedIndex, bool memoryMapedData);
    bool Close();

    bool GetOffsets(const std::set<Id>& ids,
                    std::vector<FileOffset>& offsets) const;
    bool GetOffsets(const std::vector<Id>& ids,
                    std::vector<FileOffset>& offsets) const;

    bool GetByOffset(const std::vector<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::list<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::set<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;

    bool Get(const std::vector<Id>& ids,
             std::vector<ValueType>& data) const;
    bool Get(const std::list<Id>& ids,
             std::vector<ValueType>& data) const;
    bool Get(const std::set<Id>& ids,
             std::vector<ValueType>& data) const;

    bool Get(const Id& id, ValueType& entry) const;

    void FlushCache();
    void DumpStatistics() const;
  };

  template <class N>
  DataFile<N>::DataFile(const std::string& datafile,
                        const std::string& indexfile,
                        unsigned long dataCacheSize,
                        unsigned long indexCacheSize)
  : isOpen(false),
    datafile(datafile),
    cache(dataCacheSize),
    index(indexfile,indexCacheSize)
  {
    // no code
  }

  template <class N>
  DataFile<N>::~DataFile()
  {
    if (isOpen) {
      Close();
    }
  }

  template <class N>
  bool DataFile<N>::Open(const std::string& path, bool memoryMapedIndex, bool memoryMapedData)
  {
    datafilename=AppendFileToDir(path,datafile);

    isOpen=index.Open(path,memoryMapedIndex) && scanner.Open(datafilename,true,memoryMapedData);

    return isOpen;
  }

  template <class N>
  bool DataFile<N>::Close()
  {
    bool success=true;

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    if (!index.Close()) {
      success=false;
    }

    isOpen=false;

    return success;
  }

  template <class N>
  bool DataFile<N>::GetOffsets(const std::set<Id>& ids,
                               std::vector<FileOffset>& offsets) const
  {
    return index.GetOffsets(ids,offsets);
  }

  template <class N>
  bool DataFile<N>::GetOffsets(const std::vector<Id>& ids,
                               std::vector<FileOffset>& offsets) const
  {
    return index.GetOffsets(ids,offsets);
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::vector<FileOffset>& offsets,
                        std::vector<ValueType>& data) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    data.reserve(data.size()+offsets.size());

    typename DataCache::CacheRef cacheRef;

    for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
         offset!=offsets.end();
         ++offset) {
      if (!cache.GetEntry(*offset,cacheRef)) {
        typename DataCache::CacheEntry cacheEntry(*offset);

        cacheRef=cache.SetEntry(cacheEntry);

        scanner.SetPos(*offset);
        cacheRef->value=new N();
        cacheRef->value->Read(scanner);

        if (scanner.HasError()) {
          std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }
      }

      data.push_back(cacheRef->value);
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::list<FileOffset>& offsets,
                        std::vector<ValueType>& data) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    data.reserve(data.size()+offsets.size());

    typename DataCache::CacheRef cacheRef;

    for (std::list<FileOffset>::const_iterator offset=offsets.begin();
         offset!=offsets.end();
         ++offset) {
      if (!cache.GetEntry(*offset,cacheRef)) {
        typename DataCache::CacheEntry cacheEntry(*offset);

        cacheRef=cache.SetEntry(cacheEntry);

        scanner.SetPos(*offset);
        cacheRef->value=new N();
        cacheRef->value->Read(scanner);

        if (scanner.HasError()) {
          std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }
      }

      data.push_back(cacheRef->value);
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::set<FileOffset>& offsets,
                        std::vector<ValueType>& data) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    data.reserve(data.size()+offsets.size());

    typename DataCache::CacheRef cacheRef;

    for (std::set<FileOffset>::const_iterator offset=offsets.begin();
         offset!=offsets.end();
         ++offset) {
      if (!cache.GetEntry(*offset,cacheRef)) {
        typename DataCache::CacheEntry cacheEntry(*offset);

        cacheRef=cache.SetEntry(cacheEntry);

        scanner.SetPos(*offset);
        cacheRef->value=new N();
        cacheRef->value->Read(scanner);

        if (scanner.HasError()) {
          std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }
      }

      data.push_back(cacheRef->value);
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::Get(const std::vector<Id>& ids,
                        std::vector<ValueType>& data) const
  {
    assert(isOpen);

    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids,offsets)) {
      std::cerr << "Ids not found in index" << std::endl;
      return false;
    }

    return GetByOffset(offsets,data);
  }

  template <class N>
  bool DataFile<N>::Get(const std::list<Id>& ids,
                        std::vector<ValueType>& data) const
  {
    assert(isOpen);

    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids,offsets)) {
      std::cerr << "Ids not found in index" << std::endl;
      return false;
    }

    return GetByOffset(offsets,data);
  }

  template <class N>
  bool DataFile<N>::Get(const std::set<Id>& ids,
                        std::vector<ValueType>& data) const
  {
    assert(isOpen);

    std::vector<Id> i;

    i.reserve(ids.size());
    for (std::set<Id>::const_iterator id=ids.begin();
         id!=ids.end();
         ++id) {
      i.push_back(*id);
    }

    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(i,offsets)) {
      std::cerr << "Ids not found in index" << std::endl;
      return false;
    }

    return GetByOffset(offsets,data);
  }

  template <class N>
  bool DataFile<N>::Get(const Id& id, ValueType& entry) const
  {
    assert(isOpen);

    std::vector<Id>        ids;
    std::vector<ValueType> data;

    ids.push_back(id);

    if (Get(ids,data) && data.size()==1) {
      entry=data.front();
      return true;
    }
    else {
      return false;
    }
  }

  template <class N>
  void DataFile<N>::FlushCache()
  {
    cache.Flush();
  }

  template <class N>
  void DataFile<N>::DumpStatistics() const
  {
    cache.DumpStatistics(datafile.c_str(),DataCacheValueSizer());
    index.DumpStatistics();
  }
}

#endif
