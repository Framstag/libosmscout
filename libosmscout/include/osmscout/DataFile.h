#ifndef OSMSCOUT_DATAFILE_H
#define OSMSCOUT_DATAFILE_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at youbase option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
*/

#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <vector>

#include <osmscout/NumericIndex.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Logger.h>

namespace osmscout {

  /**
   * \ingroup Database
   * Reference a range of data entries by giving the offset of the first entry in the file
   * and the number of data elements.
   */
  struct DataBlockSpan
  {
    FileOffset startOffset; //!< Offset for the first data entry referenced in the file. Data will be read starting from this position
    uint32_t   count;       //!< Number of entries to read.

    inline bool operator<(const DataBlockSpan& other) const
    {
      return startOffset<other.startOffset;
    }

    inline bool operator==(const DataBlockSpan& other) const
    {
      return startOffset==other.startOffset && count==other.count;
    }

    inline bool operator!=(const DataBlockSpan& other) const
    {
      return startOffset!=other.startOffset || count!=other.count;
    }
  };

  /**
   * \ingroup Database
   *
   * Access to standard format data files.
   *
   * Allows to load data objects by offset using various standard library data structures.
   */
  template <class N>
  class DataFile
  {
  public:
    typedef std::shared_ptr<N> ValueType;
    typedef Cache<FileOffset,std::shared_ptr<N>> ValueCache;

    typedef typename Cache<FileOffset,ValueType>::CacheEntry ValueCacheEntry;
    typedef typename Cache<FileOffset,ValueType>::CacheRef ValueCacheRef;

  private:
    std::string         datafile;        //!< Basename part of the data file name
    std::string         datafilename;    //!< complete filename for data file

    mutable ValueCache  cache;

    mutable FileScanner scanner;         //!< File stream to the data file

    mutable std::mutex  accessMutex;     //!< Mutex to secure multi-thread access

  protected:
    TypeConfigRef       typeConfig;

  private:
    bool ReadData(const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  N& data) const;
    bool ReadData(const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  FileOffset offset,
                  N& data) const;

  public:
    DataFile(const std::string& datafile, size_t cacheSize);

    virtual ~DataFile();

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMapedData);
    virtual bool IsOpen() const;
    virtual bool Close();

    bool GetByOffset(const FileOffset& offset,
                     ValueType& entry) const;

    bool GetByBlockSpan(const DataBlockSpan& span,
                        std::vector<ValueType>& data) const;

    template<typename IteratorIn>
    bool GetByOffset(IteratorIn begin, IteratorIn end, size_t size,
                     std::vector<ValueType>& data) const;

    template<typename IteratorIn>
    bool GetByOffset(IteratorIn begin, IteratorIn end, size_t size,
                     const GeoBox& boundingBox,
                     std::vector<ValueType>& data) const;

    template<typename IteratorIn>
    bool GetByOffset(IteratorIn begin, IteratorIn end, size_t size,
                     std::unordered_map<FileOffset,ValueType>& dataMap) const;

