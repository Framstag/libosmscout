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

#include <limits>

#if defined(HAVE_MMAP)
  #include <unistd.h>
  #include <sys/mman.h>
#endif

#if defined(HAVE_FCNTL_H)
  #include <fcntl.h>
#endif

#if defined(_WIN32)
  #include <io.h>

  #if !defined(_fileno)
    #define _fileno(__F) ((__F)->_file)
  #endif
#endif

#include <osmscout/system/Assert.h>
#include <osmscout/system/Compiler.h>

#include <osmscout/util/Exception.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

namespace osmscout {

  FileScanner::FileScanner()
   : file(NULL),
     hasError(true),
     buffer(NULL),
     size(0),
     offset(0),
     byteBuffer(NULL),
     byteBufferSize(0)
#if defined(_WIN32)
     ,mmfHandle((HANDLE)0)
#endif
  {
    // no code
  }

  FileScanner::~FileScanner()
  {
    if (IsOpen()) {
      log.Warn() << "Automatically closing FileScanner for file '" << filename << "'!";
      CloseFailsafe();
    }

    delete [] byteBuffer;
  }

  void FileScanner::AssureByteBufferSize(size_t size)
  {
    if (byteBufferSize>=size) {
      return;
    }

    delete [] byteBuffer;

    byteBuffer=new uint8_t[size];
    byteBufferSize=size;

  }

  void FileScanner::FreeBuffer()
  {
#if defined(HAVE_MMAP)
    if (buffer!=NULL) {
      if (munmap(buffer,size)!=0) {
        log.Error() << "Error while calling munmap: "<< strerror(errno) << " for file '" << filename << "'";
      }

      buffer=NULL;
    }
#elif  defined(_WIN32)
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

  void FileScanner::Open(const std::string& filename,
                         Mode mode,
                         bool useMmap)
  {
    if (file!=NULL) {
      throw IOException(filename,"Error opening file for reading","File already opened");
    }

    hasError=true;
    this->filename=filename;

    file=fopen(filename.c_str(),"rb");

    if (file==NULL) {
      throw IOException(filename,"Cannot open file for reading");
    }

#if defined(HAVE_FSEEKO)
    off_t size;

    if (fseeko(file,0L,SEEK_END)!=0) {
      throw IOException(filename,"Cannot seek to end of file");
    }

    size=ftello(file);

    if (size==-1) {
      throw IOException(filename,"Cannot get size of file");
    }

    this->size=(FileOffset)size;

    if (fseeko(file,0L,SEEK_SET)!=0) {
      throw IOException(filename,"Cannot seek to start of file");
    }
#elif defined(HAVE__FSEEKI64) && defined(HAVE__FTELLI64)
    __int64 size;

    if (_fseeki64(file,0L,SEEK_END)!=0) {
      throw IOException(filename,"Cannot seek to end of file");
    }

    size=_ftelli64(file);

    if (size==-1) {
      throw IOException(filename,"Cannot get size of file");
    }

    this->size=(FileOffset)size;

    if (_fseeki64(file,0L,SEEK_SET)!=0) {
      throw IOException(filename,"Cannot seek to start of file");
    }
#else
    long size;

    if (fseek(file,0L,SEEK_END)!=0) {
      log.Error() << "Cannot seek to end of file '" << filename << "' (" << strerror(errno) << ")";
      throw IOException(filename,"Cannot seek to end of file");
    }

    size=ftell(file);

    if (size==-1) {
      log.Error() << "Cannot get size of file '" << filename << "' (" << strerror(errno) << ")";
      throw IOException(filename,"Cannot get size of file");
    }

    this->size=(FileOffset)size;

    if (fseek(file,0L,SEEK_SET)!=0) {
      log.Error() << "Cannot seek to start of file '" << filename << "' (" << strerror(errno) << ")";
      throw IOException(filename,"Cannot seek to start of file");
    }
#endif

#if defined(HAVE_POSIX_FADVISE)
    if (mode==FastRandom) {
      if (posix_fadvise(fileno(file),0,size,POSIX_FADV_WILLNEED)<0) {
        log.Error() << "Cannot set file access advice for file '" << filename << "' (" << strerror(errno) << ")";
      }
    }
    else if (mode==Sequential) {
      if (posix_fadvise(fileno(file),0,size,POSIX_FADV_SEQUENTIAL)<0) {
        log.Error() << "Cannot set file access advice for file '" << filename << "' (" << strerror(errno) << ")";
      }
    }
    else if (mode==LowMemRandom) {
      if (posix_fadvise(fileno(file),0,size,POSIX_FADV_RANDOM)<0) {
        log.Error() << "Cannot set file access advice for file '" << filename << "' (" << strerror(errno) << ")";
      }
    }
#endif

#if defined(HAVE_MMAP)
    if (useMmap && this->size>0) {
      FreeBuffer();

      buffer=(char*)mmap(NULL,(size_t)size,PROT_READ,MAP_PRIVATE,fileno(file),0);
      if (buffer!=MAP_FAILED) {
        offset=0;
#if defined(HAVE_POSIX_MADVISE)
        if (mode==FastRandom) {
          if (posix_madvise(buffer,(size_t)size,POSIX_MADV_WILLNEED)<0) {
            log.Error() << "Cannot set mmaped file access advice for file '" << filename << "' (" << strerror(errno) << ")";
          }
        }
        else if (mode==Sequential) {
          if (posix_madvise(buffer,(size_t)size,POSIX_MADV_SEQUENTIAL)<0) {
            log.Error() << "Cannot set mmaped file access advice for file '" << filename << "' (" << strerror(errno) << ")";
          }
        }
        else if (mode==LowMemRandom) {
          if (posix_madvise(buffer,(size_t)size,POSIX_MADV_RANDOM)<0) {
            log.Error() << "Cannot set mmaped file access advice for file '" << filename << "' (" << strerror(errno) << ")";
          }
        }
#endif
      }
      else {
        log.Error() << "Cannot mmap file '" << filename << "' of size " << size << " (" << strerror(errno) << ")";
        buffer=NULL;
      }
    }
#elif  defined(_WIN32)
    unused(mode);
    if (useMmap && this->size>0) {
      FreeBuffer();

      mmfHandle=CreateFileMapping((HANDLE)_get_osfhandle(fileno(file)),
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
          log.Error() << "Cannot map view for file '" << filename << "' of size " << size << " (" << GetLastError() << ")";
        }
      }
      else {
        log.Error() << "Cannot create file mapping for file '" << filename << "' of size " << size << " (" << GetLastError() << ")";
      }
    }
#endif

    hasError=false;
  }

