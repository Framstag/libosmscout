
#ifndef POINTSEQUENCE_H
#define	POINTSEQUENCE_H

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

#include <memory>
#include <vector>

#include <osmscout/GeoCoord.h>
#include <osmscout/util/GeoBox.h>
#include <osmscout/Point.h>

namespace osmscout {

  class PointSequenceIteratorPriv
  {
  public:      
      virtual inline ~PointSequenceIteratorPriv(){};
      
      virtual const Point operator*() const = 0;
      
      virtual void operator++() = 0;
      
      virtual bool operator==(const PointSequenceIteratorPriv *another) const = 0;
      
      virtual bool inline operator!=(const PointSequenceIteratorPriv *another) const 
      {
          return ! operator==(another);
      };
  };
  
  /**
   * STL like iterator
   */
  class OSMSCOUT_API PointSequenceIterator
  {
  private:
      PointSequenceIteratorPriv *priv; 
  public:
      inline PointSequenceIterator(PointSequenceIteratorPriv *priv):priv(priv){};
      inline ~PointSequenceIterator(){ delete priv;};
      inline const Point operator*() const { return priv->operator *();};
      inline void operator++(){priv->operator ++();};
      inline bool operator==(const PointSequenceIterator& another) const {return priv->operator ==(another.priv);};
      inline bool operator!=(const PointSequenceIterator& another) const {return priv->operator !=(another.priv);};
  };

  class OSMSCOUT_API PointSequence
  {
  protected:
      mutable osmscout::GeoBox *bboxPtr;

  public:
      inline PointSequence(): bboxPtr(NULL) {}
      virtual inline ~PointSequence() 
      {
          if (bboxPtr!=NULL)
              delete bboxPtr;
      };
      
      virtual const Point operator[](size_t i) const = 0;
      virtual PointSequenceIterator begin() const = 0;
      virtual PointSequenceIterator end() const = 0;
      virtual const Point front() const = 0;
      virtual const Point back() const = 0;
      virtual size_t size() const = 0;
      virtual bool empty() const = 0;
      virtual const osmscout::GeoBox bbox() const;
  };  
  
  class OSMSCOUT_API VectorPointSequence : public PointSequence
  {
  private:
      std::vector<Point>    nodes;
  public:
      inline VectorPointSequence(std::vector<Point> nodes): 
        PointSequence(), nodes(nodes){};
      virtual inline ~VectorPointSequence(){};

      class VectorPointSequenceIteratorPriv: public PointSequenceIteratorPriv
      {
      private:
          std::vector<Point>::const_iterator it;
      public:
          inline VectorPointSequenceIteratorPriv(std::vector<Point>::const_iterator it): it(it) {}
          virtual inline const Point operator*() const {return *it;};
          virtual inline void operator++(){ it++; };
          virtual inline bool operator==(const PointSequenceIteratorPriv *another) const 
          {
              const VectorPointSequenceIteratorPriv *vectorBased = dynamic_cast<const VectorPointSequenceIteratorPriv*> (another);
              if (vectorBased==NULL)
                  return false;
              return it == vectorBased->it;
          }      
      };
        
      virtual inline const Point operator[](size_t i) const {return nodes[i];}
      
      virtual inline PointSequenceIterator begin() const 
      {
          return PointSequenceIterator(new VectorPointSequenceIteratorPriv(nodes.begin()));
      };
      
      virtual inline PointSequenceIterator end() const 
      {
          return PointSequenceIterator(new VectorPointSequenceIteratorPriv(nodes.end()));
      };
      
      virtual inline const Point front() const {return nodes.front();};
      virtual inline const Point back() const {return nodes.back();};
      virtual inline size_t size() const { return nodes.size(); }
      virtual inline bool empty() const {return nodes.empty(); };
  };
  
  /**
   * Like VectorPointSequence, but it don't create own copy of node vector,
   * it retrieve pointer to node vector but don't take its ownership!
   */
  class TempVectorPointSequence : public PointSequence
  {
  private:
      const std::vector<Point>    *nodes;
  public:
      inline TempVectorPointSequence(const std::vector<Point> *nodes): 
        PointSequence(), nodes(nodes){};
      virtual inline ~TempVectorPointSequence(){};

      virtual inline const Point operator[](size_t i) const {return nodes->operator [](i);}
      
      virtual inline PointSequenceIterator begin() const 
      {
          return PointSequenceIterator(new VectorPointSequence::VectorPointSequenceIteratorPriv(nodes->begin()));
      };
      
      virtual inline PointSequenceIterator end() const 
      {
          return PointSequenceIterator(new VectorPointSequence::VectorPointSequenceIteratorPriv(nodes->end()));
      };
      
      virtual inline const Point front() const {return nodes->front();};
      virtual inline const Point back() const {return nodes->back();};
      virtual inline size_t size() const { return nodes->size(); }
      virtual inline bool empty() const {return nodes->empty(); };
  };
  
  class MMapPointSequence : public PointSequence
  {
  private:
    char *ptr;
    size_t coordBitSize;
    bool   hasNodes;
    size_t nodeCount;      
          
    class MMapPointSequenceIteratorPriv: public PointSequenceIteratorPriv
    {
        friend class MMapPointSequence;
        
    private:
        const MMapPointSequence *sequence;
        size_t position;
        uint32_t latValue;
        uint32_t lonValue;
        char *nodeSerialPtr;
        uint8_t serialBits;
        
        // cached last value to avoid creating many temporary Points
        mutable size_t lastPosition;
        mutable Point lastPoint;
        
        inline uint8_t serialMask() const { return (1 << (position % 8));};
        void readCoordDelta16(int32_t &latDelta, int32_t &lonDelta);
        void readCoordDelta32(int32_t &latDelta, int32_t &lonDelta);
        void readCoordDelta48(int32_t &latDelta, int32_t &lonDelta);
        
    public:
        MMapPointSequenceIteratorPriv(const MMapPointSequence *sequence);
        MMapPointSequenceIteratorPriv(const MMapPointSequence *sequence, 
            size_t position, uint32_t latValue, uint32_t lonValue,  
            char *nodeSerialPtr, uint8_t serialBits);
        virtual inline ~MMapPointSequenceIteratorPriv(){};
        
        virtual const Point operator*() const ;
        virtual void operator++();
        virtual bool operator==(const PointSequenceIteratorPriv *another) const;
    };
    friend class MMapPointSequenceIteratorPriv;

    mutable MMapPointSequenceIteratorPriv *internalIterator;
    mutable Point *frontPoint;
    mutable Point *backPoint;
    
#ifndef SWIG // It seems that Swig has problems with inline methods: "Error: Syntax error in input"
    const inline static Point createPoint(uint8_t serial, uint32_t latValue, uint32_t lonValue);
#endif
    
  public:
    MMapPointSequence(char *ptr, size_t coordBitSize, bool hasNodes, size_t nodeCount);    
    virtual ~MMapPointSequence();
    virtual const Point operator[](size_t i) const ;
    
    virtual PointSequenceIterator begin() const ;      
    virtual PointSequenceIterator end() const ;

    virtual const Point front() const ;
    virtual const Point back() const ;
    virtual inline size_t size() const { return nodeCount; }
    virtual inline bool empty() const {return nodeCount == 0; };

    virtual const osmscout::GeoBox bbox() const;
  };
}

#endif	/* POINTSEQUENCE_H */

