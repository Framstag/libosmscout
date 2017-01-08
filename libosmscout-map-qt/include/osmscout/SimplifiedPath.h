#ifndef OSMSCOUT_MAP_SIMPLIFIEDPATH_H
#define OSMSCOUT_MAP_SIMPLIFIEDPATH_H

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

#include <QObject>
#include <QPointF>
#include <QVector>

#include <osmscout/private/MapQtImportExport.h>

namespace osmscout {
  struct Segment
  {
    QPointF start;
    qreal offset;
    qreal length;
    qreal angle;
  };

  class OSMSCOUT_MAP_QT_API SimplifiedPath{
  private:
    qreal length;
    QVector<Segment> segments;
    QVector<int> offsetIndex; // segment offset by length 100
    qreal minSegmentLength;
    QPointF end;
    qreal endDistance;

  public:
    SimplifiedPath(qreal minSegmentLength=5);
    virtual ~SimplifiedPath();
    void AddPoint(qreal x,qreal y);
    inline qreal GetLength() const {
      return length+endDistance;
    }
    QPointF PointAtLength(qreal offset) const;
    qreal AngleAtLength(qreal offset) const;
    qreal AngleAtLengthDeg(qreal offset) const;

    bool TestAngleVariance(qreal startOffset, qreal endOffset, qreal maximumAngle);
  private:
    const Segment& segmentBefore(qreal offset) const;
  };
}

#endif /* OSMSCOUT_MAP_SIMPLIFIEDPATH_H */