  /**
   * Closes the file.
   *
   * If the file was never opened or was already closed an exception is thrown.
   *
   * If closing the file fails, an exception is thrown
   */
  void FileScanner::Close()
  {
    if (file==NULL) {
      throw IOException(filename,"Cannot close file","File already closed");
    }

    FreeBuffer();

    if (fclose(file)!=0) {
      file=NULL;
      throw IOException(filename,"Cannot close file");
    }

    file=NULL;
  }

  /**
   * Closes the file. Does not throw any exceptions even if an error occurs.
   *
   * Use this variant of Close() in cases where the file already run into errors and you just
   * want to clean up the resources using best effort.
   *
   */
  void FileScanner::CloseFailsafe()
  {
    if (file==NULL) {
      return;
    }

    FreeBuffer();

    fclose(file);

    file=NULL;
  }

  bool FileScanner::IsEOF() const
  {
    if (HasError()) {
      return true;
    }

    if (size==0) {
      return true;
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      return offset>=size;
    }
#endif

    return feof(file)!=0;
  }

  std::string FileScanner::GetFilename() const
  {
    return filename;
  }

  /**
   * Moves the reading cursor to the start of the file (offset 0)
   *
   * throws IOException on error
   */
  void FileScanner::GotoBegin()
  {
    SetPos(0);
  }

  /**
   * Moves the reading cursor to the given file position
   *
   * throws IOException on error
   */
  void FileScanner::SetPos(FileOffset pos)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot set position in file","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (pos>=size) {
        hasError=true;
        throw IOException(filename,"Cannot set position in file to "+std::to_string(pos),"Position beyond file end");
      }

      offset=pos;

      return;
    }
#endif

    clearerr(file);

