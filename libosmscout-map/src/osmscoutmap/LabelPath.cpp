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

#include <osmscoutmap/LabelPath.h>
#include <osmscout/system/Math.h>
#include <osmscout/util/Geometry.h>

namespace osmscout {


  LabelPath::LabelPath(double minSegmentLength):
    length(0), minSegmentLength(minSegmentLength), endDistance(0)
  {
    offsetIndex.push_back(0);
  }

  LabelPath::~LabelPath()
  {
  }

  void LabelPath::AddPoint(const Vertex2D& point)
  {
    if (segments.empty()){
      end=point;
      Segment s={end,0,0,0};
      segments.push_back(s);
    }else{
      end=point;
      Segment last=segments.back();
      double endDistance = last.start.DistanceTo(point); //  QVector2D(last.start).distanceToPoint(QVector2D(x,y));
      if (endDistance>minSegmentLength){
        length+=endDistance;
        last.length=endDistance;
        last.angle=std::atan2(last.start.GetY()-point.GetY(),
                              point.GetX()-last.start.GetX());
        segments[segments.size()-1]=last;

        // fill offsetIndex
        for (size_t i=offsetIndex.size();i<length/100;i++){
          offsetIndex.push_back(segments.size()-1);
        }

        Segment s={end,length,0,0};
        segments.push_back(s);
      }
    }
  }

  Vertex2D LabelPath::PointAtLength(double offset) const
  {
    if (segments.empty()){
      return Vertex2D();
    }
    Segment  relevantSegment= SegmentBefore(offset);
    Vertex2D p              =relevantSegment.start;
    double   mul            = (offset-relevantSegment.offset);
    Vertex2D add(std::cos(relevantSegment.angle)*mul,-std::sin(relevantSegment.angle)*mul);

    return Vertex2D(p.GetX() + add.GetX(), p.GetY() + add.GetY());
  }

  const Segment& LabelPath::SegmentBefore(double offset) const
  {
    size_t hundred=offset/100;
    if (hundred>=offsetIndex.size())
      return segments.back();
    size_t i=offsetIndex[hundred];
    for (;i<segments.size();i++){
      const Segment &seg=segments[i];
      if (offset<(seg.offset+seg.length)){
        return seg;
      }
    }
    return segments.back();
  }

  double LabelPath::AngleAtLength(double offset) const
  {
    return SegmentBefore(offset).angle;
  }

  double LabelPath::AngleAtLengthDeg(double offset) const
  {
    return (AngleAtLength(offset) * 180) / M_PI;
  }

  bool LabelPath::TestAngleVariance(double startOffset, double endOffset, double maximumAngle) const
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
          if (AngleDiff(initialAngle,seg.angle) > maximumAngle){
            return false;
          }
        }
      }
    }
    return true;
  }

}
