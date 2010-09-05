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

#include <osmscout/FileScanner.h>

#include <cassert>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <limits>

#include <osmscout/Private/Config.h>

#if defined(HAVE_MMAP)
  #include <unistd.h>
  #include <sys/mman.h>
#endif

#if defined(__WIN32__) || defined(WIN32)
  #include<io.h>
#endif

#include <osmscout/Util.h>

namespace osmscout {

  FileScanner::FileScanner()
   : file(NULL),
     hasError(true),
     buffer(NULL),
     offset(0)
#if defined(__WIN32__) || defined(WIN32)
     ,mmfHandle((HANDLE)0)
#endif
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

  bool FileScanner::Open(const std::string& filename, bool readOnly)
  {
    if (file!=NULL) {
      std::cerr << "File already opened, cannot open it again!" << std::endl;
      return false;
    }

    this->readOnly=readOnly;

    if (readOnly) {
      file=fopen(filename.c_str(),"rb");
    }
    else {
      file=fopen(filename.c_str(),"r+b");
    }

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

    this->size=(size_t)size;

    if (fseek(file,0L,SEEK_SET)!=0) {
      std::cerr << "Cannot seek to start of file!" << std::endl;
      hasError=true;
      return false;
    }
    
#if defined(HAVE_MMAP)
    if (file!=NULL && readOnly && this->size>0) {
      FreeBuffer();

      buffer=(char*)mmap(NULL,size,PROT_READ,MAP_SHARED,fileno(file),0);
      if (buffer!=MAP_FAILED) {
        offset=0;
      }
      else {
        std::cerr << "Cannot mmap file " << filename << " of size " << size << ": " << strerror(errno) << std::endl;
        buffer=NULL;
      }
    }
#elif  defined(__WIN32__) || defined(WIN32)
    if (file!=NULL && readOnly && this->size>0) {
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

    if (!hasError) {
      this->filename=filename;
    }

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

  bool FileScanner::IsOpen() const
  {
    return file!=NULL;
  }

  bool FileScanner::IsEOF() const
  {
    FileOffset offset;

    if (!GetPos(offset)) {
      return true;
    }

    return offset>=(FileOffset)size;
  }

  bool FileScanner::HasError() const
  {
    return file==NULL || hasError;
  }

  std::string FileScanner::GetFilename() const
  {
    return filename;
  }

  bool FileScanner::SetPos(FileOffset pos)
  {
    if (file==NULL || hasError) {
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

    hasError=fseek(file,pos,SEEK_SET)!=0;

    return !hasError;
  }

  bool FileScanner::GetPos(FileOffset& pos) const
  {
    if (file==NULL || hasError) {
      return false;
    }

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      pos=offset;
      return true;
    }
#endif

    pos=ftell(file);

    hasError=pos==-1;

    if (hasError) {
      std::cerr << "Cannot read file pos:" << strerror(errno) << std::endl;
    }

    return !hasError;
  }

  bool FileScanner::Read(std::string& value)
  {
    if (file==NULL || hasError) {
      return false;
    }

    value.clear();

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(char)>size) {
        std::cerr << "Cannot read string beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      size_t start=offset;

      while (offset<(FileOffset)size && buffer[offset]!='\0') {
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
      std::cerr << "Cannot read string beyond file end!" << std::endl;
      return false;
    }

    while (character!='\0') {
      value.append(1,character);

      hasError=fread(&character,sizeof(char),1,file)!=1;

      if (hasError) {
        return true;
      }
    }

    return true;
  }

  bool FileScanner::Read(bool& boolean)
  {
    if (file==NULL || hasError) {
      return false;
    }

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(char)>size) {
        std::cerr << "Cannot read bool beyond file end!" << std::endl;
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
      std::cerr << "Cannot read bool beyond file end!" << std::endl;
      return false;
    }

    boolean=value!=0;

    return true;
  }

  bool FileScanner::Read(uint8_t& number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(uint8_t)>size) {
        std::cerr << "Cannot read uint8_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

        number=buffer[offset];

      offset+=sizeof(uint8_t);

      return true;
    }
#endif

    hasError=fread(&number,sizeof(char),sizeof(uint8_t),file)!=sizeof(uint8_t);

    if (hasError) {
      std::cerr << "Cannot read uint8_t beyond file end!" << std::endl;
      return false;
    }

    return true;
  }

  bool FileScanner::Read(uint16_t& number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(uint16_t)>size) {
        std::cerr << "Cannot read uint16_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      for (size_t i=0; i<sizeof(uint16_t); i++) {
        number=number | (((unsigned char)buffer[offset+i]) << (i*8));
      }

