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

#include <osmscout/util/FileScanner.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <iostream>
#include <limits>

#if defined(HAVE_MMAP)
  #include <unistd.h>
  #include <sys/mman.h>
#endif

#if defined(HAVE_FCNTL_H)
  #include <fcntl.h>
#endif

#if defined(__WIN32__) || defined(WIN32)
  #include<io.h>

  #if !defined(_fileno)
    #define _fileno(__F) ((__F)->_file)
  #endif
#endif

#include <osmscout/system/Assert.h>

#include <osmscout/util/Number.h>

namespace osmscout {

  FileScanner::FileScanner()
   : file(NULL),
     hasError(true),
     buffer(NULL),
     size(0),
     offset(0)
#if defined(__WIN32__) || defined(WIN32)
     ,mmfHandle((HANDLE)0)
#endif
  {
    // no code
  }

  FileScanner::~FileScanner()
  {
    if (IsOpen()) {
      Close();
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
#elif  defined(__WIN32__) || defined(WIN32)
      if (buffer!=NULL) {
        UnmapViewOfFile(buffer);
        buffer=NULL;
      }
      if (mmfHandle!=NULL) {
        CloseHandle(mmfHandle);
        mmfHandle=NULL;
      }
#endif
  }

  bool FileScanner::Open(const std::string& filename,
                         Mode mode,
                         bool useMmap)
  {
    if (file!=NULL) {
      std::cerr << "File '" << filename << "' already opened, cannot open it again!" << std::endl;
      return false;
    }

    this->filename=filename;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      hasError=true;
      return false;
    }

#if defined(HAVE_FSEEKO)
    off_t size;

    if (fseeko(file,0L,SEEK_END)!=0) {
      std::cerr << "Cannot seek to end of file!" << std::endl;
      hasError=true;
      return false;
    }

    size=ftello(file);

    if (size==-1) {
      std::cerr << "Cannot get size of file!" << std::endl;
      hasError=true;
      return false;
    }

    this->size=(FileOffset)size;

    if (fseeko(file,0L,SEEK_SET)!=0) {
      std::cerr << "Cannot seek to start of file!" << std::endl;
      hasError=true;
      return false;
    }
#else
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

    this->size=(FileOffset)size;

    if (fseek(file,0L,SEEK_SET)!=0) {
      std::cerr << "Cannot seek to start of file!" << std::endl;
      hasError=true;
      return false;
    }
#endif

#if defined(HAVE_POSIX_FADVISE)
    if (mode==FastRandom) {
      if (posix_fadvise(fileno(file),0,size,POSIX_FADV_WILLNEED)<0) {
        std::cerr << "Cannot set file access advice: " << strerror(errno) << std::endl;
      }
    }
    else if (mode==Sequential) {
      if (posix_fadvise(fileno(file),0,size,POSIX_FADV_SEQUENTIAL)<0) {
        std::cerr << "Cannot set file access advice: " << strerror(errno) << std::endl;
      }
    }
    else if (mode==LowMemRandom) {
      if (posix_fadvise(fileno(file),0,size,POSIX_FADV_RANDOM)<0) {
        std::cerr << "Cannot set file access advice: " << strerror(errno) << std::endl;
      }
    }
#endif

#if defined(HAVE_MMAP)
    if (file!=NULL && useMmap && this->size>0) {
      FreeBuffer();

      buffer=(char*)mmap(NULL,size,PROT_READ,MAP_PRIVATE,fileno(file),0);
      if (buffer!=MAP_FAILED) {
        offset=0;
#if defined(HAVE_POSIX_MADVISE)
        if (mode==FastRandom) {
          if (posix_madvise(buffer,size,POSIX_MADV_WILLNEED)<0) {
            std::cerr << "Cannot set mmaped file access advice: " << strerror(errno) << std::endl;
          }
        }
        else if (mode==Sequential) {
          if (posix_madvise(buffer,size,POSIX_MADV_SEQUENTIAL)<0) {
            std::cerr << "Cannot set mmaped file access advice: " << strerror(errno) << std::endl;
          }
        }
        else if (mode==LowMemRandom) {
          if (posix_madvise(buffer,size,POSIX_MADV_RANDOM)<0) {
            std::cerr << "Cannot set mmaped file access advice: " << strerror(errno) << std::endl;
          }
        }
#endif
      }
      else {
        std::cerr << "Cannot mmap file " << filename << " of size " << size << ": " << strerror(errno) << std::endl;
        buffer=NULL;
      }
    }
#elif  defined(__WIN32__) || defined(WIN32)
    if (file!=NULL && useMmap && this->size>0) {
      FreeBuffer();

      mmfHandle=CreateFileMapping((HANDLE)_get_osfhandle(_fileno(file)),
                                  (LPSECURITY_ATTRIBUTES)NULL,
                                  PAGE_READONLY,
                                  0,0,
                                  (LPCTSTR)NULL);

      if (mmfHandle!=NULL) {
        buffer=(char*)MapViewOfFile(mmfHandle,
                                    FILE_MAP_READ,
                                    0,
                                    0,
                                    0);

        if (buffer!=NULL) {
          offset=0;
        }
        else {
          std::cerr << "Cannot map view for file " << filename << " of size " << size << ": " << GetLastError() << std::endl;
        }
      }
      else {
        std::cerr << "Cannot create file mapping for file " << filename << " of size " << size << ": " << GetLastError() << std::endl;
      }
    }
#endif

    hasError=file==NULL;

    return !hasError;
  }