#if defined(HAVE_FSEEKO)
    hasError=fseeko(file,(off_t)pos,SEEK_SET)!=0;
#elif defined(HAVE__FSEEKi64)
    hasError=_fseeki64(file,(__int64)pos,SEEK_SET)!=0;
#else
    hasError=fseek(file,(long)pos,SEEK_SET)!=0;
#endif

    if (hasError) {
      throw IOException(filename,"Cannot set position in file");
    }
  }

  /**
   * Returns the current position of the reading cursor in relation to the begining of the file
   *
   * throws IOException on error
   */
  FileOffset FileScanner::GetPos() const
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read position in file","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      return offset;
    }
#endif

#if defined(HAVE_FSEEKO)
    off_t filepos=ftello(file);

    if (filepos==-1) {
      hasError=true;
      throw IOException(filename,"Cannot set position in file");
    }

    return (FileOffset)filepos;
#elif defined(HAVE__FTELLI64)
    __int64 filepos=_ftelli64(file);

    if (filepos==-1) {
      hasError=true;
      throw IOException(filename,"Cannot set position in file");
    }

    return (FileOffset)filepos;
#else
    long filepos=ftell(file);

    if (filepos==-1) {
      hasError=true;
      throw IOException(filename,"Cannot set position in file");
    }

    return (FileOffset)filepos;
#endif
  }

  void FileScanner::Read(char* buffer, size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read byte array","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (this->buffer!=NULL) {
      if (offset+(FileOffset)bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read byte array","Cannot read beyond end of file");
      }

      memcpy(buffer,&this->buffer[offset],bytes);

      offset+=bytes;

      return;
    }
#endif

    hasError=fread(buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read byte array");
    }
  }

  void FileScanner::Read(std::string& value)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read string","File already in error state");
    }

    value.clear();

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read string","Cannot read beyond end of file");
      }

      FileOffset start=offset;

      while (offset<size &&
             buffer[offset]!='\0') {
        offset++;
      }

      value.assign(&buffer[start],offset-start);

      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read string","String has no terminating '\\0' before end of file");
      }

      offset++;

      return;
    }
#endif

    char character;

    hasError=fread(&character,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read string");
    }

    while (character!='\0') {
      value.append(1,character);

      hasError=fread(&character,1,1,file)!=1;

      if (hasError) {
        throw IOException(filename,"Cannot read string");
      }
    }
  }

  void FileScanner::Read(bool& boolean)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read bool","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read bool","Cannot read beyond end of file");
      }

      boolean=buffer[offset]!=0;

      offset++;

      return;
    }
#endif

    char value;

    hasError=fread(&value,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read bool");
    }

    boolean=value!=0;
  }

  void FileScanner::Read(int8_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int8_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int8_t","Cannot read beyond end of file");
      }

      number=(int8_t)buffer[offset];

      offset++;

      return;
    }
#endif

    hasError=fread(&number,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read int8_t");
    }
  }

  void FileScanner::Read(int16_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int16_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+2-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int16_t","Cannot read beyond end of file");
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

      offset+=2;

      return;
    }
#endif

    unsigned char buffer[2];

    hasError=fread(&buffer,1,2,file)!=2;

    if (hasError) {
      throw IOException(filename,"Cannot read int16_t");
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
  }

  void FileScanner::Read(int32_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int32_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+4-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int32_t","Cannot read beyond end of file");
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

      offset+=4;

      return;
    }
#endif

    unsigned char buffer[4];

    hasError=fread(&buffer,1,4,file)!=4;

    if (hasError) {
      throw IOException(filename,"Cannot read int32_t");
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
  }

  void FileScanner::Read(int64_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int64_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int64_t","Cannot read beyond end of file");
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

      offset+=8;

      return;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      throw IOException(filename,"Cannot read int64_t");
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
  }

  void FileScanner::Read(uint8_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint8_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint8_t","Cannot read beyond end of file");
      }

      number=(uint8_t)buffer[offset];

      offset++;

      return;
    }
#endif

    hasError=fread(&number,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read uint8_t");
    }
  }

  void FileScanner::Read(uint16_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint16_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+2-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint16_t","Cannot read beyond end of file");
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

      offset+=2;

      return;
    }
