#ifndef OSMSCOUT_UTIL_TRANSFORMATION_H
#define OSMSCOUT_UTIL_TRANSFORMATION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2011  Tim Teulings

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

// For memcpy
#include <string.h>

#include <iostream>
#include <vector>

#include <osmscout/private/CoreImportExport.h>

#include <osmscout/GeoCoord.h>
#include <osmscout/Pixel.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/Projection.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

namespace osmscout {

  class OSMSCOUT_API TransPolygon
  {
  private:
    size_t  pointsSize;
    size_t  length;
    size_t  start;
    size_t  end;

  public:
    enum OptimizeMethod
    {
      none = 0,
      fast = 1,
      quality = 2
    };

    struct OSMSCOUT_API TransPoint
    {
      bool   draw;
      double x;
      double y;
    };

  public:
    TransPoint* points;

  private:
    void TransformGeoToPixel(const Projection& projection,
                             const std::vector<GeoCoord>& nodes);
    void DropSimilarPoints(double optimizeErrorTolerance);
    void DropRedundantPointsFast(double optimizeErrorTolerance);
    void DropRedundantPointsDouglasPeucker(double optimizeErrorTolerance, bool isArea);

  public:
    TransPolygon();
    virtual ~TransPolygon();

    inline bool IsEmpty() const
    {
      return length==0;
    }

    inline size_t GetLength() const
    {
      return length;
    }

    inline size_t GetStart() const
    {
      return start;
    }

    inline size_t GetEnd() const
    {
      return end;
    }

    void TransformArea(const Projection& projection,
                       OptimizeMethod optimize,
                       const std::vector<GeoCoord>& nodes,
                       double optimizeErrorTolerance);

    void TransformWay(const Projection& projection,
                      OptimizeMethod optimize,
                      const std::vector<GeoCoord>& nodes,
                      double optimizeErrorTolerance);

    bool GetBoundingBox(double& xmin, double& ymin,
                        double& xmax, double& ymax) const;

    bool GetCenterPixel(double& cx,
                        double& cy) const;
  };

  class OSMSCOUT_API CoordBuffer
  {
  public:
    virtual ~CoordBuffer();

    virtual void Reset() = 0;
    virtual size_t PushCoord(double x, double y) = 0;
    virtual size_t GetLength() const = 0;
    virtual bool GenerateParallelWay(size_t orgStart,
                                     size_t orgEnd,
                                     double offset,
                                     size_t& start,
                                     size_t& end) = 0;
    virtual void ScanConvertLine(size_t start,
                                 size_t end,
                                 std::vector<ScanCell>& cells) = 0;
  };

  template<class P>
  class OSMSCOUT_API CoordBufferImpl : public CoordBuffer
  {
  private:
    size_t bufferSize;
    size_t usedPoints;

  public:
    P*     buffer;

  public:
    CoordBufferImpl();
    ~CoordBufferImpl();

    void Reset();
    size_t PushCoord(double x, double y);
    size_t GetLength() const;

    bool GenerateParallelWay(size_t orgStart,
                             size_t orgEnd,
                             double offset,
                             size_t& start,
                             size_t& end);

    void ScanConvertLine(size_t start,
                         size_t end,
                         std::vector<ScanCell>& cells);

/*
    void GetBoundingBox(size_t start, size_t end,
                        double& xmin, double& ymin,
                        double& xmax, double& ymax) const;

    void GetCenterPixel(size_t start, size_t end,
                        double& cx,
                        double& cy) const;*/
  };

  template<class P>
  CoordBufferImpl<P>::CoordBufferImpl()
  : bufferSize(131072),
    usedPoints(0),
    buffer(new P[bufferSize])
  {
    // no code
  }

  template<class P>
  CoordBufferImpl<P>::~CoordBufferImpl()
  {
    delete buffer;
  }

  template<class P>
  void CoordBufferImpl<P>::Reset()
  {
    usedPoints=0;
  }

  template<class P>
  size_t CoordBufferImpl<P>::PushCoord(double x, double y)
  {
    if (usedPoints>=bufferSize)
    {
      bufferSize=bufferSize*2;

      P* newBuffer=new P[bufferSize];

      memcpy(newBuffer,buffer,sizeof(P)*usedPoints);

      std::cout << "*** Buffer reallocation: " << bufferSize << std::endl;

      delete [] buffer;

      buffer=newBuffer;
    }

    buffer[usedPoints].Set(x,y);

    return usedPoints++;
  }

  template<class P>
  size_t CoordBufferImpl<P>::GetLength() const
  {
    return usedPoints;
  }

