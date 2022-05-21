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

#include <cerrno>
#include <cstdio>
#include <cstring>

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

#include <osmscout/util/Exception.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Number.h>
#include <osmscout/util/String.h>

namespace osmscout {

  FileScanner::FileScanner()
   : file(nullptr),
     hasError(true),
     mmap(nullptr),
     size(0),
     offset(0),
     byteBuffer(nullptr),
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
    if (mmap!=nullptr) {
      if (munmap(mmap,size)!=0) {
        log.Error() << "Error while calling munmap: "<< strerror(errno) << " for file '" << filename << "'";
      }

      mmap=nullptr;
    }
#elif  defined(_WIN32)
      if (mmap!=nullptr) {
        UnmapViewOfFile(mmap);
        mmap=nullptr;
      }
      if (mmfHandle!=nullptr) {
        CloseHandle(mmfHandle);
        mmfHandle=nullptr;
      }
#endif
  }

  void FileScanner::Open(const std::string& filename,
                         [[maybe_unused]] Mode mode,
                         bool useMmap)
  {
    if (file!=nullptr) {
      throw IOException(filename,"Error opening file for reading","File already opened");
    }

    hasError=true;
    this->filename=filename;

    file=fopen(filename.c_str(),"rb");

    if (file==nullptr) {
      throw IOException(filename,"Cannot open file for reading");
    }

#if defined(HAVE_FSEEKO)
    off_t fileSize;

    if (fseeko(file,0L,SEEK_END)!=0) {
      throw IOException(filename,"Cannot seek to end of file");
    }

    fileSize=ftello(file);

    if (fileSize==-1) {
      throw IOException(filename,"Cannot get size of file");
    }

    this->size=(FileOffset)fileSize;

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
      if (posix_fadvise(fileno(file),0,fileSize,POSIX_FADV_WILLNEED)!=0) {
        log.Error() << "Cannot set file access advice for file '" << filename << "' (" << strerror(errno) << ")";
      }
    }
    else if (mode==Sequential) {
      if (posix_fadvise(fileno(file),0,fileSize,POSIX_FADV_SEQUENTIAL)!=0) {
        log.Error() << "Cannot set file access advice for file '" << filename << "' (" << strerror(errno) << ")";
      }
    }
    else if (mode==LowMemRandom) {
      if (posix_fadvise(fileno(file),0,fileSize,POSIX_FADV_RANDOM)!=0) {
        log.Error() << "Cannot set file access advice for file '" << filename << "' (" << strerror(errno) << ")";
      }
    }
#endif

#if defined(HAVE_MMAP)
    if (useMmap && this->size>0) {
      FreeBuffer();

      mmap=(char*)::mmap(nullptr,(size_t)fileSize,PROT_READ,MAP_PRIVATE,fileno(file),0);
      if (mmap!=MAP_FAILED) {
        offset=0;
#if defined(HAVE_POSIX_MADVISE)
        if (mode==FastRandom) {
          if (posix_madvise(mmap,(size_t)fileSize,POSIX_MADV_WILLNEED)!=0) {
            log.Error() << "Cannot set mmaped file access advice for file '" << filename << "' (" << strerror(errno) << ")";
          }
        }
        else if (mode==Sequential) {
          if (posix_madvise(mmap,(size_t)fileSize,POSIX_MADV_SEQUENTIAL)!=0) {
            log.Error() << "Cannot set mmaped file access advice for file '" << filename << "' (" << strerror(errno) << ")";
          }
        }
        else if (mode==LowMemRandom) {
          if (posix_madvise(mmap,(size_t)fileSize,POSIX_MADV_RANDOM)!=0) {
            log.Error() << "Cannot set mmaped file access advice for file '" << filename << "' (" << strerror(errno) << ")";
          }
        }
#endif
      }
      else {
        log.Error() << "Cannot mmap file '" << filename << "' of size " << fileSize << " (" << strerror(errno) << ")";
        mmap=nullptr;
      }
    }