#endif

    unsigned char buffer[2];

    hasError=fread(&buffer,1,2,file)!=2;

    if (hasError) {
      throw IOException(filename,"Cannot read uint16_t");
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
  }

  void FileScanner::Read(uint32_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint32_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+4-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint32_t","Cannot read beyond end of file");
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

      offset+=4;

      return;
    }
#endif

    unsigned char buffer[4];

    hasError=fread(&buffer,1,4,file)!=4;

    if (hasError) {
      throw IOException(filename,"Cannot read uint32_t");
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
  }

  void FileScanner::Read(uint64_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint64_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint64_t","Cannot read beyond end of file");
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

      offset+=8;

      return;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      throw IOException(filename,"Cannot read uint64_t");
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
  }

  void FileScanner::Read(uint16_t& number,
                         size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read size limited uint16_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read size limited uint16_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&buffer[offset];
      uint16_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      if (bytes>=2) {
        add=(unsigned char)(*dataPtr);
        add=add << 8;
        number|=add;
      }

      offset+=bytes;

      return;
    }
#endif

    unsigned char buffer[2];

    hasError=fread(&buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read size limited uint16_t");
    }

    unsigned char *dataPtr=buffer;
    uint16_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    if (bytes>=2) {
      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
    }
  }

  void FileScanner::Read(uint32_t& number,
                         size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read size limited uint32_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read size limited uint32_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&buffer[offset];
      uint32_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      if (bytes>=2) {
        add=(unsigned char)(*dataPtr);
        add=add << 8;
        number|=add;
        dataPtr++;

        if (bytes>=3) {
          add=(unsigned char)(*dataPtr);
          add=add << 16;
          number|=add;
          dataPtr++;

          if (bytes>=4) {
            add=(unsigned char)(*dataPtr);
            add=add << 24;
            number|=add;
          }
        }
      }

      offset+=bytes;

      return;
    }
#endif

    unsigned char buffer[4];

    hasError=fread(&buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read size limited uint32_t");
    }

    unsigned char *dataPtr=buffer;
    uint32_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    if (bytes>=2) {
      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      if (bytes>=3) {
        add=(unsigned char)(*dataPtr);
        add=add << 16;
        number|=add;
        dataPtr++;

        if (bytes>=4) {
          add=(unsigned char)(*dataPtr);
          add=add << 24;
          number|=add;
        }
      }
    }
  }

  void FileScanner::Read(uint64_t& number,
                         size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read size limited uint64_t","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read size limited uint64_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&buffer[offset];
      uint64_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      if (bytes>=2) {
        add=(unsigned char)(*dataPtr);
        add=add << 8;
        number|=add;
        dataPtr++;

        if (bytes>=3) {
          add=(unsigned char)(*dataPtr);
          add=add << 16;
          number|=add;
          dataPtr++;

          if (bytes>=4) {
            add=(unsigned char)(*dataPtr);
            add=add << 24;
            number|=add;
            dataPtr++;

            if (bytes>=5) {
              add=(unsigned char)(*dataPtr);
              add=add << 32;
              number|=add;
              dataPtr++;

              if (bytes>=6) {
                add=(unsigned char)(*dataPtr);
                add=add << 40;
                number|=add;
                dataPtr++;

                if (bytes>=7) {
                  add=(unsigned char)(*dataPtr);
                  add=add << 48;
                  number|=add;
                  dataPtr++;

                  if (bytes>=8) {
                    add=(unsigned char)(*dataPtr);
                    add=add << 56;
                    number|=add;
                  }
                }
              }
            }
          }
        }
      }

      offset+=bytes;

      return;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read size limited uint64_t");
    }

    unsigned char *dataPtr=buffer;
    uint64_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    if (bytes>=2) {
      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;
      dataPtr++;

      if (bytes>=3) {
        add=(unsigned char)(*dataPtr);
        add=add << 16;
        number|=add;
        dataPtr++;

        if (bytes>=4) {
          add=(unsigned char)(*dataPtr);
          add=add << 24;
          number|=add;
          dataPtr++;

          if (bytes>=5) {
            add=(unsigned char)(*dataPtr);
            add=add << 32;
            number|=add;
            dataPtr++;

            if (bytes>=6) {
              add=(unsigned char)(*dataPtr);
              add=add << 40;
              number|=add;
              dataPtr++;

              if (bytes>=7) {
                add=(unsigned char)(*dataPtr);
                add=add << 48;
                number|=add;
                dataPtr++;

                if (bytes>=8) {
                  add=(unsigned char)(*dataPtr);
                  add=add << 56;
                  number|=add;
                }
              }
            }
          }
        }
      }
    }
  }

  void FileScanner::Read(ObjectFileRef& ref)
  {
    uint8_t    typeByte;
    FileOffset fileOffset;

    Read(typeByte);
    ReadFileOffset(fileOffset);

    ref.Set(fileOffset,(RefType)typeByte);
  }

  void FileScanner::ReadFileOffset(FileOffset& fileOffset)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read file offset","File already in error state");
    }

    fileOffset=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+8-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read file offset","Cannot read beyond end of file");
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

      offset+=8;

      return;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,8,file)!=8;

    if (hasError) {
      throw IOException(filename,"Cannot read file offset");
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
  }

  void FileScanner::ReadFileOffset(FileOffset& fileOffset,
                                   size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read file offset","File already in error state");
    }

    assert(bytes>0 && bytes<=8);

    fileOffset=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read file offset","Cannot read beyond end of file");
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
                  }
                }
              }
            }
          }
        }
      }

      offset+=bytes;

      return;
    }
