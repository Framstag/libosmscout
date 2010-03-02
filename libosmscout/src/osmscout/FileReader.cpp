/*
  TravelJinni - Openstreetmap offline viewer
  Copyright (C) 2009  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/FileReader.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>

#include <osmscout/Util.h>

#include <osmscout/private/Config.h>

#if defined(HAVE_MMAP)
  #include <unistd.h>
  #include <sys/mman.h>
#endif

namespace osmscout {

  FileReader::FileReader()
   : file(NULL),
     buffer(NULL),
     offset(0),
     hasError(true)
  {
    // no code
  }

  FileReader::~FileReader()
  {
    if (file!=NULL) {
      fclose(file);
    }
  }

  void FileReader::FreeBuffer()
  {
    if (buffer!=NULL) {
  #if defined(HAVE_MMAP)
      if (munmap(buffer,size)!=0) {
        std::cerr << "Error while calling munmap: "<< strerror(errno) << std::endl;
      }
  #else
      delete [] buffer;
  #endif

      buffer=NULL;
    }
  }

  bool FileReader::Open(const std::string& filename)
  {
    if (file!=NULL || buffer!=NULL) {
      return false;
    }

    file=fopen(filename.c_str(),"rb");

    hasError=file==NULL;

    return !hasError;
  }

  bool FileReader::Close()
  {
    bool result;

    if (file==NULL) {
      return false;
    }

    FreeBuffer();

    result=fclose(file)==0;

    if (result) {
      file=NULL;
    }

    return result;
  }

  bool FileReader::IsOpen() const
  {
    return file!=NULL;
  }

  bool FileReader::HasError() const
  {
    return file==NULL || buffer==NULL || hasError;
  }

  bool FileReader::SetPos(long pos)
  {
    if (file==NULL || buffer==NULL || hasError || pos>=size) {
      return false;
    }

    offset=pos;

    return true;
  }

  bool FileReader::GetPos(long& pos)
  {
    if (file==NULL || buffer==NULL || hasError) {
      return false;
    }

    pos=offset;

    return true;
  }

  bool FileReader::ReadFileToBuffer()
  {
    if (file==NULL) {
      return false;
    }

    FreeBuffer();

    long size;

    if (fseek(file,0L,SEEK_END)!=0) {
      hasError=true;
      return false;
    }

    size=ftell(file);

    if (size==-1) {
      hasError=true;
      return false;
    }

  #if defined(HAVE_MMAP)
    buffer=(char*)mmap(NULL,size,PROT_READ,MAP_SHARED,fileno(file),0);
    if (buffer==MAP_FAILED) {
      std::cerr << "Cannot mmap complete file: " << strerror(errno) << std::endl;
      buffer=NULL;
      return false;
    }
  #else
    if (fseek(file,0L,SEEK_SET)!=0) {
      hasError=true;
      return false;
    }

    buffer=new char[size];

    if (fread(buffer,sizeof(char),(size_t)size,file)!=(size_t)size) {
      hasError=true;

      FreeBuffer();

      return false;
    }
  #endif
    this->size=(size_t)size;
    this->offset=0;
    hasError=false;

    return true;
  }

  bool FileReader::ReadPageToBuffer(unsigned long offset, unsigned long size)
  {
    assert(offset>=0);
    assert(size>0);

    if (file==NULL) {
      return false;
    }

    FreeBuffer();

  #if defined(HAVE_MMAP)
    size_t mmapOff=(offset/getpagesize())*getpagesize();

    size=size+offset-mmapOff;

    buffer=(char*)mmap(NULL,size,PROT_READ,MAP_SHARED,fileno(file),mmapOff);
    if (buffer==MAP_FAILED) {
      std::cerr << "Cannot mmap page of file: " << strerror(errno) << std::endl;
      buffer=NULL;
      return false;
    }

    this->offset=offset-mmapOff;
    this->size=(size_t)size;

  #else
    if (fseek(file,offset,SEEK_SET)!=0) {
      hasError=true;
      return false;
    }

    buffer=new char[size];

    size_t res=0;

    if ((res=fread(buffer,sizeof(char),(size_t)size,file))!=(size_t)size) {
      hasError=true;

      FreeBuffer();

      return false;
    }
    this->offset=0;
    this->size=(size_t)size;
  #endif
    hasError=false;

    return true;
  }

  bool FileReader::Read(std::string& value)
  {
    if (file==NULL || buffer==NULL || hasError || offset>=size) {
      hasError=true;
      return false;
    }

    size_t start=offset;

    while (offset<size && buffer[offset]!='\0') {
      offset++;
    }

    value.assign(&buffer[start],offset-start);

    offset++;

    return true;
  }

  bool FileReader::Read(bool& boolean)
  {
    if (file==NULL || buffer==NULL || hasError || offset+sizeof(char)>size) {
      hasError=true;
      return false;
    }

    boolean=buffer[offset]!=0;

    offset+=sizeof(char);

    return true;
  }

  bool FileReader::Read(unsigned long& number)
  {
    if (file==NULL || buffer==NULL || hasError || offset+sizeof(unsigned long)>size) {
      hasError=true;
      return false;
    }

    number=0;

    for (size_t i=0; i<sizeof(unsigned long); i++) {
      number=number | (((unsigned char)buffer[offset+i]) << (i*8));
    }

    offset+=sizeof(unsigned long);

    return true;
  }

  bool FileReader::Read(unsigned int& number)
  {
    if (file==NULL || buffer==NULL || hasError || offset+sizeof(unsigned int)>size) {
      hasError=true;
      return false;
    }

    number=0;

    for (size_t i=0; i<sizeof(unsigned int); i++) {
      number=number | (((unsigned char)buffer[offset+i]) << (i*8));
    }

    offset+=sizeof(unsigned int);

    return true;
  }

  bool FileReader::ReadNumber(unsigned long& number)
  {
    if (file==NULL || buffer==NULL || hasError || offset>=size) {
      hasError=true;
      return false;
    }

    size_t bytes;

    if (DecodeNumber(&buffer[offset],number,bytes)) {
      offset+=bytes;

      return true;
    }
    else {
      hasError=true;

      return false;
    }
  }

  bool FileReader::ReadNumber(unsigned int& number)
  {
    if (file==NULL || buffer==NULL || hasError || offset>=size) {
      hasError=true;
      return false;
    }

    unsigned long value;
    size_t        bytes;

    if (DecodeNumber(&buffer[offset],value,bytes)) {
      offset+=bytes;

      if (value>(unsigned long)std::numeric_limits<unsigned int>::max()) {
        return false;
      }

      number=(unsigned int)value;

      return true;
    }
    else {
      hasError=true;

      return false;
    }
  }

  bool FileReader::ReadNumber(NodeCount& number)
  {
    if (file==NULL || buffer==NULL || hasError || offset>=size) {
      hasError=true;
      return false;
    }

    unsigned long value;
    size_t        bytes;

    if (DecodeNumber(&buffer[offset],value,bytes)) {
      offset+=bytes;

      if (value>(unsigned long)std::numeric_limits<NodeCount>::max()) {
        return false;
      }

      number=(unsigned int)value;

      return true;
    }
    else {
      hasError=true;

      return false;
    }
  }
}