#elif  defined(_WIN32)
    if (useMmap && this->size>0) {
      FreeBuffer();

      mmfHandle=CreateFileMapping((HANDLE)_get_osfhandle(fileno(file)),
                                  (LPSECURITY_ATTRIBUTES)nullptr,
                                  PAGE_READONLY,
                                  0,0,
                                  (LPCTSTR)nullptr);

      if (mmfHandle!=nullptr) {
        mmap=(char*)MapViewOfFile(mmfHandle,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  0);

        if (mmap!=nullptr) {
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
    if (file==nullptr) {
      throw IOException(filename,"Cannot close file","File already closed");
    }

    FreeBuffer();

    if (fclose(file)!=0) {
      file=nullptr;
      throw IOException(filename,"Cannot close file");
    }

    file=nullptr;
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
    if (file==nullptr) {
      return;
    }

    FreeBuffer();

    fclose(file);

    file=nullptr;
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
    if (mmap!=nullptr) {
      return offset>=size;
    }
#endif

    return feof(file)!=0;
  }

  std::string FileScanner::GetFilename() const
  {
    if (file==nullptr && filename.empty()) {
      throw IOException(filename,"Cannot get filename of file","File was not opened");
    }

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
    if (mmap!=nullptr) {
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
#elif defined(HAVE__FSEEKI64)
    hasError=_fseeki64(file,(__int64)pos,SEEK_SET)!=0;
#else
    assert(pos <= (FileOffset)std::numeric_limits<long>::max());
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
    if (mmap!=nullptr) {
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

  char* FileScanner::ReadInternal(size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read byte array","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (this->mmap!=nullptr) {
      if (offset+(FileOffset)bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read byte array","Cannot read beyond end of file");
      }

      char *res = &this->mmap[offset];

      offset+=bytes;

      return res;
    }
#endif

    AssureByteBufferSize(bytes);
    hasError=fread(byteBuffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read byte array");
    }
    return (char*)byteBuffer;
  }

  void FileScanner::Read(char* buffer, size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read byte array","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (this->mmap!=nullptr) {
      if (offset+(FileOffset)bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read byte array","Cannot read beyond end of file");
      }

      memcpy(buffer,&this->mmap[offset],bytes);

      offset+=bytes;

      return;
    }
#endif

    hasError=fread(buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read byte array");
    }
  }

  std::string FileScanner::ReadString()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read string","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read string","Cannot read beyond end of file");
      }

      FileOffset start=offset;

      while (offset<size &&
             mmap[offset]!='\0') {
        offset++;
      }


      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read string","String has no terminating '\\0' before end of file");
      }

      std::string value(&mmap[start],offset-start);

      offset++;

      return value;
    }
#endif

    std::string value;
    char        character;

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

    return value;
  }

  bool FileScanner::ReadBool()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read bool","File already in error state");
    }

    char value;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read bool","Cannot read beyond end of file");
      }

      value=mmap[offset];
      offset++;

      return ConvertBool(value);
    }
#endif

    hasError=fread(&value,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read bool");
    }

    return ConvertBool(value);
  }

  std::byte FileScanner::ReadByte()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read byte","File already in error state");
    }

    std::byte value;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read byte","Cannot read beyond end of file");
      }

      value=std::byte(mmap[offset]);
      offset++;

      return value;
    }
#endif

    hasError=fread(&value,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read byte");
    }

    return value;
  }

  int8_t FileScanner::ReadInt8()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int8_t","File already in error state");
    }

    int8_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int8_t","Cannot read beyond end of file");
      }

      number=(int8_t)mmap[offset];

      offset++;

      return number;
    }
#endif

    hasError=fread(&number,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read int8_t");
    }

    return number;
  }

  int16_t FileScanner::ReadInt16()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int16_t","File already in error state");
    }

    int16_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+2-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int16_t","Cannot read beyond end of file");
      }

      const char *dataPtr=&mmap[offset];
      int16_t    add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;

      offset+=2;

      return number;
    }
