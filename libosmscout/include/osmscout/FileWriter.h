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

#include <stdint.h>

#include <osmscout/Types.h>

namespace osmscout {
  class FileWriter
  {
  private:
    FILE *file;
    bool hasError;

  public:
    FileWriter();
    virtual ~FileWriter();

    bool Open(const std::string& filename);
    bool Close();
    bool IsOpen() const;

    bool HasError() const;

    bool GetPos(FileOffset &pos);
    bool SetPos(FileOffset pos);

    bool Write(const std::string& value);
    bool Write(bool boolean);
    bool Write(uint16_t number);
    bool Write(uint32_t number);
    bool Write(int8_t number);
    bool Write(int32_t number);

    bool WriteNumber(unsigned long number);
    bool WriteNumber(long number);
  };
}

#endif
