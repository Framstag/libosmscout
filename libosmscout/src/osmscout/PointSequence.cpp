/*
  This source is part of the libosmscout library
  Copyright (C) 2016  Lukas Karas

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

#include <assert.h>
#include <exception>

#include <osmscout/PointSequence.h>

#include <osmscout/util/Geometry.h>
#include "osmscout/util/GeoBox.h"

namespace osmscout {

  const GeoBox PointSequence::bbox() const
  {
    if (bboxPtr != NULL){
        return *bboxPtr;
    }
    
    PointSequenceIterator it = begin();
    Point point = *it;
    
    // Bounding box
    double lonMin=point.GetLon();
    double lonMax=lonMin;
    double latMin=point.GetLat();
    double latMax=latMin;

    for (++it; it!=end(); ++it) {
      point = *it;
      
      lonMin=std::min(lonMin,point.GetLon());
      lonMax=std::max(lonMax,point.GetLon());
      latMin=std::min(latMin,point.GetLat());
      latMax=std::max(latMax,point.GetLat());
    }
    bboxPtr = new GeoBox(GeoCoord(latMin, lonMin), GeoCoord(latMax, lonMax));
    return *bboxPtr;
  }
   
  const std::vector<Point> PointSequence::asVector() const
  {
      std::vector<Point> result;
      for (const auto& p: *this){
          result.push_back(p);
      }
      return result;
  }

  MMapPointSequence::MMapPointSequence(char *ptr, size_t coordBitSize, bool hasNodes, size_t nodeCount):
          ptr(ptr),
          coordBitSize(coordBitSize), 
          hasNodes(hasNodes), 
          nodeCount(nodeCount),
          
          internalIterator(NULL),
          frontPoint(NULL), 
          backPoint(NULL)
  {
      
  }
  MMapPointSequence::~MMapPointSequence()
  {
      if (internalIterator != NULL)
          delete internalIterator;
      if (frontPoint != NULL)
          delete frontPoint;
      if (backPoint != NULL)
          delete backPoint;
  }

  const Point MMapPointSequence::createPoint(uint8_t serial, uint32_t latValue, uint32_t lonValue)
  {
      return Point(serial, 
              GeoCoord(
              latValue/latConversionFactor-90.0,
              lonValue/lonConversionFactor-180.0));
  }
  
  const Point MMapPointSequence::operator[](size_t i) const 
  {
      if (i >= nodeCount){
          throw std::out_of_range("PointSequence index out of range");
      }
      if (internalIterator != NULL && i < internalIterator->position){
          delete internalIterator;
          internalIterator = NULL;
      }
      if (internalIterator == NULL)
          internalIterator = new MMapPointSequenceIteratorPriv(this);
      while (i > internalIterator->position){
          internalIterator->operator ++();
      }
      if (i == internalIterator->position)
          return internalIterator->operator *();
      assert(false);
  }
  
  PointSequenceIterator MMapPointSequence::begin() const
  {
      return PointSequenceIterator(new MMapPointSequenceIteratorPriv(this));
  }
  
  PointSequenceIterator MMapPointSequence::end() const
  {
      return PointSequenceIterator(new MMapPointSequenceIteratorPriv(this, nodeCount, 0, 0, NULL, 0));
  }
  
  const Point MMapPointSequence::front() const 
  {
      if (empty()){
          throw std::out_of_range("PointSequence is empty");
      }
      if (frontPoint == NULL){
          Point p = this->operator [](0);
          frontPoint = new Point(p.GetSerial(), p.GetCoord());
      }
      return *frontPoint;
  }
  
  const Point MMapPointSequence::back() const 
  {
      if (empty()){
          throw std::out_of_range("PointSequence is empty");
      }
      if (nodeCount==1){
          return front();
      }
      if (backPoint == NULL){
          Point p = this->operator [](nodeCount -1);
          backPoint = new Point(p.GetSerial(), p.GetCoord());
      }
      return *backPoint;
  }

  MMapPointSequence::MMapPointSequenceIteratorPriv::MMapPointSequenceIteratorPriv(const MMapPointSequence *sequence):
      sequence(sequence), position(0), lastPosition(-1)
  {

      if (sequence->nodeCount >0){
        // GeoCoord firstCoord;
        uint8_t *dataPtr = (uint8_t*)sequence->ptr;

        latValue=  ((unsigned char) dataPtr[0] <<  0)
               | ((unsigned char) dataPtr[1] <<  8)
               | ((unsigned char) dataPtr[2] << 16)
               | ((unsigned char) (dataPtr[6] & 0x0f) << 24);

        lonValue=  ((unsigned char) dataPtr[3] <<  0)
               | ((unsigned char) dataPtr[4] <<  8)
               | ((unsigned char) dataPtr[5] << 16)
               | ((unsigned char) (dataPtr[6] & 0xf0) << 20);

        if (sequence->hasNodes){
            size_t byteBufferSize=(sequence->nodeCount-1)*sequence->coordBitSize/8;
            nodeSerialPtr = sequence->ptr + coordByteSize + byteBufferSize;
            serialBits = *nodeSerialPtr;
            nodeSerialPtr++;
        }
      }
  }
  
  MMapPointSequence::MMapPointSequenceIteratorPriv::MMapPointSequenceIteratorPriv(const MMapPointSequence *sequence, 
    size_t position, uint32_t latValue, uint32_t lonValue,
    char *nodeSerialPtr, uint8_t serialBits):
          sequence(sequence), 
          position(position), 
          latValue(latValue),
          lonValue(lonValue), 
          nodeSerialPtr(nodeSerialPtr), 
          serialBits(serialBits),
          lastPosition(-1)
  {
      
  }

  const Point MMapPointSequence::MMapPointSequenceIteratorPriv::operator*() const 
  {
      if (position == lastPosition){
          return lastPoint;
      }

      uint8_t serial = 0;
      if (sequence->hasNodes) {
          if (serialBits & serialMask()){
              serial = *nodeSerialPtr;
          }
      }
      lastPoint = MMapPointSequence::createPoint(serial, latValue, lonValue);
      lastPosition = position;
      return lastPoint;
  }
  
  void MMapPointSequence::MMapPointSequenceIteratorPriv::operator++()
  {
      if (position >= sequence->nodeCount -1){ 
          position = sequence->nodeCount; // end reached
          return;
      }
      if (sequence->hasNodes) {
          uint8_t mask = serialMask();
          if (serialBits & mask){
              nodeSerialPtr++;
          }
          if (mask == 0x80){
              // FIXME: unchecked buffer bounds!
              serialBits = *nodeSerialPtr;
              nodeSerialPtr++;
          }
      }
      position++;
      
      int32_t  latDelta;
      int32_t  lonDelta;
      if (sequence->coordBitSize==16) {
          readCoordDelta16(latDelta, lonDelta);         
      }else if (sequence->coordBitSize==32) {
          readCoordDelta32(latDelta, lonDelta);          
      } else { // coordBitSize==48;
          readCoordDelta48(latDelta, lonDelta);
      }
      latValue+=latDelta;
      lonValue+=lonDelta;
  }
  
  void MMapPointSequence::MMapPointSequenceIteratorPriv::readCoordDelta16(int32_t &latDelta, int32_t &lonDelta)
  {
      uint8_t *byteBuffer = (uint8_t *)sequence->ptr + coordByteSize + ((position-1)*2);
              
      latDelta=(int8_t)byteBuffer[0];
      lonDelta=(int8_t)byteBuffer[1];
  }
  
  void MMapPointSequence::MMapPointSequenceIteratorPriv::readCoordDelta32(int32_t &latDelta, int32_t &lonDelta)
  {
    uint8_t *byteBuffer = (uint8_t *)sequence->ptr + coordByteSize + ((position-1)*4);
      
    uint32_t latUDelta=byteBuffer[0] | (byteBuffer[1]<<8);
    uint32_t lonUDelta=byteBuffer[2] | (byteBuffer[3]<<8);

    if (latUDelta & 0x8000) {
      latDelta=(int32_t)(latUDelta | 0xffff0000);
    }
    else {
      latDelta=(int32_t)latUDelta;
    }

    if (lonUDelta & 0x8000) {
      lonDelta=(int32_t)(lonUDelta | 0xffff0000);
    }
    else {
      lonDelta=(int32_t)lonUDelta;
    }
      
  }
  
  void MMapPointSequence::MMapPointSequenceIteratorPriv::readCoordDelta48(int32_t &latDelta, int32_t &lonDelta)
  {
    uint8_t *byteBuffer = (uint8_t *)sequence->ptr + coordByteSize + ((position-1)*6);
    
    uint32_t latUDelta=(byteBuffer[0]) | (byteBuffer[1]<<8) | (byteBuffer[2]<<16);
    uint32_t lonUDelta=(byteBuffer[3]) | (byteBuffer[4]<<8) | (byteBuffer[5]<<16);

    if (latUDelta & 0x800000) {
      latDelta=(int32_t)(latUDelta | 0xff000000);
    }
    else {
      latDelta=(int32_t)latUDelta;
    }

    if (lonUDelta & 0x800000) {
      lonDelta=(int32_t)(lonUDelta | 0xff000000);
    }
    else {
      lonDelta=(int32_t)lonUDelta;
    }
  }
  
  bool MMapPointSequence::MMapPointSequenceIteratorPriv::operator==(const PointSequenceIteratorPriv *another) const
  {
    const MMapPointSequenceIteratorPriv *mmapBased = dynamic_cast<const MMapPointSequenceIteratorPriv*> (another);
    if (mmapBased==NULL)
        return false;
    return this->sequence == mmapBased->sequence && this->position == mmapBased->position;
  }

  const GeoBox MMapPointSequence::bbox() const
  {
    if (bboxPtr != NULL){
        return *bboxPtr;
    }
    
    MMapPointSequenceIteratorPriv it(this);

    uint32_t lonMin=it.lonValue;
    uint32_t lonMax=lonMin;
    uint32_t latMin=it.latValue;
    uint32_t latMax=latMin;

    for (++it; it.position < nodeCount; ++it) {
        
      lonMin=std::min(lonMin,it.lonValue);
      lonMax=std::max(lonMax,it.lonValue);
      latMin=std::min(latMin,it.latValue);
      latMax=std::max(latMax,it.latValue);
    }
    bboxPtr = new GeoBox( 
            createPoint(0, latMin, lonMin).GetCoord(), 
            createPoint(0, latMax, lonMax).GetCoord());
    return *bboxPtr;
  }
  
  void PointSequenceContainer::GetBoundingBox(GeoBox& boundingBox) const
  {
    osmscout::GetBoundingBox(*nodes, boundingBox);
  }  
}

