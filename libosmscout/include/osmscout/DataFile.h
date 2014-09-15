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

  /**
   * \ingroup Database
   *
   * Access to standard format data files.
   *
   * Allows to load data objects by offset using various standard library data structures.
   */
  template <class N>
  class DataFile : public Referencable
  {
  public:
    typedef Ref<N> ValueType;

  private:
    typedef Cache<FileOffset,ValueType> DataCache;

    struct DataCacheValueSizer : public DataCache::ValueSizer
    {
      unsigned long GetSize(const ValueType& value) const
      {
        return sizeof(value);
      }
    };

  private:
    std::string         datafile;        //! Basename part fo the data file name
    std::string         datafilename;    //! complete filename for data file
    FileScanner::Mode   modeData;        //! Type of file access
    bool                memoryMapedData; //! Use memory mapped files for data access
    mutable DataCache   cache;           //! Entry cache
    mutable FileScanner scanner;         //! File stream to the data file

  protected:
    bool                isOpen;          //! If true,the data file is opened
    TypeConfigRef       typeConfig;

  private:
    bool ReadData(const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  N& data) const;

  public:
    DataFile(const std::string& datafile,
                 unsigned long dataCacheSize);

    virtual ~DataFile();

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              FileScanner::Mode modeData,
              bool memoryMapedData);
    bool IsOpen() const;
    bool Close();

    bool GetByOffset(const std::vector<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::list<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::set<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;

    bool GetByOffset(const std::set<FileOffset>& offsets,
                     OSMSCOUT_HASHMAP<FileOffset,ValueType>& dataMap) const;

    bool GetByOffset(const FileOffset& offset,
                     ValueType& entry) const;

    void FlushCache();
    void DumpStatistics() const;
  };

  template <class N>
  DataFile<N>::DataFile(const std::string& datafile,
                        unsigned long dataCacheSize)
  : datafile(datafile),
    modeData(FileScanner::LowMemRandom),
    memoryMapedData(false),
    cache(dataCacheSize),
    isOpen(false)

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
  bool DataFile<N>::ReadData(const TypeConfig& typeConfig,
                             FileScanner& scanner,
                             N& data) const
  {
    return data.Read(typeConfig,
                     scanner);
  }

  template <class N>
  bool DataFile<N>::Open(const TypeConfigRef& typeConfig,
                         const std::string& path,
                         FileScanner::Mode modeData,
                         bool memoryMapedData)
  {
    this->typeConfig=typeConfig;

    datafilename=AppendFileToDir(path,datafile);

    this->memoryMapedData=memoryMapedData;
    this->modeData=modeData;

    isOpen=scanner.Open(datafilename,modeData,memoryMapedData);

    return isOpen;
  }

  template <class N>
  bool DataFile<N>::IsOpen() const
  {
    return isOpen;
  }

  template <class N>
  bool DataFile<N>::Close()
  {
    bool success=true;

    typeConfig=NULL;

    if (scanner.IsOpen()) {
      if (!scanner.Close()) {
        success=false;
      }
    }

    isOpen=false;
    cache.Flush();

    return success;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::vector<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,modeData,memoryMapedData)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    data.reserve(data.size()+offsets.size());

    if (!cache.IsActive()) {
      for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
           offset!=offsets.end();
           ++offset) {
        N *value=new N();

        scanner.SetPos(*offset);

        if (!ReadData(typeConfig,
                      scanner,
                      *value)) {
          std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }

        data.push_back(value);
      }
    }
    else {
      typename DataCache::CacheRef cacheRef;

      for (std::vector<FileOffset>::const_iterator offset=offsets.begin();
           offset!=offsets.end();
           ++offset) {
        if (!cache.GetEntry(*offset,cacheRef)) {
          typename DataCache::CacheEntry cacheEntry(*offset);

          cacheRef=cache.SetEntry(cacheEntry);

          scanner.SetPos(*offset);
          cacheRef->value=new N();

          if (!ReadData(typeConfig,
                        scanner,
                        *cacheRef->value)) {
            std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
            // TODO: Remove broken entry from cache
            scanner.Close();
            return false;
          }
        }

        data.push_back(cacheRef->value);
      }
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::list<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,modeData,memoryMapedData)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    data.reserve(data.size()+offsets.size());

    if (!cache.IsActive()) {
      for (std::list<FileOffset>::const_iterator offset=offsets.begin();
           offset!=offsets.end();
           ++offset) {
        N *value=new N();

        scanner.SetPos(*offset);

        if (!ReadData(typeConfig,
                      scanner,
                      *value)) {
          std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }

        data.push_back(value);
      }
    }
    else {
      typename DataCache::CacheRef cacheRef;

      for (std::list<FileOffset>::const_iterator offset=offsets.begin();
           offset!=offsets.end();
           ++offset) {
        if (!cache.GetEntry(*offset,cacheRef)) {
          typename DataCache::CacheEntry cacheEntry(*offset);

          cacheRef=cache.SetEntry(cacheEntry);

          scanner.SetPos(*offset);
          cacheRef->value=new N();

          if (!ReadData(typeConfig,
                        scanner,
                        *cacheRef->value)) {
            std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
            // TODO: Remove broken entry from cache
            scanner.Close();
            return false;
          }
        }

        data.push_back(cacheRef->value);
      }
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::set<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,modeData,memoryMapedData)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    data.reserve(data.size()+offsets.size());

    if (!cache.IsActive()) {
      for (std::set<FileOffset>::const_iterator offset=offsets.begin();
           offset!=offsets.end();
           ++offset) {
        N *value=new N();

        scanner.SetPos(*offset);

        if (!ReadData(typeConfig,
                      scanner,
                      *value)) {
          std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }

        data.push_back(value);
      }
    }
    else {
      typename DataCache::CacheRef cacheRef;

      for (std::set<FileOffset>::const_iterator offset=offsets.begin();
           offset!=offsets.end();
           ++offset) {
        if (!cache.GetEntry(*offset,cacheRef)) {
          typename DataCache::CacheEntry cacheEntry(*offset);

          cacheRef=cache.SetEntry(cacheEntry);

          scanner.SetPos(*offset);
          cacheRef->value=new N();

          if (!ReadData(typeConfig,
                        scanner,
                        *cacheRef->value)) {
            std::cerr << "Error while reading data from offset " << *offset << " of file " << datafilename << "!" << std::endl;
            // TODO: Remove broken entry from cache
            scanner.Close();
            return false;
          }
        }

        data.push_back(cacheRef->value);
      }
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::set<FileOffset>& offsets,
                                OSMSCOUT_HASHMAP<FileOffset,ValueType>& dataMap) const
  {
    std::vector<ValueType> data;

    if (!GetByOffset(offsets,data)) {
      return false;
    }

    for (typename std::vector<ValueType>::const_iterator v=data.begin();
            v!=data.end();
            ++v) {
      dataMap.insert(std::make_pair((*v)->GetFileOffset(),*v));
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const FileOffset& offset,
                                ValueType& entry) const
  {
    assert(isOpen);

    if (!scanner.IsOpen()) {
      if (!scanner.Open(datafilename,modeData,memoryMapedData)) {
        std::cerr << "Error while opening " << datafilename << " for reading!" << std::endl;
        return false;
      }
    }

    if (!cache.IsActive()) {
      N *value=new N();

      scanner.SetPos(offset);

      if (!ReadData(typeConfig,
                    scanner,
                    *value)) {
        std::cerr << "Error while reading data from offset " << offset << " of file " << datafilename << "!" << std::endl;
        // TODO: Remove broken entry from cache
        scanner.Close();
        return false;
      }

      entry=value;
    }
    else {
      typename DataCache::CacheRef cacheRef;

      if (!cache.GetEntry(offset,cacheRef)) {
        typename DataCache::CacheEntry cacheEntry(offset);

        cacheRef=cache.SetEntry(cacheEntry);

        scanner.SetPos(offset);
        cacheRef->value=new N();

        if (!ReadData(typeConfig,
                      scanner,
                      *cacheRef->value)) {
          std::cerr << "Error while reading data from offset " << offset << " of file " << datafilename << "!" << std::endl;
          // TODO: Remove broken entry from cache
          scanner.Close();
          return false;
        }
      }

      entry=cacheRef->value;
    }

    return true;
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
  }


  /**
   * \ingroup Database
   *
   * Extension of DataFile to allow loading data not only by offset but
   * by id using an additional index file, mapping objects id to object
   * file offset.
   */
  template <class I, class N>
  class IndexedDataFile : public DataFile<N>
  {
  public:
    typedef Ref<N> ValueType;

  private:
    typedef NumericIndex<I> DataIndex;

  private:
    DataIndex     index;
    TypeConfigRef typeConfig;

  public:
    IndexedDataFile(const std::string& datafile,
                    const std::string& indexfile,
                    unsigned long dataCacheSize,
                    unsigned long indexCacheSize);

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              FileScanner::Mode modeIndex,
              bool memoryMapedIndex,
              FileScanner::Mode modeData,
              bool memoryMapedData);
    bool Close();

    bool GetOffsets(const std::set<I>& ids,
                    std::vector<FileOffset>& offsets) const;
    bool GetOffsets(const std::vector<I>& ids,
                    std::vector<FileOffset>& offsets) const;
    bool GetOffset(const I& id,
                   FileOffset& offset) const;

    bool Get(const std::vector<I>& ids,
             std::vector<ValueType>& data) const;
    bool Get(const std::list<I>& ids,
             std::vector<ValueType>& data) const;
    bool Get(const std::set<I>& ids,
             std::vector<ValueType>& data) const;

    bool Get(const I& id,
             ValueType& entry) const;

    void DumpStatistics() const;
  };

  template <class I, class N>
  IndexedDataFile<I,N>::IndexedDataFile(const std::string& datafile,
                                        const std::string& indexfile,
                                        unsigned long dataCacheSize,
                                        unsigned long indexCacheSize)
  : DataFile<N>(datafile,dataCacheSize),
    index(indexfile,indexCacheSize)
  {
    // no code
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Open(const TypeConfigRef& typeConfig,
                                  const std::string& path,
                                  FileScanner::Mode modeIndex,
                                  bool memoryMapedIndex,
                                  FileScanner::Mode modeData,
                                  bool memoryMapedData)
  {
    if (!DataFile<N>::Open(typeConfig,
                               path,
                               modeData,
                               memoryMapedData)) {
      return false;
    }

    return index.Open(path,
                      modeIndex,
                      memoryMapedIndex);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Close()
  {
    bool success=DataFile<N>::Close();

    if (!index.Close()) {
      success=false;
    }

    return success;
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::GetOffsets(const std::set<I>& ids,
                                        std::vector<FileOffset>& offsets) const
  {
    return index.GetOffsets(ids,offsets);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::GetOffsets(const std::vector<I>& ids,
                                        std::vector<FileOffset>& offsets) const
  {
    return index.GetOffsets(ids,offsets);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::GetOffset(const I& id,
                                       FileOffset& offset) const
  {
    return index.GetOffset(id,offset);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::vector<I>& ids,
                                 std::vector<ValueType>& data) const
  {
    assert(DataFile<N>::isOpen);

    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids,offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets,data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::list<I>& ids,
                                 std::vector<ValueType>& data) const
  {
    assert(DataFile<N>::isOpen);

    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids,offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets,data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::set<I>& ids,
                                 std::vector<ValueType>& data) const
  {
    assert(DataFile<N>::isOpen);

    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids,offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets,data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const I& id,
                                 ValueType& entry) const
  {
    assert(DataFile<N>::isOpen);

    FileOffset offset;

    if (!index.GetOffset(id,offset)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offset,entry);
  }

  template <class I, class N>
  void IndexedDataFile<I,N>::DumpStatistics() const
  {
    DataFile<N>::DumpStatistics();

    index.DumpStatistics();
  }
}

#endif
