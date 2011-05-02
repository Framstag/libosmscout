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

#include <osmscout/TypeConfig.h>

#if defined(__WIN32__) || defined(WIN32)
  #include <windows.h>
  #undef max
  #undef min
#endif

namespace osmscout {
  /**
    FileScanner implements platform independend sequential
    scanning-like access to data in files. File access is buffered.

    FileScanner will use mmap in read-only mode if available (and will
    fall back to normal buffered IO if available but failing), resulting in
    mapping the complete file into the memory of the process (without
    allocating real memory) resulting in measurable speed increase because of
    exchanging buffered file access with in memory array access.
    */
  class OSMSCOUT_API FileScanner
  {
  private:
    std::string  filename;
    FILE         *file;
    mutable bool hasError;
    bool         readOnly;

    // For mmap usage
    char         *buffer;
    FileOffset   size;
    FileOffset   offset;

    // For Windows mmap usage
#if defined(__WIN32__) || defined(WIN32)
    HANDLE       mmfHandle;
#endif

  private:
    void FreeBuffer();

  public:
    FileScanner();
    virtual ~FileScanner();

    bool Open(const std::string& filename, bool readOnly=true);
    bool Close();

    bool IsOpen() const;
    bool IsEOF() const;
    bool HasError() const;

    std::string GetFilename() const;

    bool GotoBegin();
    bool SetPos(FileOffset pos);
    bool GetPos(FileOffset &pos) const;

    bool Read(char* buffer, size_t bytes);

    bool Read(std::string& value);
    bool Read(bool& boolean);
    bool Read(uint8_t& number);
    bool Read(uint16_t& number);
    bool Read(uint32_t& number);
    bool Read(int8_t& number);
    bool Read(int32_t& number);

    bool ReadNumber(uint8_t& number);
    bool ReadNumber(uint16_t& number);
    bool ReadNumber(uint32_t& number);
    bool ReadNumber(int32_t& number);
  };
}

#endif