#endif

    unsigned char buffer[8];

    hasError=fread(&buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read file offset");
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
                }
              }
            }
          }
        }
      }
    }
  }

  void FileScanner::ReadNumber(int16_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int16_t number","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int16_t number","Cannot read beyond end of file");
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read int16_t number");
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
          hasError=true;
          throw IOException(filename,"Cannot read int16_t number");
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
          hasError=true;
          throw IOException(filename,"Cannot read int16_t number");
        }

        number|=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number|=static_cast<num_t>(val) << shift;
    }
  }

  void FileScanner::ReadNumber(int32_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int32_t number","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int32_t number","Cannot read beyond end of file");
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read int32_t number");
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
          hasError=true;
          throw IOException(filename,"Cannot read int32_t number");
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
          hasError=true;
          throw IOException(filename,"Cannot read int32_t number");
        }

        number|=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number|=static_cast<num_t>(val) << shift;
    }
  }

  void FileScanner::ReadNumber(int64_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int64_t number","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int64_t number","Cannot read beyond end of file");
      }

      unsigned int bytes=DecodeNumber(&buffer[offset],number);

      offset+=bytes;

      return;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read int64_t number");
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
          hasError=true;
          throw IOException(filename,"Cannot read int64_t number");
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
          hasError=true;
          throw IOException(filename,"Cannot read int64_t number");
        }

        number|=(val << shift);
        val=buffer & 0x7f;
        shift=nextShift;
        nextShift+=7;
      }

      number|=static_cast<num_t>(val) << shift;
    }
  }

  void FileScanner::ReadNumber(uint16_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint16_t number","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      unsigned int shift=0;

      for (; offset<size; offset++) {
        number|=static_cast<uint16_t>(buffer[offset] & 0x7f) << shift;

        if ((buffer[offset] & 0x80)==0) {
          offset++;
          return;
        }

        shift+=7;
      }

      hasError=true;
      throw IOException(filename,"Cannot read uint16_t number","Cannot read beyond end of file");
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read uint16_t number");
    }

    unsigned int shift=0;

    while (true) {
      number|=static_cast<uint16_t>(buffer & 0x7f) << shift;

      if ((buffer & 0x80)==0) {
        return;
      }

      if (fread(&buffer,1,1,file)!=1) {
        hasError=true;
        throw IOException(filename,"Cannot read uint16_t number");
      }

      shift+=7;
    }
  }

  void FileScanner::ReadNumber(uint32_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint32_t number","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      unsigned int shift=0;

      for (; offset<size; offset++) {
        number|=static_cast<uint32_t>(buffer[offset] & 0x7f) << shift;

        if ((buffer[offset] & 0x80)==0) {
          offset++;
          return;
        }

        shift+=7;
      }

      hasError=true;
      throw IOException(filename,"Cannot read uint32_t number","Cannot read beyond end of file");
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read uint32_t number");
    }

    unsigned int shift=0;

    while (true) {
      number|=static_cast<uint32_t>(buffer & 0x7f) << shift;

      if ((buffer & 0x80)==0) {
        return;
      }

      if (fread(&buffer,1,1,file)!=1) {
        hasError=true;
        throw IOException(filename,"Cannot read uint32_t number");
      }

      shift+=7;
    }
  }

  void FileScanner::ReadNumber(uint64_t& number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint64_t number","File already in error state");
    }

    number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      unsigned int shift=0;

      for (; offset<size; offset++) {
        number|=static_cast<uint64_t>(buffer[offset] & 0x7f) << shift;

        if ((buffer[offset] & 0x80)==0) {
          offset++;
          return;
        }

        shift+=7;
      }

      hasError=true;
      throw IOException(filename,"Cannot read uint64_t number","Cannot read beyond end of file");
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read uint64_t number");
    }

    unsigned int shift=0;

    while (true) {
      number|=static_cast<uint64_t>(buffer & 0x7f) << shift;

      if ((buffer & 0x80)==0) {
        return;
      }

      if (fread(&buffer,1,1,file)!=1) {
        hasError=true;
        throw IOException(filename,"Cannot read uint64_t number");
      }

      shift+=7;
    }
  }

  void FileScanner::ReadCoord(GeoCoord& coord)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read coordinate","File already in error state");
    }

    uint32_t latDat;
    uint32_t lonDat;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+coordByteSize-1>=size) {
        hasError=true;

        throw IOException(filename,"Cannot read coordinate","Cannot read beyonf end of file");
      }

      char *dataPtr=&buffer[offset];

      latDat=  ((unsigned char) dataPtr[0] <<  0)
             | ((unsigned char) dataPtr[1] <<  8)
             | ((unsigned char) dataPtr[2] << 16)
             | ((unsigned char) (dataPtr[6] & 0x0f) << 24);

      lonDat=  ((unsigned char) dataPtr[3] <<  0)
             | ((unsigned char) dataPtr[4] <<  8)
             | ((unsigned char) dataPtr[5] << 16)
             | ((unsigned char) (dataPtr[6] & 0xf0) << 20);

      offset+=coordByteSize;

      coord.Set(latDat/latConversionFactor-90.0,
                lonDat/lonConversionFactor-180.0);

      return;
    }