    template<typename IteratorIn>
    bool GetByBlockSpans(IteratorIn begin, IteratorIn end,
                         std::vector<ValueType>& data) const;
  };

  template <class N>
  DataFile<N>::DataFile(const std::string& datafile, size_t cacheSize)
  : datafile(datafile),cache(cacheSize)
  {
    // no code
  }

  template <class N>
  DataFile<N>::~DataFile()
  {
    if (IsOpen()) {
      Close();
    }
  }

  /**
   * Read one data value from the given file offset.
   *
   * Method is NOT thread-safe.
   */
  template <class N>
  bool DataFile<N>::ReadData(const TypeConfig& typeConfig,
                             FileScanner& scanner,
                             FileOffset offset,
                             N& data) const
  {
    try {
      scanner.SetPos(offset);

      data.Read(typeConfig,
                scanner);
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  /**
   * Read one data value from the current position of the stream
   *
   * Method is NOT thread-safe.
   */
  template <class N>
  bool DataFile<N>::ReadData(const TypeConfig& typeConfig,
                             FileScanner& scanner,
                             N& data) const
  {
    try {
      data.Read(typeConfig,
                scanner);
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
  }

  /**
   * Open the index file.
   *
   * Method is NOT thread-safe.
   */
  template <class N>
  bool DataFile<N>::Open(const TypeConfigRef& typeConfig,
                         const std::string& path,
                         bool memoryMapedData)
  {
    this->typeConfig=typeConfig;

    datafilename=AppendFileToDir(path,datafile);

    try {
      scanner.Open(datafilename,
                   FileScanner::LowMemRandom,
                   memoryMapedData);
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  /**
   * Return true, if index is currently opened.
   *
   * Method is NOT thread-safe.
   */
  template <class N>
  bool DataFile<N>::IsOpen() const
  {
    return scanner.IsOpen();
  }

  /**
   * Close the index.
   *
   * Method is NOT thread-safe.
   */
  template <class N>
  bool DataFile<N>::Close()
  {
    typeConfig=NULL;

    try  {
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

  /**
   * Reads data for the given file offsets. File offsets are passed by iterator over
   * some container. the size parameter hints as the number of entries returned by the iterators
   * and is used to preallocate enough room in the result vector.
   *
   * @tparam N
   *    Object type managed by the data file
   * @tparam IteratorIn
   *    Iterator over a colection
   * @param begin
   *    Start iterator for the file offset
   * @param end
   *    End iterator for the file offset
   * @param size
   *    Number of entries returnd by the begin, end itertaor pair. USed for preallocating enough space
   *    in result vector.
   * @param data
   *    vector containing data. Data is appended.
   * @return
   *    false if there was an error, else true
   *
   * Method is thread-safe.
   */
  template <class N>
  template <typename IteratorIn>
  bool DataFile<N>::GetByOffset(IteratorIn begin, IteratorIn end, size_t size,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+size);
    std::lock_guard<std::mutex> lock(accessMutex);

    for (IteratorIn offsetIter=begin; offsetIter!=end; ++offsetIter) {
      ValueCacheRef entryRef;

      if (cache.GetEntry(*offsetIter,entryRef)) {
        data.push_back(entryRef->value);
      }
      else {
        ValueType value=std::make_shared<N>();

        if (!ReadData(*typeConfig,
                      scanner,
                      *offsetIter,
                      *value)) {
          log.Error() << "Error while reading data from offset " << *offsetIter << " of file " << datafilename << "!";
          return false;
        }

        cache.SetEntry(ValueCacheEntry(*offsetIter,value));
        data.push_back(value);
      }
    }

    return true;
  }

  /**
   * Read data values from the given file offsets.
   *
   * Method is thread-safe.
   */
  template <class N>
  template<typename IteratorIn>
  bool DataFile<N>::GetByOffset(IteratorIn begin, IteratorIn end, size_t size,
                                const GeoBox& boundingBox,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+size);
    std::lock_guard<std::mutex> lock(accessMutex);

    for (IteratorIn offsetIter=begin; offsetIter!=end; ++offsetIter) {
      ValueType value=std::make_shared<N>();

      ValueCacheRef entryRef;
      if (cache.GetEntry(*offsetIter,entryRef)){
        value=entryRef->value;
      }else{
        if (!ReadData(*typeConfig,
                      scanner,
                      *offsetIter,
                      *value)) {
          log.Error() << "Error while reading data from offset " << *offsetIter << " of file " << datafilename << "!";
          return false;
        }

        cache.SetEntry(ValueCacheEntry(*offsetIter,value));
      }

      if (!value->Intersects(boundingBox)) {
        continue;
      }

      data.push_back(value);
    }

    return true;
  }

  /**
   * Read data values from the given file offsets.
   *
   * Method is thread-safe.
   */
  template <class N>
  template<typename IteratorIn>
  bool DataFile<N>::GetByOffset(IteratorIn begin, IteratorIn end, size_t size,
                                std::unordered_map<FileOffset,ValueType>& dataMap) const
  {
    std::vector<ValueType> data;

    if (!GetByOffset(begin,
                     end,
                     size,
                     data)) {
      return false;
    }

    for (const auto entry : data) {
      dataMap.insert(std::make_pair(entry->GetFileOffset(),entry));
    }

    return true;
  }

  /**
   * Read one data value from the given file offset.
   *
   * Method is thread-safe.
   */
  template <class N>
  bool DataFile<N>::GetByOffset(const FileOffset& offset,
                                ValueType& entry) const
  {
    std::lock_guard<std::mutex> lock(accessMutex);

    ValueCacheRef entryRef;
    if (cache.GetEntry(offset,entryRef)){
      entry=entryRef->value;
    }else{
      ValueType value=std::make_shared<N>();

      if (!ReadData(*typeConfig,
                    scanner,
                    offset,
                    *value)) {
        log.Error() << "Error while reading data from offset " << offset << " of file " << datafilename << "!";
        // TODO: Remove broken entry from cache
        return false;
      }

      cache.SetEntry(ValueCacheEntry(offset,value));
      entry=value;
    }

    return true;
  }

  /**
   * Read data values from the given DataBlockSpan.
   *
   * Method is thread-safe.
   */
  template <class N>
  bool DataFile<N>::GetByBlockSpan(const DataBlockSpan& span,
                                   std::vector<ValueType>& data) const
  {
    if (span.count==0) {
      return true;
    }

    std::lock_guard<std::mutex> lock(accessMutex);

    try {
      bool offsetSetup=false;
      FileOffset offset=span.startOffset;

      data.reserve(data.size()+span.count);

      for (uint32_t i=1; i<=span.count; i++) {
        ValueCacheRef entryRef;
        if (cache.GetEntry(offset,entryRef)){
          data.push_back(entryRef->value);
          offset=entryRef->value->GetNextFileOffset();
          offsetSetup=false;
        }else{
          if (!offsetSetup){
            scanner.SetPos(offset);
          }

          ValueType value=std::make_shared<N>();

          if (!ReadData(*typeConfig,
                        scanner,
                        *value)) {
            log.Error() << "Error while reading data #" << i << " starting from offset " << span.startOffset << " of file " << datafilename << "!";
            return false;
          }

          cache.SetEntry(ValueCacheEntry(offset,value));
          offset=value->GetNextFileOffset();
          offsetSetup=true;
          data.push_back(value);
        }
      }

      return true;
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }
  }

  /**
   * Read data values from the given DataBlockSpans.
   *
   * Method is thread-safe.
   */
  template <class N>
  template<typename IteratorIn>
  bool DataFile<N>::GetByBlockSpans(IteratorIn begin, IteratorIn end,
                                    std::vector<ValueType>& data) const
  {
    uint32_t overallCount=0;

    for (IteratorIn spanIter=begin; spanIter!=end; ++spanIter) {
      overallCount+=spanIter->count;
    }

    data.reserve(data.size()+overallCount);

    try {
      std::lock_guard<std::mutex> lock(accessMutex);
      for (IteratorIn spanIter=begin; spanIter!=end; ++spanIter) {
        if (spanIter->count==0) {
          continue;
        }

        bool offsetSetup=false;
        FileOffset offset=spanIter->startOffset;

        for (uint32_t i=1; i<=spanIter->count; i++) {
          ValueCacheRef entryRef;
          if (cache.GetEntry(offset,entryRef)){
            data.push_back(entryRef->value);
            offset=entryRef->value->GetNextFileOffset();
            offsetSetup=false;
          }else{
            if (!offsetSetup){
              scanner.SetPos(offset);
            }

            ValueType value=std::make_shared<N>();

            if (!ReadData(*typeConfig,
                          scanner,
                          *value)) {
              log.Error() << "Error while reading data #" << i << " starting from offset " << spanIter->startOffset <<
              " of file " << datafilename << "!";
              return false;
            }

            cache.SetEntry(ValueCacheEntry(offset,value));
            offset=value->GetNextFileOffset();
            offsetSetup=true;
            data.push_back(value);
          }
        }
      }
    }
    catch (IOException& e) {
      log.Error() << e.GetDescription();
      return false;
    }

    return true;
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
    typedef std::shared_ptr<N> ValueType;

  private:
    typedef NumericIndex<I> DataIndex;

  private:
    DataIndex     index;
    TypeConfigRef typeConfig;

  public:
    IndexedDataFile(const std::string& datafile,
                    const std::string& indexfile,
                    unsigned long indexCacheSize,
                    unsigned long dataCacheSize);

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMapedIndex,
              bool memoryMapedData);
    bool Close();

    bool IsOpen() const;

    bool GetOffset(const I& id,
                   FileOffset& offset) const;

    bool Get(const I& id,
             ValueType& entry) const;

    template<typename IteratorIn>
    bool GetOffsets(IteratorIn begin, IteratorIn end, size_t size,
                    std::vector<FileOffset>& offsets) const;

    bool Get(const std::vector<I>& ids,
             std::vector<ValueType>& data) const;
    bool Get(const std::list<I>& ids,
             std::vector<ValueType>& data) const;
    bool Get(const std::set<I>& ids,
             std::vector<ValueType>& data) const;

    bool Get(const std::set<I>& ids,
             std::unordered_map<I,ValueType>& data) const;

  };

  template <class I, class N>
  IndexedDataFile<I,N>::IndexedDataFile(const std::string& datafile,
                                        const std::string& indexfile,
                                        unsigned long indexCacheSize,
                                        unsigned long dataCacheSize)
  : DataFile<N>(datafile,dataCacheSize),
    index(indexfile,indexCacheSize)
  {
    // no code
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Open(const TypeConfigRef& typeConfig,
                                  const std::string& path,
                                  bool memoryMapedIndex,
                                  bool memoryMapedData)
  {
    if (!DataFile<N>::Open(typeConfig,
                           path,
                           memoryMapedData)) {
      return false;
    }

    return index.Open(path,
                      memoryMapedIndex);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Close()
  {
    bool result=true;

    if (!DataFile<N>::Close()) {
      result=false;
    }

    if (!index.Close()) {
      result=false;
    }

    return result;
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::IsOpen() const
  {
    return DataFile<N>::IsOpen() &&
           index.IsOpen();
  }

  template <class I, class N>
  template<typename IteratorIn>
  bool IndexedDataFile<I,N>::GetOffsets(IteratorIn begin, IteratorIn end, size_t size,
                                        std::vector<FileOffset>& offsets) const
  {
    return index.GetOffsets(begin,
                           end,
                           size,
                           offsets);
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
    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids.begin(),
                          ids.end(),
                          ids.size(),
                          offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets.begin(),
                                    offsets.end(),
                                    offsets.size(),
                                    data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::list<I>& ids,
                                 std::vector<ValueType>& data) const
  {
    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids.begin(),
                          ids.end(),
                          ids.size(),
                          offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets.begin(),
                                    offsets.end(),
                                    offsets.size(),
                                    data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::set<I>& ids,
                                 std::vector<ValueType>& data) const
  {
    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids.begin(),
                          ids.end(),
                          ids.size(),
                          offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets.begin(),
                                    offsets.end(),
                                    offsets.size(),
                                    data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::set<I>& ids,
                                 std::unordered_map<I,ValueType>& data) const
  {
    std::vector<FileOffset> offsets;
    std::vector<ValueType>  d;

    if (!index.GetOffsets(ids.begin(),
                          ids.end(),
                          ids.size(),
                          offsets)) {
      return false;
    }

    if (!DataFile<N>::GetByOffset(offsets.begin(),
                                  offsets.end(),
                                  offsets.size(),
                                  d)) {
      return false;
    }

    for (const auto& value : d) {
      data[value->GetId()]=value;
    }

    return true;
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const I& id,
                                 ValueType& entry) const
  {
    FileOffset offset;

    if (!index.GetOffset(id,offset)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offset,entry);
  }
}

#endif
