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
#include <QVector2D>
#include <math.h>

namespace osmscout {

  SimplifiedPath::SimplifiedPath(qreal minSegmentLength):
    length(0), minSegmentLength(minSegmentLength), endDistance(0)
  {
  }

  SimplifiedPath::~SimplifiedPath()
  {
  }

  void SimplifiedPath::AddPoint(qreal x,qreal y)
  {
    if (segments.empty()){
      end.setX(x);
      end.setY(y);
      Segment s={end,0,0,0};
      segments<<s;
    }else{
      end.setX(x);
      end.setY(y);
      Segment last=segments.last();
      float endDistance=QVector2D(last.start).distanceToPoint(QVector2D(x,y));
      if (endDistance>minSegmentLength){
        length+=endDistance;
        last.length=endDistance;
        last.angle=std::atan2(last.start.y()-y,x-last.start.x());
        segments[segments.size()-1]=last;
        Segment s={end,length,0,0};
        segments<<s;
        endDistance=0;
      }
    }
  }

  QPointF SimplifiedPath::PointAtLength(qreal offset) const
  {
    if (segments.isEmpty()){
      return QPointF();
    }
    Segment relevantSetment=segmentBefore(offset);
    QPointF p=relevantSetment.start;
    QPointF add(std::cos(relevantSetment.angle), -std::sin(relevantSetment.angle));
    
    return p + add*(offset-relevantSetment.offset);
  }

  const Segment& SimplifiedPath::segmentBefore(qreal offset) const
  {
    for (const Segment &seg:segments){
      if (offset<(seg.offset+seg.length)){
        return seg;
      }
    }
    return segments.last();
  }

  qreal SimplifiedPath::AngleAtLength(qreal offset) const
  {
    return segmentBefore(offset).angle;
  }

  qreal SimplifiedPath::AngleAtLengthDeg(qreal offset) const
  {
    return (AngleAtLength(offset) * 180) / M_PI;
  }
}