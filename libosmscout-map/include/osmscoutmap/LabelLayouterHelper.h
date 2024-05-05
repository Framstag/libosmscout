#ifndef OSMSCOUT_MAP_LABELLAYOUTERHELPER_H
#define OSMSCOUT_MAP_LABELLAYOUTERHELPER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2018 Lukas Karas
  Copyright (C) 2024 Tim Teulngs

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
#include <set>
#include <array>

#include <osmscoutmap/MapImportExport.h>

#include <osmscoutmap/StyleConfig.h>
#include <osmscoutmap/LabelPath.h>
#include <osmscout/system/Math.h>

#include <iostream>

namespace osmscout {

  struct ScreenPixelRectangle {
    int x;
    int y;
    int width;
    int height;

    // not initialised viewport
    ScreenPixelRectangle() = default;

    ScreenPixelRectangle(int x, int y, int width, int height)
    : x(x),
      y(y),
      width(width),
      height(height)
    {
    }

    /**
     * Returns true , if the area of the rectangles intersect. The area is defined by
     * area including [x-x+width-1] and [y,y+height-1]. x+width or y+height is outside the rectangle.
     *
     * @param other rectangle
     * @return true, if areas intersect, else false
     */
    bool Intersects(const ScreenPixelRectangle &other) const
    {
      return !(
              (x + width-1) < other.x ||
              x > (other.x + other.width-1) ||
              (y + height-1) < other.y ||
              y > (other.y + other.height-1)
      );
    }
  };

  struct ScreenVectorRectangle {
    double x;
    double y;
    double width;
    double height;

    // not initialised viewport
    ScreenVectorRectangle() = default;

    ScreenVectorRectangle(double x, double y, double width, double height)
    : x(x),
      y(y),
      width(width),
      height(height)
    {
    }

    ScreenVectorRectangle& Set(double nx, double ny, double nw, double nh)
    {
      x = nx;
      y = ny;
      width = nw;
      height = nh;

      return *this;
    }

    bool Intersects(const ScreenVectorRectangle &other) const
    {
      return !(
          (x + width) < other.x ||
          x > (other.x + other.width) ||
          (y + height) < other.y ||
          y > (other.y + other.height)
      );
    }
  };

  /**
   * Holds a rectangular bit mask
   *
   * Implementation:
   * Only one row of the mask is stored together with the indexes of the starting and final row
   */
  class OSMSCOUT_MAP_API ScreenRectMask CLASS_FINAL
  {
  private:
    int                   cellFrom{0}; // First used byte of mask
    int                   cellTo{0};   // Last used byte of mask
    int                   rowFrom{0};  // First row of mask
    int                   rowTo{0};    // Last row of mask
    std::vector<uint64_t> bitmask;     // bitmask for one row

  public:
    ScreenRectMask() = default;
    ScreenRectMask(size_t screenWidth,
                   const ScreenPixelRectangle &rect);

    bool Intersects(const ScreenRectMask& other) const;

    /**
     * Return starting index of row (y-coordinate of rectangle)
     * @return index
     */
    int GetFirstRow() const {
      return rowFrom;
    }

    /**
     * Return final index of row (y+height-1 of rectangle)
     * @return index
     */
    int GetLastRow() const {
      return rowTo;
    }

    /**
     * Return the index of the initial, left-sided bit mask cell (containing x coordinate of rectangle)
     * @return index
     */
    int GetFirstCell() const {
      return cellFrom;
    }

    /**
     * Return the index of the final, right-sided bit mask cell (containing x+width-1 coordinate of rectangle)
     * @return index
     */
    int GetLastCell() const {
      return cellTo;
    }

    /**
     * Return the cells in the interval [GetFirstCell(),GetLastCell()]. A cell contains a part
     * of the bitmask of a row of the rectangle.
     *
     * The lowest bit 0x1 is the first bit in the mask, higher bits represent further bits to the "right" in the mask
     *
     * @param idx the index
     * @return te bit mask
     */
    uint64_t GetCell(size_t idx) const;
  };

  class OSMSCOUT_MAP_API ScreenMask CLASS_FINAL
  {
  private:
    std::vector<uint64_t> bitmask;
    size_t                rowLength;
    size_t                height;

  public:
    ScreenMask(size_t width, size_t height);

    void AddMask(const ScreenRectMask& mask);
    bool HasCollision(const ScreenRectMask& mask) const;
  };
}

#endif