  bool FileScanner::Close()
  {
    bool result;

    filename.clear();

    if (file==NULL) {
      std::cerr << "File already closed, cannot close it again!" << std::endl;
      return false;
    }

    FreeBuffer();

    result=fclose(file)==0;

    if (result) {
      file=NULL;
    }

    return result;
  }

  bool FileScanner::IsEOF() const
  {
    if (HasError()) {
      return true;
    }

    if (size==0) {
      return true;
    }

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      return offset>=size;
    }
#endif

    return feof(file);
  }

  std::string FileScanner::GetFilename() const
  {
    return filename;
  }

  bool FileScanner::GotoBegin()
  {
    return SetPos(0);
  }

  bool FileScanner::SetPos(FileOffset pos)
  {
    if (HasError()) {
      return false;
    }

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (pos>=(FileOffset)size) {
        return false;
      }

      offset=pos;

      return true;
    }
#endif

    clearerr(file);

#if defined(HAVE_FSEEKO)
    hasError=fseeko(file,(off_t)pos,SEEK_SET)!=0;
#else
    hasError=fseek(file,pos,SEEK_SET)!=0;
#endif

    if (hasError) {
      std::cerr << "Cannot set file pos:" << strerror(errno) << std::endl;
    }

    return !hasError;
  }

  bool FileScanner::GetPos(FileOffset& pos) const
  {
    if (HasError()) {
      return false;
    }

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      pos=offset;
      return true;
    }
#endif

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

    if (hasError) {
      std::cerr << "Cannot read file pos:" << strerror(errno) << std::endl;
    }

    return !hasError;
  }

  bool FileScanner::Read(char* buffer, size_t bytes)
  {
#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (this->buffer!=NULL) {
      if (offset+(FileOffset)bytes-1>=size) {
        std::cerr << "Cannot read byte array beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      memcpy(buffer,&this->buffer[offset],bytes);

      offset+=bytes;

      return true;
    }
#endif

    hasError=fread(buffer,1,bytes,file)!=bytes;

    if (hasError) {
      std::cerr << "Cannot read byte array beyond file end!" << std::endl;
      return false;
    }

    return true;
  }

  bool FileScanner::Read(std::string& value)
  {
    if (HasError()) {
      return false;
    }

    value.clear();

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read string beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      size_t start=offset;

      while (offset<(FileOffset)size &&
             buffer[offset]!='\0') {
        offset++;
      }

      value.assign(&buffer[start],offset-start);

      if (offset>=(FileOffset)size) {
        std::cerr << "String has no terminating '\\0' before file end!" << std::endl;
        hasError=true;
        return false;
      }

      offset++;

      return true;
    }
#endif

    char character;

    hasError=fread(&character,1,1,file)!=1;

    if (hasError) {
      std::cerr << "Cannot read string beyond file end!" << std::endl;
      return false;
    }

    while (character!='\0') {
      value.append(1,character);

      hasError=fread(&character,1,1,file)!=1;

      if (hasError) {
        std::cerr << "String has no terminating '\\0' before file end!" << std::endl;
        return false;
      }
    }

    return true;
  }

  bool FileScanner::Read(bool& boolean)
  {
    if (HasError()) {
      return false;
    }

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read bool beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      boolean=buffer[offset]!=0;

      offset++;

      return true;
    }
