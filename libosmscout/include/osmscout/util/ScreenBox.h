#ifndef OSMSCOUT_UTIL_SCREENBOX_H
#define OSMSCOUT_UTIL_SCREENBOX_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2022  Tim Teulings

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

#include <osmscout/CoreImportExport.h>

#include <osmscout/Pixel.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   *
   * Anonymous screen rectangular bounding box.
   *
   * The bounding box is defined by two coordinates (type Vertex2D) that span a
   * (in coordinate space) rectangular area.
   */
  class OSMSCOUT_API ScreenBox CLASS_FINAL
  {
  private:
    Vertex2D minCoord=Vertex2D(0.0,0.0);
    Vertex2D maxCoord=Vertex2D(0.0,0.0);

  public:
    /**
     * The default constructor creates an invalid instance.
     */
    ScreenBox() = default;

    /**
     * Copy-Constructor
     */
    ScreenBox(const ScreenBox& other) = default;

    /**
     * Assign the value of other
     */
    ScreenBox& operator=(const ScreenBox& other) = default;

    /**
     * Initialize the GeoBox based on the given coordinates. The two Coordinates
     * together span a rectangular (in coordinates, not on the sphere) area.
     */
    ScreenBox(const Vertex2D& coordA,
              const Vertex2D& coordB);

    double GetMinX() const
    {
      return minCoord.GetX();
    }

    double GetMinY() const
    {
      return minCoord.GetY();
    }

    double GetMaxX() const
    {
      return maxCoord.GetX();
    }

    double GetMaxY() const
    {
      return maxCoord.GetY();
    }

    /**
     * Returns the width of the bounding box (maxLon-minLon).
     */
    double GetWidth() const
    {
      return maxCoord.GetX()-minCoord.GetX();
    }

    /**
     * Returns the height of the bounding box (maxLat-minLat).
     */
    double GetHeight() const
    {
      return maxCoord.GetY()-minCoord.GetY();
    }

    /**
     * Returns the center coordinates of the box
     * @return the center coordinates
     */
    Vertex2D GetCenter() const
    {
      return {(minCoord.GetX()+maxCoord.GetX())/2,
              (minCoord.GetY()+maxCoord.GetY())/2};
    }

    ScreenBox Merge(const ScreenBox& other) const;
  };
}

#endif
