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

// Make sure that 64 file access activation works, by first importing
// The Config.h, than our class and then std io.
#include <osmscout/private/Config.h>

#include <osmscout/util/FileWriter.h>

#include <string.h>

#include <stdio.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Number.h>
#include <iostream>
#include <iomanip>
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

    this->filename=filename;

    file=fopen(filename.c_str(),"w+b");

    hasError=file==NULL;

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

    hasError=fwrite(&number,sizeof(char),sizeof(int8_t),file)!=sizeof(int8_t);

    return !hasError;
  }

  bool FileWriter::Write(int16_t number)
  {
    if (HasError()) {
      return false;
    }

    char buffer[2];

    buffer[0]=((number >> 0) & 0xff);
    buffer[1]=((number >> 8) & 0xff);

    hasError=fwrite(buffer,1,2,file)!=2;

    return !hasError;
  }

  bool FileWriter::Write(int32_t number)
  {
    if (HasError()) {
      return false;
    }

    char buffer[4];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);

    hasError=fwrite(buffer,1,4,file)!=4;

    return !hasError;
  }

#if defined(OSMSCOUT_HAVE_INT64_T)
  bool FileWriter::Write(int64_t number)
  {
    if (HasError()) {
      return false;
    }

    char buffer[8];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);
    buffer[4]=((number >> 32) & 0xff);
    buffer[5]=((number >> 40) & 0xff);
    buffer[6]=((number >> 48) & 0xff);
    buffer[7]=((number >> 56) & 0xff);

    hasError=fwrite(buffer,1,8,file)!=8;

    return !hasError;
  }
#endif

  bool FileWriter::Write(uint8_t number)
  {
    if (HasError()) {
      return false;
    }

    hasError=fwrite(&number,1,1,file)!=1;

    return !hasError;
  }

  bool FileWriter::Write(uint16_t number)
  {
    if (HasError()) {
      return false;
    }

    char buffer[2];

    buffer[0]=((number >> 0) & 0xff);
    buffer[1]=((number >> 8) & 0xff);

    hasError=fwrite(buffer,1,2,file)!=2;

    return !hasError;
  }

  bool FileWriter::Write(uint32_t number)
  {
    if (HasError()) {
      return false;
    }

    char buffer[4];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);

    hasError=fwrite(buffer,1,4,file)!=4;

    return !hasError;
  }

#if defined(OSMSCOUT_HAVE_UINT64_T)
  bool FileWriter::Write(uint64_t number)
  {
    if (HasError()) {
      return false;
    }

    char buffer[8];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);
    buffer[4]=((number >> 32) & 0xff);
    buffer[5]=((number >> 40) & 0xff);
    buffer[6]=((number >> 48) & 0xff);
    buffer[7]=((number >> 56) & 0xff);

    hasError=fwrite(buffer,1,8,file)!=8;

    return !hasError;
  }
#endif

  bool FileWriter::Write(uint16_t number, size_t bytes)
  {
    if (HasError()) {
      return false;
    }

    char buffer[2];

    buffer[0]=((number >> 0) & 0xff);
    buffer[1]=((number >> 8) & 0xff);

    hasError=fwrite(buffer,1,bytes,file)!=bytes;

    return !hasError;
  }

  bool FileWriter::Write(uint32_t number, size_t bytes)
  {
    if (HasError()) {
      return false;
    }

    char buffer[4];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);

    hasError=fwrite(buffer,1,bytes,file)!=bytes;

    return !hasError;
  }

#if defined(OSMSCOUT_HAVE_UINT64_T)
  bool FileWriter::Write(uint64_t number, size_t bytes)
  {
    if (HasError()) {
      return false;
    }

    char buffer[8];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);
    buffer[4]=((number >> 32) & 0xff);
    buffer[5]=((number >> 40) & 0xff);
    buffer[6]=((number >> 48) & 0xff);
    buffer[7]=((number >> 56) & 0xff);

    hasError=fwrite(buffer,1,bytes,file)!=bytes;

    return !hasError;
  }