#endif

    char value;

    hasError=fread(&value,1,1,file)!=1;

    if (hasError) {
      std::cerr << "Cannot read bool beyond file end!" << std::endl;
      return false;
    }

    boolean=value!=0;

    return true;
  }

  bool FileScanner::Read(int8_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read int8_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      number=(int8_t)buffer[offset];

      offset++;

      return true;
    }
#endif

    hasError=fread(&number,1,1,file)!=1;

    if (hasError) {
      std::cerr << "Cannot read int8_t beyond file end!" << std::endl;
      return false;
    }

    return true;
  }

  bool FileScanner::Read(int16_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+2-1>=size) {
        std::cerr << "Cannot read int16_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char    *dataPtr=&buffer[offset];
      int16_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      offset+=2;

      return true;
    }
#endif

    unsigned char buffer[2];

    hasError=fread(&buffer,1,2,file)!=2;

    if (hasError) {
      std::cerr << "Cannot read int16_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    int16_t       add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;
    dataPtr++;

    return true;
  }

  bool FileScanner::Read(int32_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+4-1>=size) {
        std::cerr << "Cannot read int32_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char    *dataPtr=&buffer[offset];
      int32_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 16;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 24;
      number|=add;
      dataPtr++;

      offset+=4;

      return true;
    }
#endif

    unsigned char buffer[4];

    hasError=fread(&buffer,1,4,file)!=4;

    if (hasError) {
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    int32_t       add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 16;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 24;
    number|=add;
    dataPtr++;

    return true;
  }

#if defined(OSMSCOUT_HAVE_INT64_T)
  bool FileScanner::Read(int64_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        std::cerr << "Cannot read int64_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char    *dataPtr=&buffer[offset];
      int64_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 16;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 24;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 32;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 40;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 48;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 56;
      number|=add;
      dataPtr++;

      offset+=8;

      return true;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      std::cerr << "Cannot read int64_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    int64_t       add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 16;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 24;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 32;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 40;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 48;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 56;
    number|=add;
    dataPtr++;

    return true;
  }
#endif

  bool FileScanner::Read(uint8_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read uint8_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      number=(uint8_t)buffer[offset];

      offset++;

      return true;
    }
#endif

    hasError=fread(&number,1,1,file)!=1;

    if (hasError) {
      std::cerr << "Cannot read uint8_t beyond file end!" << std::endl;
      return false;
    }

    return true;
  }

  bool FileScanner::Read(uint16_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+2-1>=size) {
        std::cerr << "Cannot read uint16_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char     *dataPtr=&buffer[offset];
      uint16_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      offset+=2;

      return true;
    }
#endif

    unsigned char buffer[2];

    hasError=fread(&buffer,1,2,file)!=2;

    if (hasError) {
      std::cerr << "Cannot read uint16_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    uint16_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;
    dataPtr++;

    return true;
  }

  bool FileScanner::Read(uint32_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+4-1>=size) {
        std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char     *dataPtr=&buffer[offset];
      uint32_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 16;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 24;
      number|=add;
      dataPtr++;

      offset+=4;

      return true;
    }
#endif

    unsigned char buffer[4];

    hasError=fread(&buffer,1,4,file)!=4;

    if (hasError) {
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    uint32_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 16;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 24;
    number|=add;
    dataPtr++;

    return true;
  }

#if defined(OSMSCOUT_HAVE_UINT64_T)
  bool FileScanner::Read(uint64_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        std::cerr << "Cannot read uint64_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char     *dataPtr=&buffer[offset];
      uint64_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 16;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 24;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 32;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 40;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 48;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 56;
      number|=add;
      dataPtr++;

      offset+=8;

      return true;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      std::cerr << "Cannot read uint64_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    uint64_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 16;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 24;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 32;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 40;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 48;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 56;
    number|=add;
    dataPtr++;

    return true;
  }
