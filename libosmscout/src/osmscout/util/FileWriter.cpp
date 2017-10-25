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

  const uint64_t FileWriter::MAX_NODES=0x03FFFFFF; // 26 bits

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

  /**
   *
   * @throws IOException
   */
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

  /**
   *
   * @throws IOException
   */
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
   * @throws IOException
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
#elif defined(HAVE__FTELLI64)
    __int64 filepos=_ftelli64(file);

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
   * @throws IOException
   */
  void FileWriter::SetPos(FileOffset pos)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot read position in file","File already in error state");
    }

#if defined(HAVE_FSEEKO)
    hasError=fseeko(file,(off_t)pos,SEEK_SET)!=0;
#elif defined(HAVE__FTELLI64)
    hasError=_fseeki64(file,(__int64)pos,SEEK_SET)!=0;
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
   * @throws IOException
   */
  void FileWriter::GotoBegin()
  {
    return SetPos(0);
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(const char* buffer, size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write char*","File already in error state");
    }

    hasError=fwrite(buffer,sizeof(char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write char*");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(const std::string& value)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write std::string","File already in error state");
    }

    size_t length=value.length()+1;

    hasError=fwrite(value.c_str(),sizeof(char),length,file)!=length;

    if (hasError) {
      throw IOException(filename,"Cannot write std::string");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(bool boolean)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write bool","File already in error state");
    }

    char value=boolean ? 1 : 0;

    hasError=fwrite((const char*)&value,sizeof(char),1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot write bool");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(int8_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int8_t","File already in error state");
    }

    hasError=fwrite(&number,sizeof(char),sizeof(int8_t),file)!=sizeof(int8_t);

    if (hasError) {
      throw IOException(filename,"Cannot write int8_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(int16_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int16_t","File already in error state");
    }

    char buffer[2];

    buffer[0]=((number >> 0) & 0xff);
    buffer[1]=((number >> 8) & 0xff);

    hasError=fwrite(buffer,1,2,file)!=2;

    if (hasError) {
      throw IOException(filename,"Cannot write int16_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(int32_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int32_t","File already in error state");
    }

    char buffer[4];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);

    hasError=fwrite(buffer,1,4,file)!=4;

    if (hasError) {
      throw IOException(filename,"Cannot write int32_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(int64_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int64_t","File already in error state");
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

    if (hasError) {
      throw IOException(filename,"Cannot write int64_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint8_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint8_t","File already in error state");
    }

    hasError=fwrite(&number,1,1,file)!=1;

    if (hasError) {
      throw IOException(filename,"Cannot write uint8_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint16_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint16_t","File already in error state");
    }

    char buffer[2];

    buffer[0]=((number >> 0) & 0xff);
    buffer[1]=((number >> 8) & 0xff);

    hasError=fwrite(buffer,1,2,file)!=2;

    if (hasError) {
      throw IOException(filename,"Cannot write uint16_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint32_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint32_t","File already in error state");
    }

    char buffer[4];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);

    hasError=fwrite(buffer,1,4,file)!=4;

    if (hasError) {
      throw IOException(filename,"Cannot write uint32_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint64_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint64_t","File already in error state");
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

    if (hasError) {
      throw IOException(filename,"Cannot write uint64_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint16_t number, size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write size restricted uint16_t","File already in error state");
    }

    char buffer[2];

    buffer[0]=((number >> 0) & 0xff);
    buffer[1]=((number >> 8) & 0xff);

    hasError=fwrite(buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write size restricted uint16_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint32_t number, size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write size restricted uint32_t","File already in error state");
    }

    char buffer[4];

    buffer[0]=((number >>  0) & 0xff);
    buffer[1]=((number >>  8) & 0xff);
    buffer[2]=((number >> 16) & 0xff);
    buffer[3]=((number >> 24) & 0xff);

    hasError=fwrite(buffer,1,bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write size restricted uint32_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(uint64_t number, size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write size restricted uint64_t","File already in error state");
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

    if (hasError) {
      throw IOException(filename,"Cannot write size restricted uint64_t");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::Write(const ObjectFileRef& ref)
  {
    Write((uint8_t)ref.GetType());
    WriteFileOffset(ref.GetFileOffset());
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::WriteFileOffset(FileOffset fileOffset)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write FileOffset","File already in error state");
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

    if (hasError) {
      throw IOException(filename,"Cannot write FileOffset");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::WriteFileOffset(FileOffset fileOffset,
                                   size_t bytes)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write size limited FileOffset","File already in error state");
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

    if (HasError()) {
      throw IOException(filename,"Cannot write size limited FileOffset");
    }
  }

  /**
   * Write a numeric value to the file using some internal encoding
   * to reduce storage size. Note that this works only if the average number
   * is small.
   *
   * @throws IOException
   */
  void FileWriter::WriteNumber(int16_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int16_t number","File already in error state");
    }

    char         buffer[3];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write int16_t number");
    }
  }

  /**
   * Write a numeric value to the file using some internal encoding
   * to reduce storage size. Note that this works only if the average number
   * is small.
   *
   * @throws IOException
   */
  void FileWriter::WriteNumber(int32_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int32_t number","File already in error state");
    }

    char         buffer[5];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write int32_t number");
    }
  }

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.

    @throws IOException
    */
  void FileWriter::WriteNumber(int64_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write int64_t number","File already in error state");
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write int64_t number");
    }
  }

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.

    @throws IOException
    */
  void FileWriter::WriteNumber(uint16_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint16_t number","File already in error state");
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write uint16_t number");
    }
  }

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.

    @throws IOException
    */
  void FileWriter::WriteNumber(uint32_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint32_t number","File already in error state");
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write uint32_t number");
    }
  }

  /**
    Write a numeric value to the file using some internal encoding
    to reduce storage size. Note that this works only if the average number
    is small.

    @throws IOException
    */
  void FileWriter::WriteNumber(uint64_t number)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write uint64_t number","File already in error state");
    }

    char         buffer[10];
    unsigned int bytes;

    bytes=EncodeNumber(number,buffer);

    hasError=fwrite(buffer,sizeof(unsigned char),bytes,file)!=bytes;

    if (hasError) {
      throw IOException(filename,"Cannot write uint64_t number");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::WriteCoord(const GeoCoord& coord)
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write coordinate","File already in error state");
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

    if (hasError) {
      throw IOException(filename,"Cannot write coordinate");
    }
  }

  /**
   *
   * @throws IOException
   */
  void FileWriter::WriteInvalidCoord()
  {
    if (HasError()) {
      throw IOException(filename,"Cannot write coordinate","File already in error state");
    }

    unsigned char buffer[coordByteSize];

    buffer[0]=0xff;
    buffer[1]=0xff;
    buffer[2]=0xff;

    buffer[3]=0xff;
    buffer[4]=0xff;
    buffer[5]=0xff;

    buffer[6]=0xff;

    hasError=fwrite(buffer,1,coordByteSize,file)!=coordByteSize;

    if (hasError) {
      throw IOException(filename,"Cannot write coordinate");
    }
  }

  void FileWriter::Write(const std::vector<GeoCoord>& nodes)
  {
    // Quick exit for empty vector arrays
    if (nodes.empty()) {
      uint8_t size=0;

      Write(size);

      return;
    }

    size_t nodesSize=nodes.size();

    assert(nodesSize<=MAX_NODES);

    // A lat and a lon delta for each coordinate delta
    deltaBuffer.resize((nodesSize-1)*2);

    //
    // Calculate deltas and required space
    //

    uint32_t lastLat=(uint32_t)round((nodes[0].GetLat()+90.0)*latConversionFactor);
    uint32_t lastLon=(uint32_t)round((nodes[0].GetLon()+180.0)*lonConversionFactor);
    size_t   deltaBufferPos=0;
    size_t   coordBitSize=16;

    for (size_t i=1; i<nodesSize; i++) {
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
        throw IOException(filename,"Cannot write coordinate","Delta between coordinates too big");
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
        throw IOException(filename,"Cannot write coordinate","Delta between coordinates too big");
      }

      deltaBuffer[deltaBufferPos]=lonDelta;
      deltaBufferPos++;

      lastLat=currentLat;
      lastLon=currentLon;
    }

    size_t bytesNeeded=(nodesSize-1)*coordBitSize/8; // all coordinates in the same encoding

    //
    // Write starting length / signal bit section
    //

    // We use the first two bits to signal encoding size for coordinates

    uint8_t coordSizeFlags;

    if (coordBitSize==16) {
      coordSizeFlags=0x00;
    }
    else if (coordBitSize==32) {
      coordSizeFlags=0x01;
    }
    else {
      coordSizeFlags=0x02;
    }

    if (nodesSize<=0x1f) {    // 2^5 (8 bits - 2 flag bit -1 continuation bit)
      uint8_t nodeSize1=(nodesSize & 0x1f) << 2;
      uint8_t size=coordSizeFlags | nodeSize1;

      Write(size);
    }
    else if (nodesSize<=0xfff) { // 2^(5+7)=2^12 (16-2-1-1)
      uint8_t size[2];
      uint8_t nodeSize1=((nodesSize & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
      uint8_t nodeSize2=nodesSize >> 5; // The final bits

      size[0]=coordSizeFlags | nodeSize1;
      size[1]=nodeSize2;

      Write((char*)size,2);
    }
    else if (nodesSize<=0x7ffff) { // 2^(12+7)=2^19 (24-2-1-1-1)
      uint8_t size[3];
      uint8_t nodeSize1=((nodesSize & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
      uint8_t nodeSize2=((nodesSize >> 5) & 0x7f) | 0x80; // Further 7 bits + continuation bit
      uint8_t nodeSize3=nodesSize >> 12; // The final bits

      size[0]=coordSizeFlags | nodeSize1;
      size[1]=nodeSize2;
      size[2]=nodeSize3;

      Write((char*)size,3);
    }
    else { // 2^(19+8)2^27 (32-2-1-1-1)
      uint8_t size[4];
      uint8_t nodeSize1=((nodesSize & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
      uint8_t nodeSize2=((nodesSize >> 5) & 0x7f) | 0x80; // Further 7 bits + continuation bit
      uint8_t nodeSize3=((nodesSize >> 12) & 0x7f) | 0x80; // further 7 bits + continuation bit
      uint8_t nodeSize4=nodesSize >> 19; // The final bits

      size[0]=coordSizeFlags | nodeSize1;
      size[1]=nodeSize2;
      size[2]=nodeSize3;
      size[3]=nodeSize4;

      Write((char*)size,4);
    }

    //
    // Write data array
    //

    WriteCoord(nodes[0]);

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

    Write((char*)byteBuffer.data(),byteBuffer.size());
  }

  void FileWriter::Write(const std::vector<Point>& nodes, bool writeIds)
  {
    // Quick exit for empty vector arrays
    if (nodes.empty()) {
      uint8_t size=0;

      Write(size);

      return;
    }

    size_t nodesSize=nodes.size();

    assert(nodesSize<=MAX_NODES);

    // A lat and a lon delta for each coordinate delta
    deltaBuffer.resize((nodesSize-1)*2);

    //
    // Calculate deltas and required space
    //

    uint32_t lastLat=(uint32_t)round((nodes[0].GetLat()+90.0)*latConversionFactor);
    uint32_t lastLon=(uint32_t)round((nodes[0].GetLon()+180.0)*lonConversionFactor);
    size_t   deltaBufferPos=0;
    size_t   coordBitSize=16;

    for (size_t i=1; i<nodesSize; i++) {
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
        throw IOException(filename,"Cannot write coordinate","Delta between coordinates too big");
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
        throw IOException(filename,"Cannot write coordinate","Delta between coordinates too big");
      }

      deltaBuffer[deltaBufferPos]=lonDelta;
      deltaBufferPos++;

      lastLat=currentLat;
      lastLon=currentLon;
    }

    size_t bytesNeeded=(nodesSize-1)*coordBitSize/8; // all coordinates in the same encoding

    //
    // do we need to store node ids?
    //

    bool hasNodes=false;

    if (writeIds) {
      for (const auto& node : nodes) {
        if (node.IsRelevant()) {
          hasNodes=true;
          break;
        }
      }
    }

    //
    // Write starting length / signal bit section
    //

    // We use the first two bits to signal encoding size for coordinates

    uint8_t coordSizeFlags;

    if (coordBitSize==16) {
      coordSizeFlags=0x00;
    }
    else if (coordBitSize==32) {
      coordSizeFlags=0x01;
    }
    else {
      coordSizeFlags=0x02;
    }

    if (writeIds) {
      uint8_t hasNodesFlags=hasNodes ? 0x04 : 0x00;

      if (nodesSize<=0x0f) { // 2^4 (8 -3 flags -1 continuation bit)
        uint8_t nodeSize1=(nodesSize & 0x0f) << 3;
        uint8_t size=coordSizeFlags | hasNodesFlags | nodeSize1;

        Write(size);
      }
      else if (nodesSize<=0x7ff) { // 2^(4+7)=2^11 (16-3-1-1)
        uint8_t size[2];
        uint8_t nodeSize1=((nodesSize & 0x0f) << 3) | 0x80; // The initial 4 bits + continuation bit
        uint8_t nodeSize2=nodesSize >> 4;                   // The final bits

        size[0]=coordSizeFlags  | hasNodesFlags | nodeSize1;
        size[1]=nodeSize2;

        Write((char*)size,2);
      }
      else if (nodesSize<=0x3ffff) { // 2^(11+7)=2^18 (24-3-1-1-1)
        uint8_t size[3];
        uint8_t nodeSize1=((nodesSize & 0x0f) << 3) | 0x80; // The initial 4 bits + continuation bit
        uint8_t nodeSize2=((nodesSize >> 4) & 0x7f) | 0x80; // Further 7 bits + continuation bit
        uint8_t nodeSize3=nodesSize >> 11;                  // The final bits

        size[0]=coordSizeFlags  | hasNodesFlags | nodeSize1;
        size[1]=nodeSize2;
        size[2]=nodeSize3;

        Write((char*)size,3);
      }
      else { // 2^(18+8)=2^26 (32-3-1-1-1)
          uint8_t size[4];
          uint8_t nodeSize1=((nodesSize & 0x0f) << 3) | 0x80;  // The initial 4 bits + continuation bit
          uint8_t nodeSize2=((nodesSize >> 4) & 0x7f) | 0x80;  // Further 7 bits + continuation bit
          uint8_t nodeSize3=((nodesSize >> 11) & 0x7f) | 0x80; // further 7 bits + continuation bit
          uint8_t nodeSize4=nodesSize >> 18;                    // The final bits

          size[0]=coordSizeFlags  | hasNodesFlags | nodeSize1;
          size[1]=nodeSize2;
          size[2]=nodeSize3;
          size[3]=nodeSize4;

          Write((char*)size,4);
      }
    }
    else {
      if (nodesSize<=0x1f) {    // 2^5 (8 bits - 2 flag bit -1 continuation bit)
        uint8_t nodeSize1=(nodesSize & 0x1f) << 2;
        uint8_t size=coordSizeFlags | nodeSize1;

        Write(size);
      }
      else if (nodesSize<=0xfff) { // 2^(5+7)=2^12 (16-2-1-1)
        uint8_t size[2];
        uint8_t nodeSize1=((nodesSize & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
        uint8_t nodeSize2=nodesSize >> 5; // The final bits

        size[0]=coordSizeFlags | nodeSize1;
        size[1]=nodeSize2;

        Write((char*)size,2);
      }
      else if (nodesSize<=0x7ffff) { // 2^(12+7)=2^19 (24-2-1-1-1)
        uint8_t size[3];
        uint8_t nodeSize1=((nodesSize & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
        uint8_t nodeSize2=((nodesSize >> 5) & 0x7f) | 0x80; // Further 7 bits + continuation bit
        uint8_t nodeSize3=nodesSize >> 12; // The final bits

        size[0]=coordSizeFlags | nodeSize1;
        size[1]=nodeSize2;
        size[2]=nodeSize3;

        Write((char*)size,3);
      } else { // 2^(19+8)2^27 (32-2-1-1-1)
          uint8_t size[4];
          uint8_t nodeSize1=((nodesSize & 0x1f) << 2) | 0x80; // The initial 5 bits + continuation bit
          uint8_t nodeSize2=((nodesSize >> 5) & 0x7f) | 0x80; // Further 7 bits + continuation bit
          uint8_t nodeSize3=((nodesSize >> 12) & 0x7f) | 0x80; // further 7 bits + continuation bit
          uint8_t nodeSize4=nodesSize >> 19; // The final bits

          size[0]=coordSizeFlags | nodeSize1;
          size[1]=nodeSize2;
          size[2]=nodeSize3;
          size[3]=nodeSize4;

          Write((char*)size,4);
      }
    }

    //
    // Write data array
    //

    WriteCoord(nodes[0].GetCoord());

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

    Write((char*)byteBuffer.data(),byteBuffer.size());

    if (hasNodes) {
      size_t idCurrent=0;

      while (idCurrent<nodesSize) {
        uint8_t bitset=0;
        uint8_t bitMask=1;
        size_t idEnd=std::min(idCurrent+8,nodesSize);

        for (size_t i=idCurrent; i<idEnd; i++) {
          if (nodes[i].IsRelevant()) {
            bitset=bitset | bitMask;
          }

          bitMask*=2;
        }

        Write(bitset);

        for (size_t i=idCurrent; i<idEnd; i++) {
          if (nodes[i].IsRelevant()) {
            Write(nodes[i].GetSerial());
          }

          bitMask=bitMask*2;
        }

        idCurrent+=8;
      }

      /*
      size_t bytes=nodesSize;

      byteBuffer.resize(bytes);
      for (size_t i=0; i<bytes; i++) {
        byteBuffer[i]=nodes[i].GetSerial();
      }

      Write((char*)byteBuffer.data(),bytes);*/
    }
  }

/**
   *
   * @throws IOException
   */
  void FileWriter::WriteTypeId(TypeId id, uint8_t maxBytes)
  {
    if (maxBytes==1) {
      Write((uint8_t)id);
    }
    else if (maxBytes==2) {
      Write((uint8_t)(id/256));
      Write((uint8_t)(id%256));
    }
    else {
      assert(false);
    }
  }

  /**
   *
   * @throws IOException
   */
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

  /**
   *
   * @throws IOException
   */
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

  bool IsValidToWrite(const std::vector<Point>& nodes)
  {
    size_t nodeSize=nodes.size();
    if (nodeSize<=1) {
      return true;
    }

    uint32_t lastLat=(uint32_t)round((nodes[0].GetLat()+90.0)*latConversionFactor);
    uint32_t lastLon=(uint32_t)round((nodes[0].GetLon()+180.0)*lonConversionFactor);

    for (size_t i=1; i<nodeSize; i++) {
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

  /**
   *
   * @throws IOException
   */
  void ObjectFileRefStreamWriter::Write(const ObjectFileRef& ref)
  {
    // ObjectFileRefs must be sorted
    assert(ref.GetFileOffset()>=lastFileOffset);

    FileOffset offset=ref.GetFileOffset()-lastFileOffset;

    // Make room for two additional lower bits
    offset=offset << 2;

    offset=offset+ref.GetType();

    lastFileOffset=ref.GetFileOffset();

    writer.WriteNumber(offset);
  }

}

