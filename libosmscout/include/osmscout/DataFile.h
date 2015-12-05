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

#include <memory>
#include <set>
#include <unordered_map>
#include <vector>

#include <osmscout/NumericIndex.h>

#include <osmscout/util/Cache.h>
#include <osmscout/util/FileScanner.h>

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
    FileScanner::Mode   modeData;        //!< Type of file access
    bool                memoryMapedData; //!< Use memory mapped files for data access
    mutable FileScanner scanner;         //!< File stream to the data file

  protected:
    TypeConfigRef       typeConfig;

  private:
    bool ReadData(const TypeConfig& typeConfig,
                  FileScanner& scanner,
                  N& data) const;

  public:
    DataFile(const std::string& datafile);

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
  : datafile(datafile),
    modeData(FileScanner::LowMemRandom),
    memoryMapedData(false)
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

    return scanner.Open(datafilename,modeData,memoryMapedData);
  }

  template <class N>
  bool DataFile<N>::IsOpen() const
  {
    return scanner.IsOpen();
  }

  template <class N>
  bool DataFile<N>::Close()
  {
    typeConfig=NULL;

    if (scanner.IsOpen()) {
      return scanner.Close();
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::vector<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      scanner.SetPos(offset);

      if (!ReadData(*typeConfig,
                    scanner,
                    *value)) {
        std::cerr << "Error while reading data from offset " << offset << " of file " << datafilename << "!" << std::endl;
        scanner.Close();
        return false;
      }

      data.push_back(value);
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::list<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      scanner.SetPos(offset);

      if (!ReadData(*typeConfig,
                    scanner,
                    *value)) {
        std::cerr << "Error while reading data from offset " << offset << " of file " << datafilename << "!" << std::endl;
        // TODO: Remove broken entry from cache
        scanner.Close();
        return false;
      }

      data.push_back(value);
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByOffset(const std::set<FileOffset>& offsets,
                                std::vector<ValueType>& data) const
  {
    data.reserve(data.size()+offsets.size());

    for (const auto& offset : offsets) {
      ValueType value=std::make_shared<N>();

      scanner.SetPos(offset);

      if (!ReadData(*typeConfig,
                    scanner,
                    *value)) {
        std::cerr << "Error while reading data from offset " << offset << " of file " << datafilename << "!" << std::endl;
        // TODO: Remove broken entry from cache
        scanner.Close();
        return false;
      }

      data.push_back(value);
    }

    return true;
  }

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

  template <class N>
  bool DataFile<N>::GetByOffset(const FileOffset& offset,
                                ValueType& entry) const
  {
    ValueType value=std::make_shared<N>();

    scanner.SetPos(offset);

    if (!ReadData(*typeConfig,
                  scanner,
                  *value)) {
      std::cerr << "Error while reading data from offset " << offset << " of file " << datafilename << "!" << std::endl;
      // TODO: Remove broken entry from cache
      scanner.Close();
      return false;
    }

    entry=value;

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByBlockSpan(const DataBlockSpan& span,
                                   std::vector<ValueType>& area) const
  {
    if (span.count==0) {
      return true;
    }

    if (!scanner.SetPos(span.startOffset)) {
      std::cerr << "Error while navigating to offset " << span.startOffset << " of file " << datafilename << "!" << std::endl;
      scanner.Close();
      return false;
    }

    area.reserve(area.size()+span.count);

    for (uint32_t i=1; i<=span.count; i++) {
      ValueType value=std::make_shared<N>();

      if (!ReadData(*typeConfig,
                    scanner,
                    *value)) {
        std::cerr << "Error while reading data #" << i << " starting from offset " << span.startOffset << " of file " << datafilename << "!" << std::endl;
        scanner.Close();
        return false;
      }

      area.push_back(value);
    }

    return true;
  }

  template <class N>
  bool DataFile<N>::GetByBlockSpans(const std::vector<DataBlockSpan>& spans,
                                    std::vector<ValueType>& data) const
  {
    uint32_t overallCount=0;

    for (const auto& span : spans) {
      overallCount+=span.count;
    }

    data.reserve(data.size()+overallCount);

    for (const auto& span : spans) {
      if (span.count==0) {
        continue;
      }

      if (!scanner.SetPos(span.startOffset)) {
        std::cerr << "Error while navigating to offset " << span.startOffset << " of file " << datafilename << "!" <<
        std::endl;
        scanner.Close();
        return false;
      }

      for (uint32_t i=1; i<=span.count; i++) {
        ValueType value=std::make_shared<N>();

        if (!ReadData(*typeConfig,
                      scanner,
                      *value)) {
          std::cerr << "Error while reading data #" << i << " starting from offset " << span.startOffset <<
          " of file " << datafilename << "!" << std::endl;
          scanner.Close();
          return false;
        }

        data.push_back(value);
      }
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
              FileScanner::Mode modeIndex,
              bool memoryMapedIndex,
              FileScanner::Mode modeData,
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
    bool result=true;

    if (!DataFile<N>::Close()) {
      result=false;
    }

    if (index.Close()) {
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
