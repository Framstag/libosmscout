#ifndef OSMSCOUT_FILEREADER_H
#define OSMSCOUT_FILEREADER_H

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

namespace osmscout {
  /**
    FileReader implements platform independend access to blocks of data
    in files.

    It is assumed that blocks fit into memory. FileReader offers read methods
    for various datatypes. The data first must be loaded into memory by
    ReadPageIntoBuffer and then will be parsed by using the various Read methods.

    Note that FileReader works but still should be avoided, because working with
    OSM data often result in huge amounts of data that cannot be loaded
    into memory. So using FileReader might signal that something will fail for
    huge amounts of data. Try to use FileScanner instead!

    FileReader will use mmap if available.
    */
  class FileReader
  {
  private:
    FILE   *file;
    char   *buffer;
    size_t size;
    size_t offset;
    bool   hasError;

  private:
    void FreeBuffer();

  public:
    FileReader();
    virtual ~FileReader();

    bool Open(const std::string& filename);
    bool Close();

    bool IsOpen() const;
    bool HasError() const;

    bool SetPos(long pos);
    bool GetPos(long &pos);

    bool ReadFileToBuffer();
    bool ReadPageToBuffer(unsigned long offset, unsigned long size);

    bool Read(std::string& value);
    bool Read(bool& boolean);
    bool Read(unsigned long& number);
    bool Read(unsigned int& number);

    bool ReadNumber(unsigned long& number);
    bool ReadNumber(unsigned int& number);
    bool ReadNumber(NodeCount& number);
  };
}

#endif
