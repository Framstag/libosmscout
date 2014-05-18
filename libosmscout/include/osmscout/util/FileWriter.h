#ifndef OSMSCOUT_FILEWRITER_H
#define OSMSCOUT_FILEWRITER_H

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
#include <vector>

#include <osmscout/CoreFeatures.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/Types.h>

namespace osmscout {

  /**
    \ingroup File

    FileScanner implements platform independent writing to data in files.
    It uses C standard library FILE internally and wraps it to offer
    a number of convenience methods.
    */
  class OSMSCOUT_API FileWriter
  {
  private:
    std::string filename;
    std::FILE   *file;
    bool        hasError;

  public:
    FileWriter();
    virtual ~FileWriter();

    bool Open(const std::string& filename);
    bool Close();
    inline bool IsOpen() const
    {
      return file!=NULL;
    }

    inline bool HasError() const
    {
      return file==NULL || hasError;
    }

    std::string GetFilename() const;

    bool GetPos(FileOffset &pos);
    bool SetPos(FileOffset pos);

    bool Write(const char* buffer, size_t bytes);

    bool Write(const std::string& value);

    bool Write(bool boolean);

    bool Write(int8_t number);
    bool Write(int16_t number);
    bool Write(int32_t number);
#if defined(OSMSCOUT_HAVE_INT64_T)
    bool Write(int64_t number);
#endif

    bool Write(uint8_t number);
    bool Write(uint16_t number);
    bool Write(uint32_t number);
#if defined(OSMSCOUT_HAVE_UINT64_T)
    bool Write(uint64_t number);
#endif

    bool WriteFileOffset(FileOffset offset);
    bool WriteFileOffset(FileOffset offset,
                         size_t bytes);

    bool WriteNumber(int16_t number);
    bool WriteNumber(int32_t number);
#if defined(OSMSCOUT_HAVE_INT64_T)
    bool WriteNumber(int64_t number);
#endif

    bool WriteNumber(uint16_t number);
    bool WriteNumber(uint32_t number);
#if defined(OSMSCOUT_HAVE_UINT64_T)
    bool WriteNumber(uint64_t number);
#endif

    bool WriteCoord(const GeoCoord& coord);
    bool WriteCoord(double lat, double lon);

    bool Write(const std::vector<GeoCoord>& nodes);

    bool Flush();
    bool FlushCurrentBlockWithZeros(size_t blockSize);
  };
}

#endif
