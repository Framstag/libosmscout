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
#include <osmscout/TypeConfig.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>
#include <osmscout/util/Logger.h>

//#include <map>
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

    bool operator<(const DataBlockSpan& other) const
    {
      return startOffset<other.startOffset;
    }

    bool operator==(const DataBlockSpan& other) const
    {
      return startOffset==other.startOffset && count==other.count;
    }

    bool operator!=(const DataBlockSpan& other) const
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
    using ValueType = std::shared_ptr<N>;
    using ValueCache = Cache<FileOffset, std::shared_ptr<N> >;

    using ValueCacheEntry = typename Cache<FileOffset, ValueType>::CacheEntry;
    using ValueCacheRef = typename Cache<FileOffset, ValueType>::CacheRef;

  private:
    std::string         datafile;        //!< Basename part of the data file name
    std::string         datafilename;    //!< complete filename for data file

    mutable ValueCache  cache;

    mutable FileScanner scanner;         //!< File stream to the data file

    mutable std::mutex  accessMutex;     //!< Mutex to secure multi-thread access

  protected:
    TypeConfigRef       typeConfig;

  private:
    bool ReadData(N& data) const;
    bool ReadData(FileOffset offset,
                  N& data) const;

  public:
    DataFile(const std::string& datafile,
             size_t cacheSize);

    // disable copy and move
    DataFile(const DataFile&) = delete;
    DataFile(DataFile&&) = delete;
    DataFile& operator=(const DataFile&) = delete;
    DataFile& operator=(DataFile&&) = delete;

    virtual ~DataFile();

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMappedData);
    virtual bool IsOpen() const;
    virtual bool Close();

    void FlushCache();

    std::string GetFilename() const
    {
      return datafilename;
    }

    bool GetByOffset(FileOffset offset,
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
  bool DataFile<N>::ReadData(FileOffset offset,
                             N& data) const
  {
    try {
      scanner.SetPos(offset);

      data.Read(*typeConfig,
                scanner);
    }
    catch (const IOException& e) {
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
  bool DataFile<N>::ReadData(N& data) const
  {
    try {
      data.Read(*typeConfig,
                scanner);
    }
    catch (const IOException& e) {
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
                         bool memoryMappedData)
  {
    this->typeConfig=typeConfig;

    datafilename=AppendFileToDir(path,datafile);

    try {
      scanner.Open(datafilename,
                   FileScanner::LowMemRandom,
                   memoryMappedData);
    }
    catch (const IOException& e) {
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
    typeConfig=nullptr;
    cache.Flush();

    try  {
      if (scanner.IsOpen()) {
        scanner.Close();
      }
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
      scanner.CloseFailsafe();
      return false;
    }

    return true;
  }

  template <class N>
  void DataFile<N>::FlushCache()
  {
    std::scoped_lock<std::mutex> lock(accessMutex);
    cache.Flush();
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
  bool DataFile<N>::GetByOffset(IteratorIn begin, IteratorIn end,
                                size_t size,
                                std::vector<ValueType>& data) const
  {
    if (size==0) {
      return true;
    }

    data.reserve(data.size()+size);
    std::scoped_lock<std::mutex> lock(accessMutex);

    if (cache.GetMaxSize()>0 &&
        size>cache.GetMaxSize()){
      log.Warn() << "Cache size (" << cache.GetMaxSize() << ") for file " << datafile << " is smaller than current request (" << size << ")";
    }

    for (IteratorIn offsetIter=begin; offsetIter!=end; ++offsetIter) {
      ValueCacheRef entryRef;

      if (cache.GetEntry(*offsetIter,entryRef)) {
        data.push_back(entryRef->value);
      }
      else {
        ValueType value=std::make_shared<N>();

        if (!ReadData(*offsetIter,
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
  bool DataFile<N>::GetByOffset(IteratorIn begin, IteratorIn end,
                                size_t size,
                                const GeoBox& boundingBox,
                                std::vector<ValueType>& data) const
  {
    if (size==0) {
      return true;
    }

    data.reserve(data.size()+size);
    std::scoped_lock<std::mutex> lock(accessMutex);

    if (cache.GetMaxSize()>0 &&
        size>cache.GetMaxSize()){
      log.Warn() << "Cache size (" << cache.GetMaxSize() << ") for file " << datafile << " is smaller than current request (" << size << ")";
    }

    //std::map<std::string,size_t> hitRateTypes;
    //std::map<std::string,size_t> missRateTypes;
    size_t inBoxCount=0;
    for (IteratorIn offsetIter=begin; offsetIter!=end; ++offsetIter) {
      ValueType value=std::make_shared<N>();

      ValueCacheRef entryRef;
      if (cache.GetEntry(*offsetIter,entryRef)){
        value=entryRef->value;
      }else{
        if (!ReadData(*offsetIter,
                      *value)) {
          log.Error() << "Error while reading data from offset " << *offsetIter << " of file " << datafilename << "!";
          return false;
        }

        cache.SetEntry(ValueCacheEntry(*offsetIter,value));
      }

      if (!value->Intersects(boundingBox)) {
        //missRateTypes[value->GetType()->GetName()]++;
        continue;
      }
      /*else {
        hitRateTypes[value->GetType()->GetName()]++;
      }*/

      inBoxCount++;

      data.push_back(value);
    }

    size_t hitRate=inBoxCount*100/size;
    if (size>100 && hitRate<50) {
      log.Warn() << "Bounding box hit rate for file " << datafile << " is only " << hitRate << "% (" << inBoxCount << "/" << size << ")";
      /*
      for (const auto& missRateType: missRateTypes) {
        log.Warn() << "- " << missRateType.first << " " << missRateType.second;
      }
      for (const auto& hitRateType: hitRateTypes) {
        log.Warn() << "+ " << hitRateType.first << " " << hitRateType.second;
      }*/
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
  bool DataFile<N>::GetByOffset(IteratorIn begin, IteratorIn end,
                                size_t size,
                                std::unordered_map<FileOffset,ValueType>& dataMap) const
  {
    if (size==0) {
      return true;
    }

    std::vector<ValueType> data;

    if (!GetByOffset(begin,
                     end,
                     size,
                     data)) {
      return false;
    }

    for (const auto& entry : data) {
      dataMap.emplace(entry->GetFileOffset(),entry);
    }

    return true;
  }

  /**
   * Read one data value from the given file offset.
   *
   * Method is thread-safe.
   */
  template <class N>
  bool DataFile<N>::GetByOffset(FileOffset offset,
                                ValueType& entry) const
  {
    std::scoped_lock<std::mutex> lock(accessMutex);

    ValueCacheRef entryRef;
    if (cache.GetEntry(offset,entryRef)){
      entry=entryRef->value;
    }else{
      ValueType value=std::make_shared<N>();

      if (!ReadData(offset,
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

    std::scoped_lock<std::mutex> lock(accessMutex);

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

          if (!ReadData(*value)) {
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
    catch (const IOException& e) {
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
      std::scoped_lock<std::mutex> lock(accessMutex);
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

            if (!ReadData(*value)) {
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
    catch (const IOException& e) {
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
    using ValueType = std::shared_ptr<N>;

  private:
    using DataIndex = NumericIndex<I>;

  private:
    DataIndex index;

  public:
    IndexedDataFile(const std::string& datafile,
                    const std::string& indexfile,
                    size_t indexCacheSize,
                    size_t dataCacheSize);

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMappedIndex,
              bool memoryMappedData);
    bool Close() override;

    bool IsOpen() const override;

    bool GetOffset(I id,
                   FileOffset& offset) const;

    bool Get(I id,
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
                                        size_t indexCacheSize,
                                        size_t dataCacheSize)
  : DataFile<N>(datafile,dataCacheSize),
    index(indexfile,indexCacheSize)
  {
    // no code
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Open(const TypeConfigRef& typeConfig,
                                  const std::string& path,
                                  bool memoryMappedIndex,
                                  bool memoryMappedData)
  {
    if (!DataFile<N>::Open(typeConfig,
                           path,
                           memoryMappedData)) {
      return false;
    }

    return index.Open(path,
                      memoryMappedIndex);
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
  bool IndexedDataFile<I,N>::GetOffset(I id,
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
  bool IndexedDataFile<I,N>::Get(I id,
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