#endif

  bool FileWriter::WriteFileOffset(FileOffset fileOffset)
  {
    if (HasError()) {
      return false;
    }

    char buffer[8];

    buffer[0]=((fileOffset >>  0) & 0xff);
    buffer[1]=((fileOffset >>  8) & 0xff);
    buffer[2]=((fileOffset >> 16) & 0xff);
    buffer[3]=((fileOffset >> 24) & 0xff);
    buffer[4]=((fileOffset >> 32) & 0xff);
    buffer[5]=((fileOffset >> 40) & 0xff);
    buffer[6]=((fileOffset >> 48) & 0xff);
    buffer[7]=((fileOffset >> 56) & 0xff);

    hasError=fwrite(buffer,1,8,file)!=8;

    return !hasError;
  }

  bool FileWriter::WriteFileOffset(FileOffset fileOffset,
                                   size_t bytes)
  {
    if (HasError()) {
      return false;
    }

    assert(bytes>0 && bytes<=8);

    char buffer[8];

    buffer[0]=((fileOffset >>  0) & 0xff);
    buffer[1]=((fileOffset >>  8) & 0xff);
    buffer[2]=((fileOffset >> 16) & 0xff);
    buffer[3]=((fileOffset >> 24) & 0xff);
    buffer[4]=((fileOffset >> 32) & 0xff);
    buffer[5]=((fileOffset >> 40) & 0xff);
    buffer[6]=((fileOffset >> 48) & 0xff);
    buffer[7]=((fileOffset >> 56) & 0xff);

    hasError=fwrite(buffer,1,bytes,file)!=bytes;

    return !hasError;
  }

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.
    */
  bool FileWriter::WriteNumber(int16_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[3];
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
  bool FileWriter::WriteNumber(int32_t number)
  {
    if (HasError()) {
      return false;
    }

    char         buffer[5];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    return !hasError;
  }

#if defined(OSMSCOUT_HAVE_INT64_T)
  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.
    */
  bool FileWriter::WriteNumber(int64_t number)
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

  bool FileWriter::WriteCoord(const GeoCoord& coord)
  {
    if (HasError()) {
      return false;
    }

    uint32_t latValue=(uint32_t)round((coord.GetLat()+90.0)*latConversionFactor);
    uint32_t lonValue=(uint32_t)round((coord.GetLon()+180.0)*lonConversionFactor);

    char buffer[coordByteSize];

    buffer[0]=((latValue >>  0) & 0xff);
    buffer[1]=((latValue >>  8) & 0xff);
    buffer[2]=((latValue >> 16) & 0xff);

    buffer[3]=((lonValue >>  0) & 0xff);
    buffer[4]=((lonValue >>  8) & 0xff);
    buffer[5]=((lonValue >> 16) & 0xff);

    buffer[6]=((latValue >> 24) & 0x07) | ((lonValue >> 20) & 0x70);

    //std::cout << coord.GetLat() << "," << coord.GetLon() << " " << latValue << " " << lonValue << std::endl;

    hasError=fwrite(buffer,1,coordByteSize,file)!=coordByteSize;

    return !hasError;
  }

  bool FileWriter::WriteInvalidCoord()
  {
    if (HasError()) {
      return false;
    }

    char buffer[coordByteSize];

    buffer[0]=0xff;
    buffer[1]=0xff;
    buffer[2]=0xff;

    buffer[3]=0xff;
    buffer[4]=0xff;
    buffer[5]=0xff;

    buffer[6]=0xff;

    hasError=fwrite(buffer,1,coordByteSize,file)!=coordByteSize;

    return !hasError;
  }

  bool FileWriter::Write(const std::vector<GeoCoord>& nodes)
  {
    if (!WriteNumber((uint32_t)nodes.size())) {
      return false;
    }

    GeoCoord minCoord=nodes[0];

    for (size_t i=1; i<nodes.size(); i++) {
      minCoord.Set(std::min(minCoord.GetLat(),nodes[i].GetLat()),
                   std::min(minCoord.GetLon(),nodes[i].GetLon()));
    }

    if (!WriteCoord(minCoord)) {
      return false;
    }

    for (size_t i=0; i<nodes.size(); i++) {
      if (!WriteNumber((uint32_t)round((nodes[i].GetLat()-minCoord.GetLat())*latConversionFactor))) {
        return false;
      }

      if (!WriteNumber((uint32_t)round((nodes[i].GetLon()-minCoord.GetLon())*lonConversionFactor))) {
        return false;
      }
    }

    return true;
  }

  bool FileWriter::Write(const std::vector<GeoCoord>& nodes,
                         size_t count)
  {
    GeoCoord minCoord=nodes[0];

    for (size_t i=1; i<count; i++) {
      minCoord.Set(std::min(minCoord.GetLat(),nodes[i].GetLat()),
                   std::min(minCoord.GetLon(),nodes[i].GetLon()));
    }

    if (!WriteCoord(minCoord)) {
      return false;
    }

    for (size_t i=0; i<count; i++) {
      uint32_t latValue=(uint32_t)round((nodes[i].GetLat()-minCoord.GetLat())*latConversionFactor);
      uint32_t lonValue=(uint32_t)round((nodes[i].GetLon()-minCoord.GetLon())*lonConversionFactor);

      if (!WriteNumber(latValue)) {
        return false;
      }

      if (!WriteNumber(lonValue)) {
        return false;
      }
    }

    return true;
  }

  bool FileWriter::WriteTypeId(TypeId id, uint8_t maxBytes)
  {
    if (maxBytes==1) {
      return Write((uint8_t)id);
    }
    else {
      return WriteNumber(id);
    }
  }

  bool FileWriter::Flush()
  {
    if (HasError()) {
      return false;
    }

    hasError=fflush(file)!=0;

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

    delete [] buffer;

    return !hasError;
  }
}