#endif

    std::array<unsigned char,2> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read int16_t");
    }

    const unsigned char *dataPtr=buffer.data();
    int16_t             add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;

    return number;
  }

  int32_t FileScanner::ReadInt32()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int32_t","File already in error state");
    }

    int32_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+4-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int32_t","Cannot read beyond end of file");
      }

      char    *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,4> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read int32_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  int64_t FileScanner::ReadInt64()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int64_t","File already in error state");
    }

    int64_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+8-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int64_t","Cannot read beyond end of file");
      }

      char    *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,8> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read int64_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  uint8_t FileScanner::ReadUInt8()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint8_t","File already in error state");
    }

    uint8_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint8_t","Cannot read beyond end of file");
      }

      number=(uint8_t)mmap[offset];

      offset++;

      return number;
    }
#endif

    hasError=fread(&number,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot read uint8_t");
    }

    return number;
  }

  uint16_t FileScanner::ReadUInt16()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint16_t","File already in error state");
    }

    uint16_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+2-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint16_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&mmap[offset];
      uint16_t add;

      add=(unsigned char)(*dataPtr);
      add=add << 0;
      number|=add;
      dataPtr++;

      add=(unsigned char)(*dataPtr);
      add=add << 8;
      number|=add;

      offset+=2;

      return number;
    }
#endif

    std::array<unsigned char,2> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read uint16_t");
    }

    unsigned char *dataPtr=buffer.data();
    uint16_t      add;

    add=(unsigned char)(*dataPtr);
    add=add << 0;
    number|=add;
    dataPtr++;

    add=(unsigned char)(*dataPtr);
    add=add << 8;
    number|=add;

    return number;
  }

  uint32_t FileScanner::ReadUInt32()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint32_t","File already in error state");
    }

    uint32_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+4-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint32_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,4> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read uint32_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  uint64_t FileScanner::ReadUInt64()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint64_t","File already in error state");
    }

    uint64_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+8-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read uint64_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,8> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read uint64_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  uint16_t FileScanner::Read(size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read size limited uint16_t","File already in error state");
    }

    uint16_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read size limited uint16_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,2> buffer;

    hasError=fread(buffer.data(),1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read size limited uint16_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  uint32_t FileScanner::ReadUInt32(size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read size limited uint32_t","File already in error state");
    }

    uint32_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read size limited uint32_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,4> buffer;

    hasError=fread(buffer.data(),1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read size limited uint32_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  uint64_t FileScanner::ReadUInt64(size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read size limited uint64_t","File already in error state");
    }

    uint64_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read size limited uint64_t","Cannot read beyond end of file");
      }

      char     *dataPtr=&mmap[offset];
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

      return number;
    }
#endif

    std::array<unsigned char,8> buffer;

    hasError=fread(buffer.data(),1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read size limited uint64_t");
    }

    unsigned char *dataPtr=buffer.data();
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

    return number;
  }

  ObjectFileRef FileScanner::ReadObjectFileRef()
  {
    uint8_t    typeByte=ReadUInt8();
    FileOffset fileOffset=ReadFileOffset();

    return {fileOffset,(RefType)typeByte};
  }

  Color FileScanner::ReadColor()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read file offset","File already in error state");
    }

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+4-1>=size) {
        hasError=true;
        throw IOException(filename,
                          "Cannot read color",
                          "Cannot read beyond end of file");
      }

      double r=((unsigned char)mmap[offset+0])/255.0;
      double g=((unsigned char)mmap[offset+1])/255.0;
      double b=((unsigned char)mmap[offset+2])/255.0;
      double a=((unsigned char)mmap[offset+3])/255.0;

      offset+=4;

      return {r,g,b,a};
    }
#endif

    std::array<unsigned char,4> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read color");
    }

    double r=buffer[0]/255.0;
    double g=buffer[1]/255.0;
    double b=buffer[2]/255.0;
    double a=buffer[3]/255.0;

    return {r,g,b,a};
  }

  FileOffset FileScanner::ReadFileOffset()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read file offset","File already in error state");
    }

    FileOffset fileOffset=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+8-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read file offset","Cannot read beyond end of file");
      }

      const char *dataPtr=&mmap[offset];
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

      return fileOffset;
    }
