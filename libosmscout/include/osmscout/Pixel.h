#ifndef OSMSCOUT_PIXEL_H
#define OSMSCOUT_PIXEL_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/system/OSMScoutTypes.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Number.h>

#include <osmscout/system/Compiler.h>

#include <string>
#include <array>
#include <type_traits>

namespace osmscout {

  /**
   * \ingroup Geometry
   *
   * Representation of a pixel on a display or a plane.
   * Coordinates are non-negative, values are decimal.
   */
  struct OSMSCOUT_API Pixel CLASS_FINAL
  {
    uint32_t x; // NOLINT
    uint32_t y; // NOLINT

    /**
     * The default constructor creates an uninitialized instance (for performance reasons).
     */
    inline Pixel() = default;

    inline Pixel(uint32_t x, uint32_t y)
     :x(x),y(y)
    {
      // no code
    }

    inline bool operator==(const Pixel& other) const
    {
      return x==other.x && y==other.y;
    }

    inline bool operator!=(const Pixel& other) const
    {
      return y!=other.y || x!=other.x;
    }

    inline bool operator<(const Pixel& other) const
    {
      return y<other.y ||
      (y==other.y && x<other.x);
    }

    /**
     * Returns a unique number based on the coordinates of the pixel. The bits of the coordinates
     * are projected onto one number by interleaving the bits of the coordinates. Coordinates close
     * in 2D space are thus likely clos ein one dimensional space, too.
     */
    inline uint64_t GetId() const
    {
      return InterleaveNumbers(x,y);
    }

    std::string GetDisplayText() const;

    inline std::ostream& operator<<(std::ostream& stream) const
    {
      stream << GetDisplayText();
      return stream;
    }
  };

  /**
   * \ingroup Geometry
   * Two dimensional coordinate (floating point values,
   * negative coordinates possible).
   */
  class OSMSCOUT_API Vertex2D CLASS_FINAL
  {
  private:
    std::array<double,2> coords;

  public:
    /**
     * The default constructor creates an uninitialized instance (for performance reasons).
     */
    Vertex2D() = default;

    inline Vertex2D(double x,
                    double y)
    {
      coords[0]=x;
      coords[1]=y;
    }

    Vertex2D(const Vertex2D& other) = default;
    Vertex2D(Vertex2D&& other) = default;

    Vertex2D& operator=(const Vertex2D& other) = default;
    Vertex2D& operator=(Vertex2D&& other) = default;

    inline void SetX(double x)
    {
      coords[0]=x;
    }

    inline void SetY(double y)
    {
      coords[1]=y;
    }

    inline void Set(double x,
                    double y)
    {
      coords[0]=x;
      coords[1]=y;
    }

    inline double GetX() const
    {
      return coords[0];
    }

    inline double GetY() const
    {
      return coords[1];
    }

    inline bool operator==(const Vertex2D& other) const
    {
      return coords[0]==other.coords[0] &&
             coords[1]==other.coords[1];
    }

    inline bool operator<(const Vertex2D& other) const
    {
      return coords[1]<other.coords[1] ||
             (coords[1]==other.coords[1] && coords[0]<other.coords[0]);
    }

    inline double DistanceTo(const Vertex2D &other) const {
      double xDiff = coords[0] - other.coords[0];
      double yDiff = coords[1] - other.coords[1];
      return sqrt(xDiff*xDiff + yDiff*yDiff);
    }
  };

  // make sure that we may use std::memcpy on Vertex2D
  static_assert(std::is_trivially_copyable<Vertex2D>::value);
  static_assert(std::is_trivially_assignable<Vertex2D,Vertex2D>::value);

  /**
   * \ingroup Geometry
   * Three dimensional coordinate (floating point values,
   * negative coordinates possible).
   */
  class OSMSCOUT_API Vertex3D CLASS_FINAL
  {
  private:
    double x;
    double y;
    double z;

  public:
    /**
     * The default constructor creates an uninitialized instance (for performance reasons).
     */
    Vertex3D() = default;

    inline Vertex3D(const Vertex3D& other) = default;

    inline Vertex3D(double x,
                    double y)
     :x(x),
      y(y),
      z(0.0)

    {
      // no code
    }

    inline double GetX() const
    {
      return x;
    }

    inline double GetY() const
    {
      return y;
    }

    inline double GetZ() const
    {
      return y;
    }

    inline void SetX(double x)
    {
      this->x=x;
    }

    inline void SetY(double y)
    {
      this->y=y;
    }

    inline void SetZ(double z)
    {
      this->z=z;
    }

    inline void Set(double x,
                    double y)
    {
      this->x=x;
      this->y=y;
      this->z=0;
    }

    inline void Set(double x,
                    double y,
                    double z)
    {
      this->x=x;
      this->y=y;
      this->z=z;
    }

    inline bool operator==(const Vertex3D& other) const
    {
      return x==other.x &&
             y==other.y &&
             z==other.z;
    }

    inline bool operator<(const Vertex3D& other) const
    {
      if (x!=other.x) {
        return x<other.x;
      }

      if (y!=other.y) {
        return y<other.y;
      }

      return z<other.z;
    }
  };
}

#endif