      offset+=sizeof(uint16_t);

      return true;
    }
#endif

    unsigned char buffer[sizeof(uint16_t)];

    hasError=fread(&buffer,sizeof(char),sizeof(uint16_t),file)!=sizeof(uint16_t);

    if (hasError) {
      std::cerr << "Cannot read uint16_t beyond file end!" << std::endl;
      return false;
    }

    for (size_t i=0; i<sizeof(uint16_t); i++) {
      number=number | (buffer[i] << (i*8));
    }

    return true;
  }

  bool FileScanner::Read(uint32_t& number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(uint32_t)>size) {
        std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      for (size_t i=0; i<sizeof(uint32_t); i++) {
      number=number | (((unsigned char)buffer[offset+i]) << (i*8));
      }

      offset+=sizeof(uint32_t);

      return true;
    }
#endif

    unsigned char buffer[sizeof(uint32_t)];
    
    hasError=fread(&buffer,sizeof(unsigned char),sizeof(uint32_t),file)!=sizeof(uint32_t);
    
    if (hasError) {
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      return false;
    }

    for (size_t i=0; i<sizeof(uint32_t); i++) {
      number=number | (buffer[i] << (i*8));
    }

    return true;
  }

  bool FileScanner::Read(int8_t& number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(int8_t)>size) {
        std::cerr << "Cannot read int8_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      for (size_t i=0; i<sizeof(int8_t); i++) {
        number=number | (((unsigned char)buffer[offset+i]) << (i*8));
      }

      offset+=sizeof(int8_t);

      return true;
    }
#endif

    unsigned char buffer[sizeof(int8_t)];

    hasError=fread(&buffer,sizeof(char),sizeof(int8_t),file)!=sizeof(int8_t);

    if (hasError) {
      std::cerr << "Cannot read int8_t beyond file end!" << std::endl;
      return false;
    }

    for (size_t i=0; i<sizeof(int8_t); i++) {
      number=number | (buffer[i] << (i*8));
    }

    return true;
  }

  bool FileScanner::Read(int32_t& number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset+sizeof(uint32_t)>size) {
        std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
        hasError=true;
        return false;
      }

      for (size_t i=0; i<sizeof(uint32_t); i++) {
        number=number | (((unsigned char)buffer[offset+i]) << (i*8));
      }

      offset+=sizeof(uint32_t);

      return true;
    }
#endif

    unsigned char buffer[sizeof(uint32_t)];

    hasError=fread(&buffer,sizeof(char),sizeof(uint32_t),file)!=sizeof(uint32_t);

    if (hasError) {
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      return false;
    }

    for (size_t i=0; i<sizeof(uint32_t); i++) {
      number=number | (buffer[i] << (i*8));
    }

    return true;
  }

  bool FileScanner::ReadNumber(uint32_t& number)
  {
    if (file==NULL || hasError) {
      return false;
    }

    number=0;

#if defined(HAVE_MMAP) || defined(__WIN32__) || defined(WIN32)
    if (buffer!=NULL) {
      if (offset>=(FileOffset)size) {
        std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
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
      std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
      hasError=true;
      return false;
    }

    if (buffer==0) {
      return true;
    }
    else {
      size_t idx=0;

      while (true) {
        uint32_t add=(buffer & 0x7f);

        number=number | (add << (idx*7));

        if ((buffer & 0x80)==0) {
          return true;
        }

        if (fread(&buffer,sizeof(char),1,file)!=1) {
          std::cerr << "Cannot read uint32_t beyond file end!" << std::endl;
          hasError=true;
          return false;
        }

        idx++;
      };
    }
  }

  bool FileScanner::ReadNumber(uint16_t& number)
  {
    uint32_t value;

    if (!ReadNumber(value)) {
      return false;
    }

    if (value>(uint32_t)std::numeric_limits<uint16_t>::max()) {
      hasError=true;
      return false;
    }

    number=(uint16_t)value;

    return true;
  }

  bool FileScanner::ReadNumber(uint8_t& number)
  {
    uint32_t value;

    if (!ReadNumber(value)) {
      return false;
    }

    if (value>(uint32_t)std::numeric_limits<uint8_t>::max()) {
      hasError=true;
      return false;
    }

    number=(uint8_t)value;

    return true;
  }

  /**
    TODO: Handle real negative numbers!
    */
  bool FileScanner::ReadNumber(int32_t& number)
  {
    uint32_t value;

    if (!ReadNumber(value)) {
      return false;
    }

    if ((int32_t)value>(int32_t)std::numeric_limits<int32_t>::max()) {
      hasError=true;
      return false;
    }

    number=(int32_t)value;

    return true;
  }
}