#endif

    std::array<unsigned char,8> buffer;

    hasError=fread(buffer.data(),1,buffer.size(),file)!=buffer.size();

    if (hasError) {
      throw IOException(filename,"Cannot read file offset");
    }

    const unsigned char *dataPtr=buffer.data();
    FileOffset          add;

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

    return fileOffset;
  }

  FileOffset FileScanner::ReadFileOffset(size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read file offset","File already in error state");
    }

    assert(bytes>0 && bytes<=8);

    FileOffset fileOffset=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+bytes-1>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read file offset","Cannot read beyond end of file");
      }

      const char *dataPtr=&mmap[offset];
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

      return fileOffset;
    }
#endif

    std::array<unsigned char,8> buffer;

    hasError=fread(buffer.data(),1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot read file offset");
    }

    const unsigned char *dataPtr=buffer.data();
    FileOffset          add;

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

    return fileOffset;
  }

  int16_t FileScanner::ReadInt16Number()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int16_t number","File already in error state");
    }

    int16_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int16_t number","Cannot read beyond end of file");
      }

      unsigned int bytes=DecodeNumber(&mmap[offset],number);

      offset+=bytes;

      return number;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read int16_t number");
    }

    using        num_t = int16_t;
    unsigned int shift=0;

    // negative form
    if ((buffer & 0x01)!=0) {
      int16_t      val=(buffer & 0x7e) >> 1;
      unsigned int nextShift=6;

      number=-1;

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
      int16_t      val=(buffer & 0x7e) >> 1;
      unsigned int nextShift=6;

      number=0;

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

    return number;
  }

  int32_t FileScanner::ReadInt32Number()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int32_t number","File already in error state");
    }

    int32_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int32_t number","Cannot read beyond end of file");
      }

      unsigned int bytes=DecodeNumber(&mmap[offset],number);

      offset+=bytes;

      return number;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read int32_t number");
    }


    using        num_t = int32_t;
    unsigned int shift=0;

    // negative form
    if ((buffer & 0x01)!=0) {
      int32_t      val=(buffer & 0x7e) >> 1;
      unsigned int nextShift=6;

      number=-1;

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
      int32_t      val=(buffer & 0x7e) >> 1;
      unsigned int nextShift=6;

      number=0;

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

    return number;
  }

  int64_t FileScanner::ReadInt64Number()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read int64_t number","File already in error state");
    }

    int64_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset>=size) {
        hasError=true;
        throw IOException(filename,"Cannot read int64_t number","Cannot read beyond end of file");
      }

      unsigned int bytes=DecodeNumber(&mmap[offset],number);

      offset+=bytes;

      return number;
    }
#endif

    char buffer;

    if (fread(&buffer,1,1,file)!=1) {
      hasError=true;
      throw IOException(filename,"Cannot read int64_t number");
    }

    using num_t = int64_t;

    unsigned int shift=0;

    // negative form
    if ((buffer & 0x01)!=0) {
      int64_t      val=(buffer & 0x7e) >> 1;
      unsigned int nextShift=6;

      number=-1;

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
      int64_t      val=(buffer & 0x7e) >> 1;
      unsigned int nextShift=6;

      number=0;

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

    return number;
  }

  uint16_t FileScanner::ReadUInt16Number()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint16_t number","File already in error state");
    }

    uint16_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      unsigned int shift=0;

      for (; offset<size; offset++) {
        number|=static_cast<uint16_t>(mmap[offset] & 0x7f) << shift;

        if ((mmap[offset] & 0x80)==0) {
          offset++;
          return number;
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
        return number;
      }

      if (fread(&buffer,1,1,file)!=1) {
        hasError=true;
        throw IOException(filename,"Cannot read uint16_t number");
      }

      shift+=7;
    }
  }

  uint32_t FileScanner::ReadUInt32Number()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint32_t number","File already in error state");
    }

    uint32_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      unsigned int shift=0;

      for (; offset<size; offset++) {
        number|=static_cast<uint32_t>(mmap[offset] & 0x7f) << shift;

        if ((mmap[offset] & 0x80)==0) {
          offset++;
          return number;
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
        return number;
      }

      if (fread(&buffer,1,1,file)!=1) {
        hasError=true;
        throw IOException(filename,"Cannot read uint32_t number");
      }

      shift+=7;
    }
  }

  uint64_t FileScanner::ReadUInt64Number()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read uint64_t number","File already in error state");
    }

    uint64_t number=0;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      unsigned int shift=0;

      for (; offset<size; offset++) {
        number|=static_cast<uint64_t>(mmap[offset] & 0x7f) << shift;

        if ((mmap[offset] & 0x80)==0) {
          offset++;
          return number;
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
        return number;
      }

      if (fread(&buffer,1,1,file)!=1) {
        hasError=true;
        throw IOException(filename,"Cannot read uint64_t number");
      }

      shift+=7;
    }
  }

  GeoCoord FileScanner::ReadCoord()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read coordinate","File already in error state");
    }

    uint32_t latDat;
    uint32_t lonDat;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+coordByteSize-1>=size) {
        hasError=true;

        throw IOException(filename,"Cannot read coordinate","Cannot read beyond end of file");
      }

      const char *dataPtr=&mmap[offset];

      latDat=  ((unsigned char) dataPtr[0] <<  0)
             | ((unsigned char) dataPtr[1] <<  8)
             | ((unsigned char) dataPtr[2] << 16)
             | ((unsigned char) (dataPtr[6] & 0x0f) << 24);

      lonDat=  ((unsigned char) dataPtr[3] <<  0)
             | ((unsigned char) dataPtr[4] <<  8)
             | ((unsigned char) dataPtr[5] << 16)
             | ((unsigned char) (dataPtr[6] & 0xf0) << 20);

      offset+=coordByteSize;

      return CreateCoord(latDat, lonDat);
    }
