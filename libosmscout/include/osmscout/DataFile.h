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

  private:
    std::string         datafile;        //!< Basename part of the data file name
    std::string         datafilename;    //!< complete filename for data file

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
    DataFile(const std::string& datafile);

    virtual ~DataFile();

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMapedData);
    virtual bool IsOpen() const;
    virtual bool Close();

    bool GetByOffset(const std::vector<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::vector<FileOffset>& offsets,
                     const GeoBox& boundingBox,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::list<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;
    bool GetByOffset(const std::set<FileOffset>& offsets,
                     std::vector<ValueType>& data) const;

    bool GetByOffset(const std::set<FileOffset>& offsets,
                     std::unordered_map<FileOffset,ValueType>& dataMap) const;

    bool GetByOffset(const FileOffset& offset,
                     ValueType& entry) const;

    bool GetByBlockSpan(const DataBlockSpan& span,
                        std::vector<ValueType>& data) const;
    bool GetByBlockSpans(const std::vector<DataBlockSpan>& spans,
                         std::vector<ValueType>& data) const;
  };

  template <class N>
  DataFile<N>::DataFile(const std::string& datafile)
  : datafile(datafile)
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
   * Method is thread-safe.
   */
  template <class N>
  bool DataFile<N>::ReadData(const TypeConfig& typeConfig,
                             FileScanner& scanner,
                             FileOffset offset,
                             N& data) const
  {
    std::lock_guard<std::mutex> lock(accessMutex);

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
   * Method is not thread-safe.
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
   * Method is not thread-safe.
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
   * Method is not thread-safe.
   */
  template <class N>
  bool DataFile<N>::IsOpen() const
  {
    return scanner.IsOpen();
  }

  /**
   * Close the index.
   *
   * Method is not thread-safe.
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
   * Read data values from the given file offsets.
   *
   * Method is thread-safe.
   */
  template <class N>
  bool DataFile<N>::GetByOffset(const std::vector<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      if (!ReadData(*typeConfig,
                    scanner,
                    offset,
                    *value)) {
        log.Error() << "Error while reading data from offset " << offset << " of file " << datafilename << "!";
        return false;
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
  bool DataFile<N>::GetByOffset(const std::vector<FileOffset>& offsets,
                                const GeoBox& boundingBox,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      if (!ReadData(*typeConfig,
                    scanner,
                    offset,
                    *value)) {
        log.Error() << "Error while reading data from offset " << offset << " of file " << datafilename << "!";
        return false;
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
  bool DataFile<N>::GetByOffset(const std::list<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      if (!ReadData(*typeConfig,
                    scanner,
                    offset,
                    *value)) {
        log.Error() << "Error while reading data from offset " << offset << " of file " << datafilename << "!";
        // TODO: Remove broken entry from cache
        return false;
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
  bool DataFile<N>::GetByOffset(const std::set<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      if (!ReadData(*typeConfig,
                    scanner,
                    offset,
                    *value)) {
        log.Error() << "Error while reading data from offset " << offset << " of file " << datafilename << "!";
        // TODO: Remove broken entry from cache
        return false;
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
  bool DataFile<N>::GetByOffset(const std::set<FileOffset>& offsets,
                                std::unordered_map<FileOffset,ValueType>& dataMap) const
  {
    std::vector<ValueType> data;

    if (!GetByOffset(offsets,data)) {
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
    ValueType value=std::make_shared<N>();

    if (!ReadData(*typeConfig,
                  scanner,
                  offset,
                  *value)) {
      log.Error() << "Error while reading data from offset " << offset << " of file " << datafilename << "!";
      // TODO: Remove broken entry from cache
      return false;
    }

    entry=value;

    return true;
  }

  /**
   * Read data values from the given DataBlockSpan.
   *
   * Method is thread-safe.
   */
  template <class N>
  bool DataFile<N>::GetByBlockSpan(const DataBlockSpan& span,
                                   std::vector<ValueType>& area) const
  {
    if (span.count==0) {
      return true;
    }

    std::lock_guard<std::mutex> lock(accessMutex);

    try {
      scanner.SetPos(span.startOffset);

      area.reserve(area.size()+span.count);

      for (uint32_t i=1; i<=span.count; i++) {
        ValueType value=std::make_shared<N>();

        if (!ReadData(*typeConfig,
                      scanner,
                      *value)) {
          log.Error() << "Error while reading data #" << i << " starting from offset " << span.startOffset << " of file " << datafilename << "!";
          return false;
        }

        area.push_back(value);
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
  bool DataFile<N>::GetByBlockSpans(const std::vector<DataBlockSpan>& spans,
                                    std::vector<ValueType>& data) const
  {
    uint32_t overallCount=0;

    for (const auto& span : spans) {
      overallCount+=span.count;
    }

    data.reserve(data.size()+overallCount);

    try {
      for (const auto& span : spans) {
        if (span.count==0) {
          continue;
        }

        std::lock_guard<std::mutex> lock(accessMutex);

        scanner.SetPos(span.startOffset);

        for (uint32_t i=1; i<=span.count; i++) {
          ValueType value=std::make_shared<N>();

          if (!ReadData(*typeConfig,
                        scanner,
                        *value)) {
            log.Error() << "Error while reading data #" << i << " starting from offset " << span.startOffset <<
            " of file " << datafilename << "!";
            return false;
          }

          data.push_back(value);
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
                    unsigned long indexCacheSize);

    bool Open(const TypeConfigRef& typeConfig,
              const std::string& path,
              bool memoryMapedIndex,
              bool memoryMapedData);
    bool Close();

    bool IsOpen() const;

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
    bool Get(const std::set<I>& ids,
             std::unordered_map<I,ValueType>& data) const;

    bool Get(const I& id,
             ValueType& entry) const;
  };

  template <class I, class N>
  IndexedDataFile<I,N>::IndexedDataFile(const std::string& datafile,
                                        const std::string& indexfile,
                                        unsigned long indexCacheSize)
  : DataFile<N>(datafile),
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
    std::vector<FileOffset> offsets;

    if (!index.GetOffsets(ids,offsets)) {
      return false;
    }

    return DataFile<N>::GetByOffset(offsets,data);
  }

  template <class I, class N>
  bool IndexedDataFile<I,N>::Get(const std::set<I>& ids,
                                 std::unordered_map<I,ValueType>& data) const
  {
    std::vector<FileOffset> offsets;
    std::vector<ValueType>  d;

    if (!index.GetOffsets(ids,offsets)) {
      return false;
    }

    if (!DataFile<N>::GetByOffset(offsets,d)) {
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