#endif

  bool FileScanner::ReadFileOffset(FileOffset& fileOffset)
  {
    if (HasError()) {
      return false;
    }

    fileOffset=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        std::cerr << "Cannot read FileOffset beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char       *dataPtr=&buffer[offset];
      FileOffset add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 16;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 24;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 32;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 40;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 48;
      fileOffset|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 56;
      fileOffset|=add;
      dataPtr++;

      offset+=8;

      return true;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      std::cerr << "Cannot read FileOffset beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    FileOffset    add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 16;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 24;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 32;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 40;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 48;
    fileOffset|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 56;
    fileOffset|=add;
    dataPtr++;

    return true;
  }

  bool FileScanner::ReadFileOffset(FileOffset& fileOffset,
                                   size_t bytes)
  {
    if (HasError()) {
      return false;
    }

    assert(bytes>0 && bytes<=8);

    fileOffset=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+bytes-1>=size) {
        std::cerr << "Cannot read FileOffset beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char       *dataPtr=&buffer[offset];
      FileOffset add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      fileOffset|=add;
      dataPtr++;

      if (bytes>=2) {
        add=(unsigned char)(*dataPtr);
        add=add << 8;
        fileOffset|=add;
        dataPtr++;

        if (bytes>=3) {
          add=(unsigned char)(*dataPtr);
          add=add << 16;
          fileOffset|=add;
          dataPtr++;

          if (bytes>=4) {
            add=(unsigned char)(*dataPtr);
            add=add << 24;
            fileOffset|=add;
            dataPtr++;

            if (bytes>=5) {
              add=(unsigned char)(*dataPtr);
              add=add << 32;
              fileOffset|=add;
              dataPtr++;

              if (bytes>=6) {
                add=(unsigned char)(*dataPtr);
                add=add << 40;
                fileOffset|=add;
                dataPtr++;

                if (bytes>=7) {
                  add=(unsigned char)(*dataPtr);
                  add=add << 48;
                  fileOffset|=add;
                  dataPtr++;

                  if (bytes>=8) {
                    add=(unsigned char)(*dataPtr);
                    add=add << 56;
                    fileOffset|=add;
                    dataPtr++;
                  }
                }
              }
            }
          }
        }
      }

      offset+=bytes;

      return true;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,bytes,file)!=bytes;

    if (hasError) {
      std::cerr << "Cannot read FileOffset beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;
    FileOffset    add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    fileOffset|=add;
    dataPtr++;

    if (bytes>=2) {
      add=(unsigned char)(*dataPtr);
      add=add << 8;
      fileOffset|=add;
      dataPtr++;

      if (bytes>=3) {
        add=(unsigned char)(*dataPtr);
        add=add << 16;
        fileOffset|=add;
        dataPtr++;

        if (bytes>=4) {
          add=(unsigned char)(*dataPtr);
          add=add << 24;
          fileOffset|=add;
          dataPtr++;

          if (bytes>=5) {
            add=(unsigned char)(*dataPtr);
            add=add << 32;
            fileOffset|=add;
            dataPtr++;

            if (bytes>=6) {
              add=(unsigned char)(*dataPtr);
              add=add << 40;
              fileOffset|=add;
              dataPtr++;

              if (bytes>=7) {
                add=(unsigned char)(*dataPtr);
                add=add << 48;
                fileOffset|=add;
                dataPtr++;

                if (bytes>=8) {
                  add=(unsigned char)(*dataPtr);
                  add=add << 56;
                  fileOffset|=add;
                  dataPtr++;
                }
              }
            }
          }
        }
      }
    }

    return true;
  }

  bool FileScanner::ReadNumber(int16_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read uint16_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return true;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      std::cerr << "Cannot read int16_t number beyond file end!" << std::endl;
      hasError=true;
      return false;
    }

    typedef int16_t num_t;

    unsigned int shift=0;
    unsigned int nextShift=0;

    // negative form
    if ((buffer & 0x01)!=0) {
      char val=(buffer & 0x7e) >> 1;

      number=-1;
      nextShift=6;

      while ((buffer & 0x80)!=0) {

        if (fread(&buffer,1,1,file)!=1) {
          std::cerr << "Cannot read int16_t number beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        number^=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number^=static_cast<num_t>(val) << shift;
    }
    else {
      char val=(buffer & 0x7e) >> 1;

      number=0;
      nextShift=6;

      while ((buffer & 0x80)!=0) {

        if (fread(&buffer,1,1,file)!=1) {
          std::cerr << "Cannot read int16_t number beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        number|=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number|=static_cast<num_t>(val) << shift;
    }

    return true;
  }

  bool FileScanner::ReadNumber(int32_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read int32_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return true;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      std::cerr << "Cannot read int32_t number beyond file end!" << std::endl;
      hasError=true;
      return false;
    }


    typedef int32_t num_t;

    unsigned int shift=0;
    unsigned int nextShift=0;

    // negative form
    if ((buffer & 0x01)!=0) {
      char val=(buffer & 0x7e) >> 1;

      number=-1;
      nextShift=6;

      while ((buffer & 0x80)!=0) {

        if (fread(&buffer,1,1,file)!=1) {
          std::cerr << "Cannot read int32_t number beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        number^=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number^=static_cast<num_t>(val) << shift;
    }
    else {
      char val=(buffer & 0x7e) >> 1;

      number=0;
      nextShift=6;

      while ((buffer & 0x80)!=0) {

        if (fread(&buffer,1,1,file)!=1) {
          std::cerr << "Cannot read int32_t number beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        number|=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number|=static_cast<num_t>(val) << shift;
    }

    return true;
  }

#if defined(OSMSCOUT_HAVE_INT64_T)
  bool FileScanner::ReadNumber(int64_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read int64_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return true;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      std::cerr << "Cannot read int64_t number beyond file end!" << std::endl;
      hasError=true;
      return false;
    }

    typedef int64_t num_t;

    unsigned int shift=0;
    unsigned int nextShift=0;

    // negative form
    if ((buffer & 0x01)!=0) {
      char val=(buffer & 0x7e) >> 1;

      number=-1;
      nextShift=6;

      while ((buffer & 0x80)!=0) {

        if (fread(&buffer,1,1,file)!=1) {
          std::cerr << "Cannot read int64_t number beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        number^=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number^=static_cast<num_t>(val) << shift;
    }
    else {
      char val=(buffer & 0x7e) >> 1;

      number=0;
      nextShift=6;

      while ((buffer & 0x80)!=0) {

        if (fread(&buffer,1,1,file)!=1) {
          std::cerr << "Cannot read int64_t number beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        number|=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number|=static_cast<num_t>(val) << shift;
    }

    return true;
  }
#endif

  bool FileScanner::ReadNumber(uint16_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read uint16_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return true;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      std::cerr << "Cannot read uint16_t number beyond file end!" << std::endl;
      hasError=true;
      return false;
    }

    unsigned int shift=0;

    while (true) {
      uint16_t add;

      add=(unsigned char)buffer;
      add=add & 0x7f;
      add=add << shift;

      number|=add;

      if ((buffer & 0x80)==0) {
        return true;
      }

      if (fread(&buffer,1,1,file)!=1) {
        std::cerr << "Cannot read uint16_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      shift+=7;
    }

    return true;
  }

  bool FileScanner::ReadNumber(uint32_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read uint32_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return true;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      std::cerr << "Cannot read uint32_t number beyond file end!" << std::endl;
      hasError=true;
      return false;
    }

    unsigned int shift=0;

    while (true) {
      uint32_t add;

      add=(unsigned char)buffer;
      add=add & 0x7f;
      add=add << shift;

      number|=add;

      if ((buffer & 0x80)==0) {
        return true;
      }

      if (fread(&buffer,1,1,file)!=1) {
        std::cerr << "Cannot read uint32_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      shift+=7;
    }

    return true;
  }

#if defined(OSMSCOUT_HAVE_UINT64_T)
  bool FileScanner::ReadNumber(uint64_t& number)
  {
    if (HasError()) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        std::cerr << "Cannot read uint64_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return true;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      std::cerr << "Cannot read uint64_t number beyond file end!" << std::endl;
      hasError=true;
      return false;
    }

    unsigned int shift=0;

    while (true) {
      uint64_t add;

      add=(unsigned char)buffer;
      add=add & 0x7f;
      add=add << shift;

      number|=add;

      if ((buffer & 0x80)==0) {
        return true;
      }

      if (fread(&buffer,1,1,file)!=1) {
        std::cerr << "Cannot read uint64_t number beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      shift+=7;
    }

    return true;
  }
#endif

  bool FileScanner::ReadCoord(GeoCoord& coord)
  {
    if (HasError()) {
      return false;
    }

    uint32_t latDat;
    uint32_t lonDat;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char *dataPtr=&buffer[offset];

      latDat=  ((unsigned char) dataPtr[0] <<  0)
             | ((unsigned char) dataPtr[1] <<  8)
             | ((unsigned char) dataPtr[2] << 16)
             | ((unsigned char) dataPtr[3] << 24);

      lonDat=  ((unsigned char) dataPtr[4] <<  0)
             | ((unsigned char) dataPtr[5] <<  8)
             | ((unsigned char) dataPtr[6] << 16)
             | ((unsigned char) dataPtr[7] << 24);

      dataPtr+=2*4;
      offset+=2*4;

      coord.Set(latDat/latConversionFactor-90.0,
                lonDat/lonConversionFactor-180.0);

      return true;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;

    latDat=  ((unsigned char) dataPtr[0] <<  0)
           | ((unsigned char) dataPtr[1] <<  8)
           | ((unsigned char) dataPtr[2] << 16)
           | ((unsigned char) dataPtr[3] << 24);

    lonDat=  ((unsigned char) dataPtr[4] <<  0)
           | ((unsigned char) dataPtr[5] <<  8)
           | ((unsigned char) dataPtr[6] << 16)
           | ((unsigned char) dataPtr[7] << 24);

    dataPtr+=2*4;

    coord.Set(latDat/latConversionFactor-90.0,
              lonDat/lonConversionFactor-180.0);

    return true;
  }

  bool FileScanner::ReadConditionalCoord(GeoCoord& coord,
                                         bool& isSet)
  {
    if (HasError()) {
      return false;
    }

    uint32_t latDat;
    uint32_t lonDat;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      char *dataPtr=&buffer[offset];

      latDat=  ((unsigned char) dataPtr[0] <<  0)
             | ((unsigned char) dataPtr[1] <<  8)
             | ((unsigned char) dataPtr[2] << 16)
             | ((unsigned char) dataPtr[3] << 24);

      lonDat=  ((unsigned char) dataPtr[4] <<  0)
             | ((unsigned char) dataPtr[5] <<  8)
             | ((unsigned char) dataPtr[6] << 16)
             | ((unsigned char) dataPtr[7] << 24);

      dataPtr+=2*4;
      offset+=2*4;

      if (latDat==0xffffffff || lonDat==0xffffffff) {
        isSet=false;
      }
      else  {
        coord.Set(latDat/latConversionFactor-90.0,
                  lonDat/lonConversionFactor-180.0);
        isSet=true;
      }

      return true;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      return false;
    }

    unsigned char *dataPtr=buffer;

    latDat=  ((unsigned char) dataPtr[0] <<  0)
           | ((unsigned char) dataPtr[1] <<  8)
           | ((unsigned char) dataPtr[2] << 16)
           | ((unsigned char) dataPtr[3] << 24);

    lonDat=  ((unsigned char) dataPtr[4] <<  0)
           | ((unsigned char) dataPtr[5] <<  8)
           | ((unsigned char) dataPtr[6] << 16)
           | ((unsigned char) dataPtr[7] << 24);

    dataPtr+=2*4;

    if (latDat==0xffffffff || lonDat==0xffffffff) {
      isSet=false;
    }
    else  {
      coord.Set(latDat/latConversionFactor-90.0,
                lonDat/lonConversionFactor-180.0);
      isSet=true;
    }

    return true;
  }

  bool FileScanner::Read(std::vector<GeoCoord>& nodes)
  {
    uint32_t nodeCount;

    if (!ReadNumber(nodeCount)) {
      return false;
    }

    GeoCoord minCoord;

    if (!ReadCoord(minCoord)) {
      return false;
    }

    nodes.resize(nodeCount);
    for (size_t i=0; i<nodeCount; i++) {
      uint32_t latValue;
      uint32_t lonValue;

      if (!ReadNumber(latValue) ||
          !ReadNumber(lonValue)) {
        return false;
      }

      nodes[i].Set(minCoord.GetLat()+latValue/latConversionFactor,
                   minCoord.GetLon()+lonValue/lonConversionFactor);
    }

    return !HasError();
  }

  bool FileScanner::Read(std::vector<GeoCoord>& nodes,
                         size_t count)
  {
    GeoCoord minCoord;

    if (!ReadCoord(minCoord)) {
      return false;
    }

    nodes.resize(count);
    for (size_t i=0; i<count; i++) {
      uint32_t latValue;
      uint32_t lonValue;

      if (!ReadNumber(latValue) ||
          !ReadNumber(lonValue)) {
        return false;
      }

      nodes[i].Set(minCoord.GetLat()+latValue/latConversionFactor,
                   minCoord.GetLon()+lonValue/lonConversionFactor);
    }

    return !HasError();
  }
}

