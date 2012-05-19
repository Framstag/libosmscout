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

#include <osmscout/private/Config.h>

#include <osmscout/util/FileWriter.h>

#include <cstring>
#include <cstdio>

#include <osmscout/util/Number.h>

namespace osmscout {

  FileWriter::FileWriter()
   : file(NULL),
     hasError(true)
  {
    // no code
  }

  FileWriter::~FileWriter()
  {
    if (file!=NULL) {
      fclose(file);
    }
  }

  bool FileWriter::Open(const std::string& filename)
  {
    if (file!=NULL) {
      return false;
    }

    file=fopen(filename.c_str(),"w+b");

    hasError=file==NULL;

    if (!hasError) {
      this->filename=filename;
    }

    return !hasError;
  }

  bool FileWriter::Close()
  {
    if (file==NULL) {
      return false;
    }

    hasError=fclose(file)!=0;

    if (!hasError) {
      file=NULL;
    }

    return !hasError;
  }

  std::string FileWriter::GetFilename() const
  {
    return filename;
  }

  bool FileWriter::GetPos(FileOffset& pos)
  {
    if (HasError()) {
      return false;
    }

#if defined(HAVE_FSEEKO)
    off_t filepos=ftello(file);

    if (filepos==-1) {
      hasError=true;
    }
    else {
      pos=(FileOffset)filepos;
    }
#else
    long filepos=ftell(file);

    if (filepos==-1) {
      hasError=true;
    }
    else {
      pos=(FileOffset)filepos;
    }
#endif

    return !hasError;
  }

  bool FileWriter::SetPos(FileOffset pos)
  {
    if (HasError()) {
      return false;
    }

#if defined(HAVE_FSEEKO)
    hasError=fseeko(file,(off_t)pos,SEEK_SET)!=0;
#else
    hasError=fseek(file,pos,SEEK_SET)!=0;
#endif

    return !hasError;
  }

  bool FileWriter::Write(const char* buffer, size_t bytes)
  {
    hasError=fwrite(buffer,sizeof(char),bytes,file)!=bytes;

    return !hasError;
  }

  bool FileWriter::Write(const std::string& value)
  {
    size_t length=value.length()+1;

    hasError=fwrite(value.c_str(),sizeof(char),length,file)!=length;

    return !hasError;
  }

  bool FileWriter::Write(bool boolean)
  {
    if (HasError()) {
      return false;
    }

    char value=boolean ? 1 : 0;

    hasError=fwrite((const char*)&value,sizeof(char),1,file)!=1;

    return !hasError;
  }

  bool FileWriter::Write(int8_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[sizeof(int8_t)];
    unsigned int mask=0xff;

    for (size_t i=0; i<sizeof(int8_t); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(int8_t),file)!=sizeof(int8_t);

    return !hasError;
  }

  bool FileWriter::Write(uint8_t number)
  {
    if (HasError()) {
      return false;
    }

    hasError=fwrite(&number,sizeof(char),sizeof(uint8_t),file)!=sizeof(uint8_t);

    return !hasError;
  }

  bool FileWriter::Write(uint16_t number)
  {
    if (HasError()) {
      return false;
    }

    char     buffer[sizeof(uint16_t)];
    uint16_t mask=0xff;

    for (size_t i=0; i<sizeof(uint16_t); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(uint16_t),file)!=sizeof(uint16_t);

    return !hasError;
  }

  bool FileWriter::Write(uint32_t number)
  {
    if (HasError()) {
      return false;
    }

    char     buffer[sizeof(uint32_t)];
    uint32_t mask=0xff;

    for (size_t i=0; i<sizeof(uint32_t); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(uint32_t),file)!=sizeof(uint32_t);

    return !hasError;
  }

#if defined(OSMSCOUT_HAVE_UINT64_T)
  bool FileWriter::Write(uint64_t number)
  {
    if (HasError()) {
      return false;
    }

    char     buffer[sizeof(uint64_t)];
    uint64_t mask=0xff;

    for (size_t i=0; i<sizeof(uint64_t); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(uint64_t),file)!=sizeof(uint64_t);

    return !hasError;
  }
#endif

  bool FileWriter::WriteFileOffset(FileOffset offset)
  {
    if (sizeof(offset)==4) {
      char buffer[4];

      buffer[0]=0;
      buffer[1]=0;
      buffer[2]=0;
      buffer[3]=0;

      if (!Write(buffer,4)) {
        return false;
      }
    }

    return Write(offset);
  }


  bool FileWriter::Write(int32_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[sizeof(FileOffset)];
    unsigned int mask=0xff;

    for (size_t i=0; i<sizeof(FileOffset); i++) {
      buffer[i]=(number >> (i*8)) & mask;
    }

    hasError=fwrite(buffer,sizeof(char),sizeof(FileOffset),file)!=sizeof(FileOffset);

    return !hasError;
  }

#if defined(OSMSCOUT_HAVE_UINT64_T)
  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.
    */
  bool FileWriter::WriteNumber(uint64_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    return !hasError;
  }
#endif

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.
    */
  bool FileWriter::WriteNumber(uint32_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    return !hasError;
  }

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.
    */
  bool FileWriter::WriteNumber(uint16_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    return !hasError;
  }

  bool FileWriter::FlushCurrentBlockWithZeros(size_t blockSize)
  {
    if (HasError()) {
      return false;
    }

    FileOffset currentPos;
    size_t     bytesToWrite;

    if (!GetPos(currentPos)) {
      return false;
    }

    if (currentPos%blockSize==0) {
      return true;
    }

    bytesToWrite=blockSize-(currentPos%blockSize);

    char *buffer=new char[blockSize];

    memset(buffer,0,bytesToWrite);

    hasError=fwrite(buffer,sizeof(char),bytesToWrite,file)!=bytesToWrite;

    delete buffer;

    return !hasError;
  }
}

