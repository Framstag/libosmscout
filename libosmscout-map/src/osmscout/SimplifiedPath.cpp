/*
  This source is part of the libosmscout-map library
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/SimplifiedPath.h>
#include <osmscout/system/Math.h>
#include <osmscout/util/Geometry.h>

namespace osmscout {


  SimplifiedPath::SimplifiedPath(double minSegmentLength):
    length(0), minSegmentLength(minSegmentLength), endDistance(0)
  {
    offsetIndex.push_back(0);
  }

  SimplifiedPath::~SimplifiedPath()
  {
  }

  void SimplifiedPath::AddPoint(double x,double y)
  {
    if (segments.empty()){
      end.Set(x,y);
      Segment s={end,0,0,0};
      segments.push_back(s);
    }else{
      end.Set(x,y);
      Segment last=segments.back();
      float endDistance= last.start.DistanceTo(Vertex2D(x,y)); //  QVector2D(last.start).distanceToPoint(QVector2D(x,y));
      if (endDistance>minSegmentLength){
        length+=endDistance;
        last.length=endDistance;
        last.angle=std::atan2(last.start.GetY()-y,x-last.start.GetX());
        segments[segments.size()-1]=last;

        // fill offsetIndex
        for (int i=offsetIndex.size();i<length/100;i++){
          offsetIndex.push_back(segments.size()-1);
        }

        Segment s={end,length,0,0};
        segments.push_back(s);
        //endDistance=0;
      }
    }
  }

  Vertex2D SimplifiedPath::PointAtLength(double offset) const
  {
    if (segments.empty()){
      return Vertex2D();
    }
    Segment relevantSetment=segmentBefore(offset);
    Vertex2D p=relevantSetment.start;
    double mul = (offset-relevantSetment.offset);
    Vertex2D add(std::cos(relevantSetment.angle) * mul, -std::sin(relevantSetment.angle) * mul);
    
    return Vertex2D(p.GetX() + add.GetX(), p.GetY() + add.GetY());
  }

  const Segment& SimplifiedPath::segmentBefore(double offset) const
  {
    int hundred=offset/100;
    if (hundred>=offsetIndex.size())
      return segments.back();
    int i=offsetIndex[hundred];
    for (;i<segments.size();i++){
      const Segment &seg=segments[i];
      if (offset<(seg.offset+seg.length)){
        return seg;
      }
    }
    return segments.back();
  }

  double SimplifiedPath::AngleAtLength(double offset) const
  {
    return segmentBefore(offset).angle;
  }

  double SimplifiedPath::AngleAtLengthDeg(double offset) const
  {
    return (AngleAtLength(offset) * 180) / M_PI;
  }

  bool SimplifiedPath::TestAngleVariance(double startOffset, double endOffset, double maximumAngle)
  {
    double initialAngle=0;
    bool initialised=false;
    for (const Segment &seg:segments){
      if (seg.offset>endOffset){
        return true;
      }
      if (seg.offset+seg.length>startOffset){
        if (!initialised){
          initialAngle=seg.angle;
          initialised=true;
        }else{
          double change=std::abs(initialAngle-seg.angle);
          if (change>M_PI)
            change-=M_PI;
          if (change>maximumAngle){
            return false;
          }
        }
      }
    }
    return true;
  }

}