#endif

    std::array<unsigned char,coordByteSize> buffer;

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

    return CreateCoord(latDat, lonDat);
  }

  std::tuple<GeoCoord,bool> FileScanner::ReadConditionalCoord()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read coordinate","File already in error state");
    }

    uint32_t latDat;
    uint32_t lonDat;
    GeoCoord coord;
    bool     isSet;

#if defined(HAVE_MMAP) || defined(_WIN32)
    if (mmap!=nullptr) {
      if (offset+coordByteSize-1>=size) {
        hasError=true;

        throw IOException(filename,"Cannot read coordinate","Cannot read beyond end of file");
      }

      const char *dataPtr=&mmap[offset];

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
        coord=CreateCoord(latDat, lonDat);
        isSet=true;
      }

      return std::make_tuple(coord,isSet);
    }
#endif

    std::array<unsigned char,coordByteSize> buffer;

    hasError=fread(&buffer,1,buffer.size(),file)!=buffer.size();

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
      coord=CreateCoord(latDat, lonDat);
      isSet=true;
    }

    return std::make_tuple(coord,isSet);
  }

  void FileScanner::Read(std::vector<Point>& nodes,
                         std::vector<SegmentGeoBox> &segments,
                         GeoBox &bbox,
                         bool readIds)
  {
    size_t  coordBitSize;
    uint8_t sizeByte;

    sizeByte=ReadUInt8();

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
        sizeByte=ReadUInt8();

        nodeCount|=(sizeByte & 0x7f) << 4;

        if ((sizeByte & 0x80) != 0) {
          sizeByte=ReadUInt8();

          nodeCount|=(sizeByte & 0x7f) << 11;

          if ((sizeByte & 0x80) != 0) {
            sizeByte=ReadUInt8();

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
        sizeByte=ReadUInt8();

        nodeCount|=(sizeByte & 0x7f) << 5;

        if ((sizeByte & 0x80) != 0) {
          sizeByte=ReadUInt8();

          nodeCount|=(sizeByte & 0x7f) << 12;

          if ((sizeByte & 0x80) != 0) {
            sizeByte=ReadUInt8();

            nodeCount|=sizeByte << 19;
          }
        }
      }
    }

    nodes.resize(nodeCount);

    size_t byteBufferSize=(nodeCount-1)*coordBitSize/8;

    GeoCoord firstCoord=ReadCoord();

    uint32_t latValue=(uint32_t)round((firstCoord.GetLat()+90.0)*latConversionFactor);
    uint32_t lonValue=(uint32_t)round((firstCoord.GetLon()+180.0)*lonConversionFactor);

    nodes[0].SetCoord(firstCoord);

    const uint8_t *tmpBuffer = (uint8_t*)ReadInternal(byteBufferSize);

    if (coordBitSize==16) {
      size_t currentCoordPos=1;

      for (size_t i=0; i<byteBufferSize; i+=2) {
        int32_t latDelta=(int8_t)tmpBuffer[i];
        int32_t lonDelta=(int8_t)tmpBuffer[i+1];

        latValue+=latDelta;
        lonValue+=lonDelta;

        SetCoord(latValue,lonValue,nodes[currentCoordPos]);

        currentCoordPos++;
      }
    }
    else if (coordBitSize==32) {
      size_t currentCoordPos=1;

      for (size_t i=0; i<byteBufferSize; i+=4) {
        uint32_t latUDelta=tmpBuffer[i+0] | (tmpBuffer[i+1]<<8);
        uint32_t lonUDelta=tmpBuffer[i+2] | (tmpBuffer[i+3]<<8);
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

        SetCoord(latValue,lonValue,nodes[currentCoordPos]);

        currentCoordPos++;
      }
    }
    else {
      size_t currentCoordPos=1;

      for (size_t i=0; i<byteBufferSize; i+=6) {
        uint32_t latUDelta=(tmpBuffer[i+0]) | (tmpBuffer[i+1]<<8) | (tmpBuffer[i+2]<<16);
        uint32_t lonUDelta=(tmpBuffer[i+3]) | (tmpBuffer[i+4]<<8) | (tmpBuffer[i+5]<<16);
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

        SetCoord(latValue,lonValue,nodes[currentCoordPos]);

        currentCoordPos++;
      }
    }

    GetBoundingBox(nodes, bbox);

    // we will prepare segment bounding boxes just for long point vectors
    if (nodeCount > 1024) {
      // initialise segments
      size_t segmentCount = (((uint64_t) nodeCount - 1) / 1024) + 1;
      segments.reserve(segmentCount);
      Point *pd = nodes.data();
      for (size_t i = 0; i < segmentCount; i++) {
        SegmentGeoBox s;
        s.from = i * 1024;
        s.to = std::min(nodeCount, s.from + 1024); // exclusive
        GetBoundingBox(pd+s.from, pd+s.to, s.bbox);
        segments.emplace_back(std::move(s));
      }
    }

    if (hasNodes) {
      size_t idCurrent=0;

      while (idCurrent<nodeCount) {
        uint8_t bitset;
        size_t bitmask=1;

        bitset=ReadUInt8();

        for (size_t i=0; i<8 && idCurrent<nodeCount; i++) {
          if ((bitset & bitmask)!=0) {
            uint8_t serial=ReadUInt8();

            nodes[idCurrent].SetSerial(serial);
          }

          bitmask*=2;
          idCurrent++;
        }
      }
    }
  }

  GeoBox FileScanner::ReadBox()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read geo box","File already in error state");
    }

    GeoCoord minCoord=ReadCoord();
    GeoCoord maxCoord=ReadCoord();

    return {minCoord,
            maxCoord};
  }

  TypeId FileScanner::ReadTypeId(uint8_t maxBytes)
  {
    if (maxBytes==1) {
      uint8_t byteValue=ReadUInt8();

      return byteValue;
    }

    if (maxBytes==2) {
      uint8_t byteValue;

      byteValue=ReadUInt8();

      TypeId id=byteValue *256;

      byteValue=ReadUInt8();

      id+=byteValue;

      return id;
    }

    assert(false);
    return 0;
  }

  std::vector<ObjectFileRef> FileScanner::ReadObjectFileRefs(size_t count)
  {
    std::vector<ObjectFileRef> refs(count);
    FileOffset                 lastFileOffset=0;

    for (size_t i=0; i<count; i++) {
      FileOffset offset=ReadUInt64Number();
      RefType    type=(RefType)(offset%4);

      offset=offset >> 2;
      offset=offset+lastFileOffset;

      refs[i].Set(offset,type);

      lastFileOffset=offset;
    }

    return refs;
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
    FileOffset offset=reader.ReadUInt64Number();
    RefType    type=(RefType)(offset%4);

    offset=offset >> 2;
    offset=offset+lastFileOffset;

    ref.Set(offset,type);

    lastFileOffset=offset;
  }
}
