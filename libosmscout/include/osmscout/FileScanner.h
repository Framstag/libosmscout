#ifndef OSMSCOUT_FILESCANNER_H
#define OSMSCOUT_FILESCANNER_H

/*
  Import/TravelJinni - Openstreetmap offline viewer
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
  class FileScanner
  {
  private:
    FILE   *file;
    bool   hasError;
    bool   readOnly;

    // For mmap usage
    char   *buffer;
    size_t size;
    size_t offset;

  private:
    void FreeBuffer();

  public:
    FileScanner();
    virtual ~FileScanner();

    bool Open(const std::string& filename, bool readOnly=true);
    bool Close();

    bool IsOpen() const;
    bool HasError() const;

    bool SetPos(long pos);
    bool GetPos(long &pos);

    bool Read(std::string& value);
    bool Read(bool& boolean);
    bool Read(unsigned long& number);
    bool Read(unsigned int& number);

    bool ReadNumber(unsigned long& number);
    bool ReadNumber(unsigned int& number);
    bool ReadNumber(NodeCount& number);
    bool ReadNumber(long& number);
  };
}

#endif