#endif

    unsigned char buffer[coordByteSize];

    hasError=fread(&buffer,1,coordByteSize,file)!=coordByteSize;

    if (hasError) {
      throw IOException(filename,"Cannot read coordinate");
    }

    latDat=  (buffer[0] <<  0)
           | (buffer[1] <<  8)
           | (buffer[2] << 16)
           | ((buffer[6] & 0x0f) << 24);

    lonDat=  (buffer[3] <<  0)
           | (buffer[4] <<  8)
           | (buffer[5] << 16)
           | ((buffer[6] & 0xf0) << 20);

    coord.Set(latDat/latConversionFactor-90.0,
              lonDat/lonConversionFactor-180.0);
  }

  void FileScanner::ReadConditionalCoord(GeoCoord& coord,
                                         bool& isSet)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read coordinate","File already in error state");
    }

    uint32_t latDat;
    uint32_t lonDat;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (buffer!=NULL) {
      if (offset+coordByteSize-1>=size) {
        hasError=true;

        throw IOException(filename,"Cannot read coordinate","Cannot read beyond end of file");
      }

      char *dataPtr=&buffer[offset];

      latDat=  ((unsigned char) dataPtr[0] <<  0)
             | ((unsigned char) dataPtr[1] <<  8)
             | ((unsigned char) dataPtr[2] << 16)
             | ((unsigned char) (dataPtr[6] & 0x0f) << 24);

      lonDat=  ((unsigned char) dataPtr[3] <<  0)
             | ((unsigned char) dataPtr[4] <<  8)
             | ((unsigned char) dataPtr[5] << 16)
             | ((unsigned char) (dataPtr[6] & 0xf0) << 20);

      offset+=coordByteSize;

      if (latDat==0xfffffff &&
          lonDat==0xfffffff) {
        isSet=false;
      }
      else  {
        coord.Set(latDat/latConversionFactor-90.0,
                  lonDat/lonConversionFactor-180.0);
        isSet=true;
      }

      return;
    }