  template<class P>
  bool CoordBufferImpl<P>::GenerateParallelWay(size_t orgStart,
                                               size_t orgEnd,
                                               double offset,
                                               size_t& start,
                                               size_t& end)
  {
    if (orgStart+1>orgEnd) {
      // To avoid "not initialized" warnings
      return false;
    }

    double oax,oay;
    double obx,oby;


    Normalize(buffer[orgStart].GetY()-buffer[orgStart+1].GetY(),
              buffer[orgStart+1].GetX()-buffer[orgStart].GetX(),
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    start=PushCoord(buffer[orgStart].GetX()+oax,
                    buffer[orgStart].GetY()+oay);

    for (size_t i=orgStart+1; i<orgEnd; i++) {
      Normalize(buffer[i-1].GetY()-buffer[i].GetY(),
                buffer[i].GetX()-buffer[i-1].GetX(),
                oax, oay);

      oax=offset*oax;
      oay=offset*oay;

      Normalize(buffer[i].GetY()-buffer[i+1].GetY(),
                buffer[i+1].GetX()-buffer[i].GetX(),
                obx, oby);

      obx=offset*obx;
      oby=offset*oby;


      double det1=Det(obx-oax,
                      oby-oay,
                      buffer[i+1].GetX()-buffer[i].GetX(),
                      buffer[i+1].GetY()-buffer[i].GetY());
      double det2=Det(buffer[i].GetX()-buffer[i-1].GetX(),
                      buffer[i].GetY()-buffer[i-1].GetY(),
                      buffer[i+1].GetX()-buffer[i].GetX(),
                      buffer[i+1].GetY()-buffer[i].GetY());

      if (fabs(det2)>0.0001) {
        PushCoord(buffer[i].GetX()+oax+det1/det2*(buffer[i].GetX()-buffer[i-1].GetX()),
                  buffer[i].GetY()+oay+det1/det2*(buffer[i].GetY()-buffer[i-1].GetY()));
      }
      else {
        PushCoord(buffer[i].GetX()+oax,
                  buffer[i].GetY()+oay);
      }
    }

    Normalize(buffer[orgEnd-1].GetY()-buffer[orgEnd].GetY(),
              buffer[orgEnd].GetX()-buffer[orgEnd-1].GetX(),
              oax, oay);

    oax=offset*oax;
    oay=offset*oay;

    end=PushCoord(buffer[orgEnd].GetX()+oax,
                  buffer[orgEnd].GetY()+oay);

    return true;
  }

  template<class P>
  void CoordBufferImpl<P>::ScanConvertLine(size_t start,
                                           size_t end,
                                           std::vector<ScanCell>& cells)
  {
    for (size_t i=start; i<end; i++) {
      size_t j=i+1;

      int x1=int(buffer[i].GetX());
      int x2=int(buffer[j].GetX());
      int y1=int(buffer[i].GetY());
      int y2=int(buffer[j].GetY());

      osmscout::ScanConvertLine(x1,y1,
                                x2,y2,
                                cells);
    }
  }

  /*
  template<class P>
  void CoordBufferImpl<P>::GetBoundingBox(size_t start, size_t end,
                                          double& xmin, double& ymin,
                                          double& xmax, double& ymax) const
  {
    xmin=buffer[start].GetX();
    xmax=xmin;
    ymin=buffer[start].GetY();
    ymax=ymin;

    for (size_t i=start+1; i<=end; i++) {
      xmin=std::min(xmin,buffer[i].GetX());
      xmax=std::max(xmax,buffer[i].GetX());
      ymin=std::min(ymin,buffer[i].GetY());
      ymax=std::max(ymax,buffer[i].GetY());
    }
  }

  template<class P>
  void CoordBufferImpl<P>::GetCenterPixel(size_t start, size_t end,
                                          double& cx,
                                          double& cy) const
  {
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    GetBoundingBox(start,end,xmin,ymin,xmax,ymax);

    cx=xmin+(xmax-xmin)/2;
    cy=ymin+(ymax-ymin)/2;
  }*/

  class OSMSCOUT_API TransBuffer
  {
  public:
    TransPolygon transPolygon;
    CoordBuffer *buffer;

  public:
    TransBuffer(CoordBuffer* buffer);
    virtual ~TransBuffer();

    void Reset();

    void TransformArea(const Projection& projection,
                       TransPolygon::OptimizeMethod optimize,
                       const std::vector<GeoCoord>& nodes,
                       size_t& start, size_t &end,
                       double optimizeErrorTolerance);
    bool TransformWay(const Projection& projection,
                      TransPolygon::OptimizeMethod optimize,
                      const std::vector<GeoCoord>& nodes,
                      size_t& start, size_t &end,
                      double optimizeErrorTolerance);
  };
}

#endif
