#ifndef OSMSCOUT_FILESCANNER_H
#define OSMSCOUT_FILESCANNER_H

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

#include <cstdio>
#include <string>
#include <tuple>
#include <vector>

#include <osmscout/CoreImportExport.h>

#include <osmscout/CoreFeatures.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/Point.h>
#include <osmscout/OSMScoutTypes.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/Exception.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Geometry.h>

#if defined(_WIN32)
  #include <windows.h>
  #undef max
  #undef min
#endif

namespace osmscout {

  /**
    \ingroup File

    FileScanner implements platform independent sequential
    scanning-like access to data in files. File access is buffered.

    FileScanner will use mmap in read-only mode if available (and will
    fall back to normal buffered IO if available but failing), resulting in
    mapping the complete file into the memory of the process (without
    allocating real memory) resulting in measurable speed increase because of
    exchanging buffered file access with in memory array access.
    */
  class OSMSCOUT_API FileScanner CLASS_FINAL
  {
  public:
    enum Mode
    {
      Sequential,
      FastRandom,
      LowMemRandom,
      Normal
    };

  private:
    std::string  filename;       //!< Filename
    std::FILE    *file;          //!< Internal low level file handle
    mutable bool hasError;       //!< Flag to signal errors in the stream

    // For mmap usage
    char         *mmap;          //!< Pointer to the file memory
    FileOffset   size;           //!< Size of the memory/file
    FileOffset   offset;         //!< Current offset into the file memory

    // For std::vector<GeoCoord> loading
    uint8_t      *byteBuffer;    //!< Temporary buffer for loading of std::vector<GeoCoord>
    size_t       byteBufferSize; //!< Size of the temporary byte buffer

    // For Windows mmap usage
#if defined(__WIN32__) || defined(WIN32)
    HANDLE       mmfHandle;
#endif

  private:
    void AssureByteBufferSize(size_t size);
    void FreeBuffer();

    /**
     * Reads bytes to internal temporary buffer
     * or just return pointer to memory mapped file.
     *
     * In case of internal buffer, data are valid until
     * next read. In case of memory mapped file, data
     * are valid until closing the reader and method don't
     * copy the memory - it should be fast.
     *
     * @param bytes
     * @return pointer to byteBuffer or memory mapped file
     */
    char* ReadInternal(size_t bytes);

    /**
     * Set coordinates using raw data from file.
     *
     * When NDEBUG macro is not defined,
     * check if GeoCoord is normalised,
     * throw IO exception otherwise
     *
     * @param latDat raw latitude data
     * @param lonDat raw longitude data
     * @return coord output
     */
    GeoCoord CreateCoord(const uint32_t &latDat,
                                const uint32_t &lonDat)
    {
#ifndef NDEBUG
      if (latDat > maxRawCoordValue ||
          lonDat > maxRawCoordValue){
        hasError=true;
        throw IOException(filename,"Cannot read coordinate","Coordinate is not normalised");
      }
#endif

      return {latDat/latConversionFactor-90.0,
              lonDat/lonConversionFactor-180.0};
    }

    /**
     * Set coordinates using raw data from file.
     *
     * When NDEBUG macro is not defined,
     * check if GeoCoord is normalised,
     * throw IO exception otherwise
     *
     * @param latDat raw latitude data
     * @param lonDat raw longitude data
     * @param coord output
     */
    void SetCoord(const uint32_t &latDat,
                         const uint32_t &lonDat,
                         Point &point)
    {
#ifndef NDEBUG
      if (latDat > maxRawCoordValue ||
          lonDat > maxRawCoordValue){
        hasError=true;
        throw IOException(filename,"Cannot read coordinate","Coordinate is not normalised");
      }
#endif

      point.SetCoord(GeoCoord(latDat/latConversionFactor-90.0,
                              lonDat/lonConversionFactor-180.0));
    }

    /**
     * Covert raw data to boolean
     *
     * When NDEBUG macro is not defined,
     * check if value is normalised,
     * throw IO exception otherwise
     *
     * @param value
     * @return boolean value
     */
    bool ConvertBool(const char &value)
    {
#ifndef NDEBUG
      if (value != 0 && value != 1){
        hasError=true;
        throw IOException(filename,"Cannot read bool","Bool value is not normalised");
      }
#endif
      return value!=0;
    }

  public:
    FileScanner();
    ~FileScanner();

    void Open(const std::string& filename,
              Mode mode,
              bool useMmap);
    void Close();
    void CloseFailsafe();

    bool IsOpen() const
    {
      return file!=nullptr;
    }

    bool IsEOF() const;

     bool HasError() const
    {
      return file==nullptr || hasError;
    }

    std::string GetFilename() const;

    void GotoBegin();
    void SetPos(FileOffset pos);
    FileOffset GetPos() const;

    void Read(char* buffer, size_t bytes);

    std::string ReadString();

    bool ReadBool();

    int8_t ReadInt8();
    int16_t ReadInt16();
    int32_t ReadInt32();
    int64_t ReadInt64();

    uint8_t ReadUInt8();
    uint16_t ReadUInt16();
    uint32_t ReadUInt32();
    uint64_t ReadUInt64();

    uint16_t Read(size_t bytes);
    uint32_t ReadUInt32(size_t bytes);
    uint64_t ReadUInt64(size_t bytes);

    ObjectFileRef ReadObjectFileRef();

    Color ReadColor();

    FileOffset ReadFileOffset();
    FileOffset ReadFileOffset(size_t bytes);

    int16_t ReadInt16Number();
    int32_t ReadInt32Number();
    int64_t ReadInt64Number();

    uint16_t ReadUInt16Number();
    uint32_t ReadUInt32Number();
    uint64_t ReadUInt64Number();

    GeoCoord ReadCoord();
    std::tuple<GeoCoord,bool> ReadConditionalCoord();

    /**
     * Reads vector of Point and pre-compute segments and bounding box for it
     *
     * @param nodes
     * @param segments
     * @param bbox
     * @param readIds
     */
    void Read(std::vector<Point>& nodes,
              std::vector<SegmentGeoBox> &segments,
              GeoBox &bbox,
              bool readIds);

    GeoBox ReadBox();

    TypeId ReadTypeId(uint8_t maxBytes);

    std::vector<ObjectFileRef> ReadObjectFileRefs(size_t count);
  };

  /**
   * Read back a stream of sorted ObjectFileRefs as written by the ObjectFileRefStreamWriter.
   */
  class OSMSCOUT_API ObjectFileRefStreamReader
  {
  private:
    FileScanner& reader;
    FileOffset   lastFileOffset;

  public:
    explicit ObjectFileRefStreamReader(FileScanner& reader);

    void Reset();

    void Read(ObjectFileRef& ref);
  };

}

#endif