#endif

    unsigned char buffer[coordByteSize];

    hasError=fread(&buffer,1,coordByteSize,file)!=coordByteSize;

    if (hasError) {
      throw IOException(filename,"Cannot read coordinate");
    }

    latDat=  (buffer[0] <<  0)
           | (buffer[1] <<  8)
           | (buffer[2] << 16)
           | ((buffer[6] & 0x0f) << 24);

    lonDat=  (buffer[3] <<  0)
           | (buffer[4] <<  8)
           | (buffer[5] << 16)
           | ((buffer[6] & 0xf0) << 20);

    if (latDat==0xfffffff &&
        lonDat==0xfffffff) {
      isSet=false;
    }
    else  {
      coord.Set(latDat/latConversionFactor-90.0,
                lonDat/lonConversionFactor-180.0);
      isSet=true;
    }
  }

  void FileScanner::Read(std::vector<Point>& nodes,bool readIds)
  {
    size_t  coordBitSize;
    uint8_t sizeByte;

    Read(sizeByte);

    // Fast exit for empty arrays
    if (sizeByte==0) {
      return;
    }

    bool   hasNodes;
    size_t nodeCount;

    if (readIds) {
      hasNodes=(sizeByte & 0x04)!=0;

      if ((sizeByte & 0x03) == 0) {
        coordBitSize=16;
      }
      else if ((sizeByte & 0x03) == 1) {
        coordBitSize=32;
      }
      else {
        coordBitSize=48;
      }

      nodeCount=(sizeByte & 0x78) >> 3;

      if ((sizeByte & 0x80) != 0) {
        Read(sizeByte);

        nodeCount|=(sizeByte & 0x7f) << 4;

        if ((sizeByte & 0x80) != 0) {
          Read(sizeByte);

          nodeCount|=(sizeByte & 0x7f) << 11;

          if ((sizeByte & 0x80) != 0) {
             Read(sizeByte);

             nodeCount|=sizeByte << 18;
          }
        }
      }
    }
    else {
      hasNodes=false;

      if ((sizeByte & 0x03) == 0) {
        coordBitSize=16;
      }
      else if ((sizeByte & 0x03) == 1) {
        coordBitSize=32;
      }
      else {
        coordBitSize=48;
      }

      nodeCount=(sizeByte & 0x7c) >> 2;

      if ((sizeByte & 0x80) != 0) {
        Read(sizeByte);

        nodeCount|=(sizeByte & 0x7f) << 5;

        if ((sizeByte & 0x80) != 0) {
          Read(sizeByte);

          nodeCount|=(sizeByte & 0x7f) << 12;

          if ((sizeByte & 0x80) != 0) {
            Read(sizeByte);

            nodeCount|=sizeByte << 19;
          }
        }
      }
    }

    nodes.resize(nodeCount);

    size_t byteBufferSize=(nodeCount-1)*coordBitSize/8;

    AssureByteBufferSize(byteBufferSize);

    GeoCoord firstCoord;

    ReadCoord(firstCoord);

    nodes[0].SetCoord(firstCoord);

    uint32_t latValue=(uint32_t)round((nodes[0].GetLat()+90.0)*latConversionFactor);
    uint32_t lonValue=(uint32_t)round((nodes[0].GetLon()+180.0)*lonConversionFactor);

    Read((char*)byteBuffer,byteBufferSize);

    if (coordBitSize==16) {
      size_t currentCoordPos=1;

      for (size_t i=0; i<byteBufferSize; i+=2) {
        int32_t latDelta=(int8_t)byteBuffer[i];
        int32_t lonDelta=(int8_t)byteBuffer[i+1];

        latValue+=latDelta;
        lonValue+=lonDelta;

        nodes[currentCoordPos].SetCoord(GeoCoord(latValue/latConversionFactor-90.0,
                                                 lonValue/lonConversionFactor-180.0));

        currentCoordPos++;
      }
    }
    else if (coordBitSize==32) {
      size_t currentCoordPos=1;

      for (size_t i=0; i<byteBufferSize; i+=4) {
        uint32_t latUDelta=byteBuffer[i+0] | (byteBuffer[i+1]<<8);
        uint32_t lonUDelta=byteBuffer[i+2] | (byteBuffer[i+3]<<8);
        int32_t  latDelta;
        int32_t  lonDelta;

        if (latUDelta & 0x8000) {
          latDelta=(int32_t)(latUDelta | 0xffff0000);
        }
        else {
          latDelta=(int32_t)latUDelta;
        }

        latValue+=latDelta;

        if (lonUDelta & 0x8000) {
          lonDelta=(int32_t)(lonUDelta | 0xffff0000);
        }
        else {
          lonDelta=(int32_t)lonUDelta;
        }

        lonValue+=lonDelta;

        nodes[currentCoordPos].SetCoord(GeoCoord(latValue/latConversionFactor-90.0,
                                                 lonValue/lonConversionFactor-180.0));
        currentCoordPos++;
      }
    }
    else {
      size_t currentCoordPos=1;

      for (size_t i=0; i<byteBufferSize; i+=6) {
        uint32_t latUDelta=(byteBuffer[i+0]) | (byteBuffer[i+1]<<8) | (byteBuffer[i+2]<<16);
        uint32_t lonUDelta=(byteBuffer[i+3]) | (byteBuffer[i+4]<<8) | (byteBuffer[i+5]<<16);
        int32_t  latDelta;
        int32_t  lonDelta;

        if (latUDelta & 0x800000) {
          latDelta=(int32_t)(latUDelta | 0xff000000);
        }
        else {
          latDelta=(int32_t)latUDelta;
        }

        latValue+=latDelta;

        if (lonUDelta & 0x800000) {
          lonDelta=(int32_t)(lonUDelta | 0xff000000);
        }
        else {
          lonDelta=(int32_t)lonUDelta;
        }

        lonValue+=lonDelta;

        nodes[currentCoordPos].SetCoord(GeoCoord(latValue/latConversionFactor-90.0,
                                                 lonValue/lonConversionFactor-180.0));

        currentCoordPos++;
      }
    }

    if (hasNodes) {
      size_t idCurrent=0;

      while (idCurrent<nodeCount) {
        uint8_t bitset;
        size_t bitmask=1;

        Read(bitset);

        for (size_t i=0; i<8 && idCurrent<nodeCount; i++) {
          if (bitset & bitmask) {
            uint8_t serial;

            Read(serial);

            nodes[idCurrent].SetSerial(serial);
          }

          bitmask*=2;
          idCurrent++;
        }
      }

      /*
      uint8_t serial;

      for (size_t i=0; i<nodes.size(); i++) {
        Read(serial);

        nodes[i].SetSerial(serial);
      }*/
    }
  }

  void FileScanner::ReadBox(GeoBox& box)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read geo box","File already in error state");
    }

    GeoCoord minCoord;
    GeoCoord maxCoord;

    ReadCoord(minCoord);
    ReadCoord(maxCoord);

    box.Set(minCoord,
            maxCoord);
  }

  void FileScanner::ReadTypeId(TypeId& id,
                               uint8_t maxBytes)
  {
    if (maxBytes==1) {
      uint8_t byteValue;

      Read(byteValue);

      id=byteValue;
    }
    else if (maxBytes==2) {
      uint8_t byteValue;

      Read(byteValue);

      id=byteValue *256;

      Read(byteValue);

      id+=byteValue;
    }
    else {
      assert(false);
    }
  }

  ObjectFileRefStreamReader::ObjectFileRefStreamReader(FileScanner& reader)
  : reader(reader),
    lastFileOffset(0)
  {
    // no code
  }

  void ObjectFileRefStreamReader::Reset()
  {
    lastFileOffset=0;
  }

  void ObjectFileRefStreamReader::Read(ObjectFileRef& ref)
  {
    RefType    type;
    FileOffset offset;

    reader.ReadNumber(offset);

    type=(RefType)(offset%4);

    offset=offset >> 2;
    offset=offset+lastFileOffset;

    ref.Set(offset,type);

    lastFileOffset=offset;
  }
}
