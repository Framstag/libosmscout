/*
  This source is part of the libosmscout library
  Copyright (C) 2015  Tim Teulings

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

#include <osmscout/util/GeoBox.h>

#include <algorithm>

#include <osmscout/system/Assert.h>

#include <osmscout/util/Geometry.h>

namespace osmscout {

  /**
   * The default constructor creates an invalid instance.
   */
  GeoBox::GeoBox()
  : minCoord(0.0,0.0),
    maxCoord(0.0,0.0),
    valid(false)
  {
    // no code
  }

  GeoBox::GeoBox(const GeoCoord& coordA,
                 const GeoCoord& coordB)
  : minCoord(std::min(coordA.GetLat(),coordB.GetLat()),
             std::min(coordA.GetLon(),coordB.GetLon())),
    maxCoord(std::max(coordA.GetLat(),coordB.GetLat()),
             std::max(coordA.GetLon(),coordB.GetLon())),
    valid(true)
  {
    // no code
  }

  void GeoBox::Set(const GeoCoord& coordA,
                   const GeoCoord& coordB)
  {
    minCoord.Set(std::min(coordA.GetLat(),coordB.GetLat()),
                 std::min(coordA.GetLon(),coordB.GetLon()));
    maxCoord.Set(std::max(coordA.GetLat(),coordB.GetLat()),
                 std::max(coordA.GetLon(),coordB.GetLon()));
    valid=true;
  }

  /**
   * Changes the GeoBox to include the other bounding box, too (calculation of the common rectangular hull)
   *
   * @param other
   *    Other geoBox to include, too
   */
  void GeoBox::Include(const GeoBox& other)
  {
    assert(valid);
    assert(other.valid);

    minCoord.Set(std::min(minCoord.GetLat(),
                          other.GetMinCoord().GetLat()),
                 std::min(minCoord.GetLon(),
                          other.GetMinCoord().GetLon()));

    maxCoord.Set(std::max(maxCoord.GetLat(),
                          other.GetMaxCoord().GetLat()),
                 std::max(maxCoord.GetLon(),
                          other.GetMaxCoord().GetLon()));
  }

  GeoBox GeoBox::Intersection(const GeoBox& other) const
  {
    if (!valid || !other.valid || !Intersects(other))
      return GeoBox();

    GeoCoord cornerMin( std::max( other.GetMinLat(), GetMinLat()),
                        std::max( other.GetMinLon(), GetMinLon()));

    GeoCoord cornerMax( std::min( other.GetMaxLat(), GetMaxLat()),
                        std::min( other.GetMaxLon(), GetMaxLon()));

    return GeoBox(cornerMin, cornerMax);
  }

  GeoCoord GeoBox::GetCenter() const
  {
    return GeoCoord((minCoord.GetLat()+maxCoord.GetLat())/2,
                    (minCoord.GetLon()+maxCoord.GetLon())/2);
  }

  /**
   * Return a string representation of the coordinate value in a human readable format.
   */
  std::string GeoBox::GetDisplayText() const
  {
    return "[" + minCoord.GetDisplayText() + " - " + maxCoord.GetDisplayText() + "]";
  }

  GeoBox GeoBox::BoxByCenterAndRadius(const GeoCoord& center,
                                      double radius)
  {
    double topLat;
    double botLat;
    double leftLon;
    double rightLon;

    GetEllipsoidalDistance(center.GetLat(),
                           center.GetLon(),
                           315.0,
                           radius,
                           topLat,
                           leftLon);

    GetEllipsoidalDistance(center.GetLat(),
                           center.GetLon(),
                           135.0,
                           radius,
                           botLat,
                           rightLon);

    GeoBox boundingBox(GeoCoord(topLat,leftLon),
                       GeoCoord(botLat,rightLon));

    return boundingBox;
  }

}
