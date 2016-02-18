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

#include <algorithm>

#include <string.h>

#include <stdio.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Logger.h>
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
      log.Warn() << "Automatically closing FileWriter for file '" << filename << "'!";
      CloseFailsafe();
    }
  }

  void FileWriter::Open(const std::string& filename)
  {
    if (file!=NULL) {
      throw IOException(filename,"Error opening file for writing","File already opened");
    }

    hasError=true;
    this->filename=filename;

    file=fopen(filename.c_str(),"w+b");

    if (file==NULL) {
      throw IOException(filename,"Error opening file for writing");
    }

    hasError=false;
  }

  void FileWriter::Close()
  {
    if (file==NULL) {
      throw IOException(filename,"Cannot close file","File already closed");
    }

    if (fclose(file)!=0) {
      file=NULL;
      throw IOException(filename,"Cannot close file");
    }

    file=NULL;
  }

  void FileWriter::CloseFailsafe()
  {
    if (file==NULL) {
      return;
    }

    fclose(file);

    file=NULL;
  }

  std::string FileWriter::GetFilename() const
  {
    return filename;
  }

  /**
   * Returns the current position of the writing cursor in relation to the begining of the file
   *
   * throws IOException on error
   */
  FileOffset FileWriter::GetPos()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read position in file","File already in error state");
    }

#if defined(HAVE_FSEEKO)
    off_t filepos=ftello(file);

    if (filepos==-1) {
      hasError=true;
      throw IOException(filename,"Cannot read position in file");
    }

    return (FileOffset)filepos;
#else
    long filepos=ftell(file);

    if (filepos==-1) {
      hasError=true;
      throw IOException(filename,"Cannot read position in file");
    }

    return (FileOffset)filepos;
#endif
  }

  /**
   * Moves the writing cursor to the given file position
   *
   * throws IOException on error
   */
  void FileWriter::SetPos(FileOffset pos)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read position in file","File already in error state");
    }

#if defined(HAVE_FSEEKO)
    hasError=fseeko(file,(off_t)pos,SEEK_SET)!=0;
#else
    hasError=fseek(file,pos,SEEK_SET)!=0;
