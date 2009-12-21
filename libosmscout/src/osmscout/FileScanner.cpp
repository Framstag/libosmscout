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

#include <osmscout/FileScanner.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>

#include <osmscout/private/Config.h>

#if defined(HAVE_MMAP)
  #include <unistd.h>
  #include <sys/mman.h>
#endif

#include <osmscout/Util.h>

FileScanner::FileScanner()
 : file(NULL),
   hasError(true),
   buffer(NULL),
   offset(0)
{
  // no code
}

FileScanner::~FileScanner()
{
  if (file!=NULL) {
    fclose(file);
  }
}

void FileScanner::FreeBuffer()
{
#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    if (munmap(buffer,size)!=0) {
      std::cerr << "Error while calling munmap: "<< strerror(errno) << std::endl;
    }

    buffer=NULL;
  }
#endif
}

bool FileScanner::Open(const std::string& filename, bool readOnly)
{
  if (file!=NULL) {
    return false;
  }

  this->readOnly=readOnly;

  if (readOnly) {
    file=fopen(filename.c_str(),"rb");
  }
  else {
    file=fopen(filename.c_str(),"r+b");
  }

#if defined(HAVE_MMAP)
  if (file!=NULL && readOnly) {
    FreeBuffer();

    long size;

    if (fseek(file,0L,SEEK_END)!=0) {
      std::cerr << "Cannot seek to end of file!" << std::endl;
      hasError=true;
      return false;
    }

    size=ftell(file);

    if (size==-1) {
      std::cerr << "Cannot get size of file!" << std::endl;
      hasError=true;
      return false;
    }

    buffer=(char*)mmap(NULL,size,PROT_READ,MAP_SHARED,fileno(file),0);
    if (buffer!=MAP_FAILED) {
      this->size=(size_t)size;
      this->offset=0;
    }
    else {
      std::cerr << "Cannot mmap complete file: " << strerror(errno) << std::endl;
      buffer=NULL;
    }
  }
#endif

  hasError=file==NULL;

  return !hasError;
}

bool FileScanner::Close()
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

bool FileScanner::IsOpen() const
{
  return file!=NULL;
}

bool FileScanner::HasError() const
{
  return file==NULL || hasError;
}

bool FileScanner::SetPos(long pos)
{
  if (file==NULL || hasError) {
    return false;
  }

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    if (pos>=size) {
      return false;
    }

    offset=pos;

    return true;
  }
#endif

  hasError=fseek(file,pos,SEEK_SET)!=0;

  return !hasError;
}

bool FileScanner::GetPos(long& pos)
{
  if (file==NULL || hasError) {
  }

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    pos=offset;
    return true;
  }
#endif

  pos=ftell(file);

  hasError=pos==-1;

  return !hasError;
}

bool FileScanner::Read(std::string& value)
{
  if (file==NULL || hasError) {
    return false;
  }

  value.clear();

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    size_t start=offset;

    while (offset<size && buffer[offset]!='\0') {
      offset++;
    }

    value.assign(&buffer[start],offset-start);

    offset++;

    return true;
  }
#endif

  char character;

  hasError=fread(&character,sizeof(char),1,file)!=1;

  if (hasError) {
    return false;
  }

  while (character!='\0') {
    value.append(1,character);

    hasError=fread(&character,sizeof(char),1,file)!=1;

    if (hasError) {
      return false;
    }
  }

  return true;
}

bool FileScanner::Read(bool& boolean)
{
  if (file==NULL || hasError) {
    return false;
  }

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    if (offset+sizeof(char)>size) {
      hasError=true;
      return false;
    }

    boolean=buffer[offset]!=0;

    offset+=sizeof(char);

    return true;
  }
#endif

  char value;

  hasError=fread(&value,sizeof(char),1,file)!=1;

  if (hasError) {
    return false;
  }

  boolean=value!=0;

  return true;
}

bool FileScanner::Read(unsigned long& number)
{
  if (file==NULL || hasError) {
    return false;
  }

  number=0;

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    if (offset+sizeof(unsigned long)>size) {
      hasError=true;
      return false;
    }

    for (size_t i=0; i<sizeof(unsigned long); i++) {
    number=number | (((unsigned char)buffer[offset+i]) << (i*8));
    }

    offset+=sizeof(unsigned long);

    return true;
  }
#endif

  unsigned char buffer[sizeof(unsigned long)];

  hasError=fread(&buffer,sizeof(char),sizeof(unsigned long),file)!=sizeof(unsigned long);

  if (!hasError) {
    for (size_t i=0; i<sizeof(unsigned long); i++) {
      number=number | (buffer[i] << (i*8));
    }
  }

  return !hasError;
}

bool FileScanner::Read(unsigned int& number)
{
  if (file==NULL || hasError) {
    return false;
  }

  number=0;

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    if (offset+sizeof(unsigned int)>size) {
      hasError=true;
      return false;
    }

    for (size_t i=0; i<sizeof(unsigned int); i++) {
      number=number | (((unsigned char)buffer[offset+i]) << (i*8));
    }

    offset+=sizeof(unsigned int);

    return true;
  }
#endif

  unsigned char buffer[sizeof(unsigned int)];

  hasError=fread(&buffer,sizeof(char),sizeof(unsigned int),file)!=sizeof(unsigned int);

  if (!hasError) {
    for (size_t i=0; i<sizeof(unsigned int); i++) {
      number=number | (buffer[i] << (i*8));
    }
  }

  return true;
}

bool FileScanner::ReadNumber(unsigned long& number)
{
  if (file==NULL || hasError) {
    return false;
  }

  number=0;

#if defined(HAVE_MMAP)
  if (buffer!=NULL) {
    if (offset>=size) {
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
#endif

  char buffer;

  if (fread(&buffer,sizeof(char),1,file)!=1) {
    hasError=true;
    return false;
  }

  if (buffer==0) {
    return true;
  }
  else {
    size_t idx=0;

    while (true) {
      unsigned long add=(buffer & 0x7f);

      number=number | (add << (idx*7));

      if ((buffer & 0x80)==0) {
        return true;
      }

      if (fread(&buffer,sizeof(char),1,file)!=1) {
        hasError=true;
        return false;
      }

      idx++;
    };
  }
}

bool FileScanner::ReadNumber(unsigned int& number)
{
  unsigned long value;

  if (!ReadNumber(value)) {
    return false;
  }

  if (value>(unsigned long)std::numeric_limits<int>::max()) {
    hasError=true;
    return false;
  }

  number=(unsigned int)value;

  return true;
}

bool FileScanner::ReadNumber(NodeCount& number)
{
  unsigned long value;

  if (!ReadNumber(value)) {
    return false;
  }

  if (value>(unsigned long)std::numeric_limits<NodeCount>::max()) {
    hasError=true;
    return false;
  }

  number=(NodeCount)value;

  return true;
}

