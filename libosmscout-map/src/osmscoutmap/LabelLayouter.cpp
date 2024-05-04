/*
  This source is part of the libosmscout-map library
  Copyright (C) 2016  Tim Teulings

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

#include <osmscoutmap/LabelLayouter.h>

namespace osmscout {
  ContourLabelPositioner::Position ContourLabelPositioner::calculatePositions(const Projection& /*projection*/,
                                                                              const MapParameter& /*parameter*/,
                                                                              const PathLabelData& labelData,
                                                                              double pathLength,
                                                                              double labelWidth) const
  {
    double minimalSpace       =2*labelData.contourLabelOffset;
    size_t countLabels        =0;
    size_t labelCountIncrement=0;

    while (true) {
      size_t newLabelCount;

      if (countLabels==0) {
        newLabelCount      =1;
      }
      else if (countLabels==1) {
        newLabelCount      =3;
      }
      else if (countLabels==3) {
        newLabelCount      =5;
        labelCountIncrement=newLabelCount-1;
      }
      else {
        newLabelCount=countLabels+labelCountIncrement;
        labelCountIncrement=newLabelCount-1;
      }

      double length=minimalSpace+
                    double(newLabelCount-1)*labelData.contourLabelSpace+
                    double(newLabelCount)*labelWidth;

      if (length>pathLength) {
        // labels + spaces exceed the length of the path
        break;
      }

      countLabels=newLabelCount;
    }

    double offset;
    double labelSpace;

    if (countLabels==0) {
      if (labelWidth>pathLength) {
        return {0,0.0,0.0};
      }

      countLabels=1;
    }

    if (countLabels==1) {
      // If we have one label, we center it

      offset=(pathLength-labelWidth)/2;
      labelSpace=0.0;
    }
    else {
      // else we increase the labelSpace

      double offsetsSpace    =2*labelData.contourLabelOffset;
      double labelsWidth     =double(countLabels)*labelWidth;
      double minLabelsSpace  =double(countLabels-1)*labelData.contourLabelSpace;
      double minPathSpace    =minLabelsSpace;
      double currentPathSpace=pathLength-labelsWidth-offsetsSpace;
      double scaleFactor     =currentPathSpace/minPathSpace;

      assert(scaleFactor>=1.0);

      offset=labelData.contourLabelOffset;
      labelSpace=labelData.contourLabelSpace*scaleFactor;

      assert(labelSpace>=labelData.contourLabelSpace);
    }

    assert(offset>=0);
    assert((countLabels%2)!=0);

    return {countLabels,offset,labelSpace};
  }
}
