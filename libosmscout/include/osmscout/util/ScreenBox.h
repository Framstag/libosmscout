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

#include <osmscout/lib/CoreImportExport.h>

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
    static const ScreenBox EMPTY;
    /**
     * The default constructor creates an invalid instance.
     */
    ScreenBox() = default;

    /**
     * Copy-Constructor
     */
    ScreenBox(const ScreenBox& other) = default;

    /**
     * Move-Constructor
     */
    ScreenBox(ScreenBox&& other) = default;

    /**
     * Initialize the GeoBox based on the given coordinates. The two Coordinates
     * together span a rectangular (in coordinates, not on the sphere) area.
     */
    ScreenBox(const Vertex2D& coordA,
              const Vertex2D& coordB);

    /**
     * Assign the value of other
     */
    ScreenBox& operator=(const ScreenBox& other) = default;

    /**
     * Move assignment operator
     */
    ScreenBox& operator=(ScreenBox&& other) = default;

    /**
     * Compare two values
     */
    bool operator==(const ScreenBox& other) const;

    [[nodiscard]] double GetMinX() const
    {
      return minCoord.GetX();
    }

    [[nodiscard]] double GetMinY() const
    {
      return minCoord.GetY();
    }

    [[nodiscard]] double GetMaxX() const
    {
      return maxCoord.GetX();
    }

    [[nodiscard]] double GetMaxY() const
    {
      return maxCoord.GetY();
    }

    /**
     * Returns the width of the bounding box (maxLon-minLon).
     */
    [[nodiscard]] double GetWidth() const
    {
      return maxCoord.GetX()-minCoord.GetX();
    }

    /**
     * Returns the height of the bounding box (maxLat-minLat).
     */
    [[nodiscard]] double GetHeight() const
    {
      return maxCoord.GetY()-minCoord.GetY();
    }

    /**
     * Returns the size of the screen box (width*height).
     *
     * @return GetWidth()*GetHeight()
     */
    [[nodiscard]] double GetSize() const
    {
      return GetWidth()*GetHeight();
    }

    /**
     * Check if size of the screen box is zero
     *
     * @return GetSize()==0.0
     */
    [[nodiscard]] bool IsEmpty() const
    {
      return GetSize()==0.0;
    }

    /**
     * Returns the center coordinates of the box
     * @return the center coordinates
     */
    [[nodiscard]] Vertex2D GetCenter() const
    {
      return {(minCoord.GetX()+maxCoord.GetX())/2,
              (minCoord.GetY()+maxCoord.GetY())/2};
    }

    [[nodiscard]] bool Intersects(const ScreenBox& other) const;
    [[nodiscard]] bool Intersects(const ScreenBox& other,
                                  bool openInterval) const;

    /**
     * Resize the rectangle in all dimension using the given amount.
     * If offset is >=0 the resulting area will be bigger, else smaller.
     *
     * If the reduction (negative offset) is bigger than width/2 or height/2,
     * resulted screen box will have zero size (will be empty).
     *
     * The size delta will be 2*offset in width and in height!
     *
     * @param offset the amount to change the coordinates.
     * @return the resulting ScreenBox
     */
    [[nodiscard]] ScreenBox Resize(double offset) const;
    [[nodiscard]] ScreenBox Merge(const ScreenBox& other) const;
  };
}

#endif