#endif

    if (hasError) {
      throw IOException(filename,"Cannot set position in file");
    }
  }

  /**
   * Moves the writing cursor to the start of the file (offset 0)
   *
   * throws IOException on error
   */
  void FileWriter::GotoBegin()
  {
    return SetPos(0);
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

  bool FileWriter::Write(const ObjectFileRef& ref)
  {
    return Write((uint8_t)ref.GetType()) &&
           WriteFileOffset(ref.GetFileOffset());
  }

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
    // Quick exit for empty vector arrays
    if (nodes.empty()) {
      uint8_t size=0;

      return Write(size);
    }

    // A lat and a lon delta for each coordinate delta
    deltaBuffer.resize((nodes.size()-1)*2);

    //
    // Calculate deltas and required space
    //

    uint32_t lastLat=(uint32_t)round((nodes[0].GetLat()+90.0)*latConversionFactor);
    uint32_t lastLon=(uint32_t)round((nodes[0].GetLon()+180.0)*lonConversionFactor);
    size_t   deltaBufferPos=0;
    size_t   coordBitSize=16;

    for (size_t i=1; i<nodes.size(); i++) {
      uint32_t currentLat=(uint32_t)round((nodes[i].GetLat()+90.0)*latConversionFactor);
      uint32_t currentLon=(uint32_t)round((nodes[i].GetLon()+180.0)*lonConversionFactor);

      // lat

      int32_t latDelta=currentLat-lastLat;

      if (latDelta>=-128 && latDelta<=127) {
        coordBitSize=std::max(coordBitSize,(size_t)16); // 2x 8 bit
      }
      else if (latDelta>=-32768 && latDelta<=32767) {
        coordBitSize=std::max(coordBitSize,(size_t)32); // 2* 16 bit
      }
      else if (latDelta>=-8388608 && latDelta<=8388608) {
        coordBitSize=std::max(coordBitSize,(size_t)48); // 2 * 24 bit
      }
      else {
        return false;
      }

      deltaBuffer[deltaBufferPos]=latDelta;
      deltaBufferPos++;

      // lon

      int32_t lonDelta=currentLon-lastLon;

      if (lonDelta>=-128 && lonDelta<=127) {
        coordBitSize=std::max(coordBitSize,(size_t)16); // 2x 8 bit
      }
      else if (lonDelta>=-32768 && lonDelta<=32767) {
        coordBitSize=std::max(coordBitSize,(size_t)32); // 2* 16 bit
      }
      else if (lonDelta>=-8388608 && lonDelta<=8388608) {
        coordBitSize=std::max(coordBitSize,(size_t)48); // 2 * 24 bit
      }
      else {
        return false;
      }
      
      deltaBuffer[deltaBufferPos]=lonDelta;
      deltaBufferPos++;

      lastLat=currentLat;
      lastLon=currentLon;
    }

    size_t bytesNeeded=(nodes.size()-1)*coordBitSize/8; // all coordinates in the same encoding

    //
    // Write starting length / signal bit section
    //

    if (nodes.size()<32) {
      uint8_t size;
      uint8_t nodeSize=(nodes.size() & 0x1f) << 2;

      if (coordBitSize==16) {
        size=0x00 | nodeSize;
      }
      else if (coordBitSize==32) {
        size=0x01 | nodeSize;
      }
      else {
        size=0x02 | nodeSize;
      }

      if (!Write(size)) {
        return false;
      }
    }
    else if (nodes.size()<4096) {
      uint8_t size[2];
      uint8_t nodeSize1=((nodes.size() & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
      uint8_t nodeSize2=nodes.size() >> 5; // The final bits

      if (coordBitSize==16) {
        size[0]=0x00 | nodeSize1;
      }
      else if (coordBitSize==32) {
        size[0]=0x01 | nodeSize1;
      }
      else {
        size[0]=0x02 | nodeSize1;
      }

      size[1]=nodeSize2;

      if (!Write((char*)size,2)) {
        return false;
      }
    }
    else {
      uint8_t size[3];
      uint8_t nodeSize1=((nodes.size() & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
      uint8_t nodeSize2=((nodes.size() >> 5) & 0x7f) | 0x80; // Further 7 bits + continuation bit
      uint8_t nodeSize3=nodes.size() >> 12; // The final bits

      if (coordBitSize==16) {
        size[0]=0x00 | nodeSize1;
      }
      else if (coordBitSize==32) {
        size[0]=0x01 | nodeSize1;
      }
      else {
        size[0]=0x02 | nodeSize1;
      }

      size[1]=nodeSize2;
      size[2]=nodeSize3;

      if (!Write((char*)size,3)) {
        return false;
      }
    }

    //std::cout << "Write " << std::dec << nodes.size() << " nodes, " << coordBitSize << " bits per coordinate pair" << std::endl;

    /*
    std::cout << "Write - Calculated deltas: ";
    for (size_t i=0; i<deltaBuffer.size(); i++) {
      std::cout << std::hex << deltaBuffer[i] << " ";
    }
    std::cout << std::endl;*/

    //
    // Write data array
    //

    if (!WriteCoord(nodes[0])) {
      return false;
    }

    byteBuffer.resize(bytesNeeded);

    if (coordBitSize==16) {
      size_t byteBufferPos=0;

      for (size_t i=0; i<deltaBuffer.size(); i++) {
        byteBuffer[byteBufferPos]=deltaBuffer[i];
        byteBufferPos++;
      }
    }
    else if (coordBitSize==32) {
      size_t byteBufferPos=0;

      for (size_t i=0; i<deltaBuffer.size(); i++) {
        byteBuffer[byteBufferPos]=deltaBuffer[i] & 0xff;
        ++byteBufferPos;

        byteBuffer[byteBufferPos]=(deltaBuffer[i] >> 8);
        ++byteBufferPos;
      }
    }
    else {
      for (size_t i=0; i<deltaBuffer.size(); i++) {
        size_t byteBufferPos=0;

        for (size_t i=0; i<deltaBuffer.size(); i+=2) {
          byteBuffer[byteBufferPos]=deltaBuffer[i] & 0xff;
          ++byteBufferPos;

          byteBuffer[byteBufferPos]=(deltaBuffer[i] >> 8) & 0xff;
          ++byteBufferPos;

          byteBuffer[byteBufferPos]=deltaBuffer[i] >> 16;
          ++byteBufferPos;

          byteBuffer[byteBufferPos]=deltaBuffer[i+1] & 0xff;
          ++byteBufferPos;

          byteBuffer[byteBufferPos]=(deltaBuffer[i+1] >> 8) & 0xff;
          ++byteBufferPos;

          byteBuffer[byteBufferPos]=deltaBuffer[i+1] >> 16;
          ++byteBufferPos;
        }
      }
    }

    if (!Write((char*)byteBuffer.data(),byteBuffer.size())) {
      return false;
    }

    /*
    std::cout << "Write - byte buffer: ";
    for (size_t i=0; i<byteBuffer.size(); i++) {
      std::cout << std::hex << (unsigned int) byteBuffer[i] << " ";
    }
    std::cout << std::endl;*/

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

  void FileWriter::Flush()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot flush file","File already in error state");
    }

    hasError=fflush(file)!=0;

    if (hasError) {
      throw IOException(filename,"Cannot flush file");
    }
  }

  void FileWriter::FlushCurrentBlockWithZeros(size_t blockSize)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot flush file","File already in error state");
    }

    FileOffset currentPos;
    size_t     bytesToWrite;

    currentPos=GetPos();

    if (currentPos%blockSize==0) {
      return;
    }

    bytesToWrite=blockSize-(currentPos%blockSize);

    char *buffer=new char[blockSize];

    memset(buffer,0,bytesToWrite);

    hasError=fwrite(buffer,sizeof(char),bytesToWrite,file)!=bytesToWrite;

    delete [] buffer;

    if (hasError) {
      throw IOException(filename,"Cannot write data to flush current block");
    }
  }

  bool IsValidToWrite(const std::vector<GeoCoord>& nodes)
  {
    if (nodes.size()<=1) {
      return true;
    }

    uint32_t lastLat=(uint32_t)round((nodes[0].GetLat()+90.0)*latConversionFactor);
    uint32_t lastLon=(uint32_t)round((nodes[0].GetLon()+180.0)*lonConversionFactor);

    for (size_t i=1; i<nodes.size(); i++) {
      uint32_t currentLat=(uint32_t)round((nodes[i].GetLat()+90.0)*latConversionFactor);
      uint32_t currentLon=(uint32_t)round((nodes[i].GetLon()+180.0)*lonConversionFactor);

      // lat

      int32_t latDelta=currentLat-lastLat;

      if (latDelta<-8388608 || latDelta>8388608) {
        return false;
      }

      // lon

      int32_t lonDelta=currentLon-lastLon;

      if (lonDelta<-8388608 || lonDelta>8388608) {
        return false;
      }

      lastLat=currentLat;
      lastLon=currentLon;
    }

    return true;
  }

  ObjectFileRefStreamWriter::ObjectFileRefStreamWriter(FileWriter& writer)
  : writer(writer),
    lastFileOffset(0)
  {
    // no code
  }

  void ObjectFileRefStreamWriter::Reset()
  {
    lastFileOffset=0;
  }

  bool ObjectFileRefStreamWriter::Write(const ObjectFileRef& ref)
  {
    // ObjectFileRefs must be sorted
    assert(ref.GetFileOffset()>=lastFileOffset);

    FileOffset offset=ref.GetFileOffset()-lastFileOffset;

    // Make room for two additional lower bits
    offset=offset << 2;

    offset=offset+ref.GetType();

    lastFileOffset=ref.GetFileOffset();

    return writer.WriteNumber(offset);
  }

}

