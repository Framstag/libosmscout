#ifndef OSMSCOUT_UTIL_PROJECTION_H
#define OSMSCOUT_UTIL_PROJECTION_H

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/GeoCoord.h>
#include <osmscout/Pixel.h>
#include <osmscout/Point.h>

#include <osmscout/util/GeoBox.h>
#include <osmscout/util/Magnification.h>
#include <osmscout/util/ScreenBox.h>

#include <osmscout/system/SSEMathPublic.h>

namespace osmscout {

  /**
   * \ingroup Geometry
   *
   * The Projection class is an abstract base class for multiple projection implementations.
   *
   * The Projection class allows transformation of geo coordinates to screen/image coordinates and
   * screen/image coordinates back to geo coordinates.
   */
  class OSMSCOUT_API Projection
  {
  protected:
    GeoCoord      center;           //!< Coordinate of the center of the displayed area
    double        angle=0.0;        //!< Display rotation angle in radians, canvas clockwise
    Magnification magnification;    //!< Current magnification
    double        dpi=0.0;          //!< Screen DPI
    size_t        width=0;          //!< Width of image
    size_t        height=0;         //!< Height of image

    GeoBox        boundingBox;      //!< Bounding box of the current projection on the ground

    double        pixelSize=0.0;    //!< Size of a pixel in meter
    double        meterInPixel=0.0; //!< Number of on screen pixel for one meter on the ground
    double        meterInMM=0.0;    //!< Number of on screen millimeters for one meter on the ground

  public:

    /**
     * This class is used to hide internal complexity concerned with batching GeoToPixel calls
     */
    class BatchTransformer CLASS_FINAL
    {
    public:
      // this should be private, but that would exclude future projection
      // implementors. I don't know a nice way to handle this
      const Projection&  projection;
#ifdef OSMSCOUT_HAVE_SSE2
      int                count=0;
      ALIGN16_BEG double lon[2] ALIGN16_END;
      ALIGN16_BEG double lat[2] ALIGN16_END;
      double*            xPointer[2]={nullptr,nullptr};
      double*            yPointer[2]={nullptr,nullptr};
#endif

    public:
      explicit BatchTransformer(const Projection& projection)
        : projection(projection)
      {
      }

      BatchTransformer(const BatchTransformer& other) = delete;
      BatchTransformer(BatchTransformer&& other) = delete;

      ~BatchTransformer()
      {
        Flush();
      }

      BatchTransformer& operator=(const BatchTransformer& other) = delete;
      BatchTransformer& operator=(BatchTransformer&& other) = delete;

      void GeoToPixel(const GeoCoord& coord,
                      double& x,
                      double& y)
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (projection.CanBatch()) {
          this->lon[count]=coord.GetLon();
          this->lat[count]=coord.GetLat();
          xPointer[count]=&x;
          yPointer[count]=&y;
          count++;

          if (count==2) {
            count=0;
            projection.GeoToPixel(*this);
          }
        }
        else {
          Vertex2D pixel;
          projection.GeoToPixel(coord,
                                pixel);
          x=pixel.GetX();
          y=pixel.GetY();
        }
#else
        Vertex2D pixel;
        projection.GeoToPixel(coord,
                              pixel);
        x=pixel.GetX();
        y=pixel.GetY();
#endif
      }

      void GeoToPixel(const Point& coord,
                      double& x,
                      double& y)
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (projection.CanBatch()) {
          this->lon[count]=coord.GetCoord().GetLon();
          this->lat[count]=coord.GetCoord().GetLat();
          xPointer[count]=&x;
          yPointer[count]=&y;
          count++;

          if (count==2) {
            count=0;
            projection.GeoToPixel(*this);
          }
        }
        else {
          Vertex2D pixel;
          projection.GeoToPixel(coord.GetCoord(),
                                pixel);
          x=pixel.GetX();
          y=pixel.GetY();
        }
#else
        Vertex2D pixel;
        projection.GeoToPixel(coord.GetCoord(),
                              pixel);
        x=pixel.GetX();
        y=pixel.GetY();
#endif
      }

      void Flush()
      {
#ifdef OSMSCOUT_HAVE_SSE2
        if (count!=0) {
          count=0;
          Vertex2D pixel;
          projection.GeoToPixel(GeoCoord(lat[0],
                                         lon[0]),
                                pixel);

          *xPointer[0]=pixel.GetX();
          *yPointer[0]=pixel.GetY();
        }
#endif
      }
    };

    Projection() = default;
    Projection(const Projection&) = default;
    Projection(Projection&&) = default;
    Projection& operator=(const Projection&) = default;
    Projection& operator=(Projection&&) = default;
    virtual ~Projection() = default;

    virtual bool CanBatch() const = 0;
    virtual bool IsValid() const = 0;

    /**
     * Return true if given coordinate is valid for this projection
     */
    virtual bool IsValidFor(const GeoCoord& coord) const = 0;

    [[nodiscard]] GeoCoord GetCenter() const
    {
      return center;
    }

    /**
     * Returns the angle in radians ([0..2*PI[) of the display in relation to the north. A degree of 0 means
     * north is to the top, a degree of PI, renders with the south to the top of the display).
     */
    [[nodiscard]] double GetAngle() const
    {
      return angle;
    }

    /**
     * Returns the width of the screen
     */
    [[nodiscard]] size_t GetWidth() const
    {
      return width;
    }

    /**
     * Returns the height of the screen
     */
    [[nodiscard]] size_t GetHeight() const
    {
      return height;
    }

    /**
     * Return a ScreenBox instance for the screen. The ScreenBox
     * has the value [(0.0,0.0)(width,height)]
     *
     * @return ScreenBox instance
     */
    [[nodiscard]] ScreenBox GetScreenBox() const
    {
      return {Vertex2D(0.0,
                       0.0),
              Vertex2D(static_cast<double>(GetWidth()),
                       static_cast<double>(GetHeight()))};
    }

    /**
     * Return the magnification as part of the projection.
     */
    [[nodiscard]] Magnification GetMagnification() const
    {
      return magnification;
    }

    /**
     * Return the DPI as part of the projection.
     */
    [[nodiscard]] double GetDPI() const
    {
      return dpi;
    }

    [[nodiscard]] GeoBox GetDimensions() const
    {
      return boundingBox;
    }

    /**
     * Returns the size of a pixel in meter
     */
    [[nodiscard]] double GetPixelSize() const
    {
      return pixelSize;
    }

    /**
     * Returns the number of on screen pixel for one meter on the ground
     */
    [[nodiscard]] double GetMeterInPixel() const
    {
      return meterInPixel;
    }

    /**
     * Returns the number of on screen millimeters for one meter on the ground
     */
    [[nodiscard]] double GetMeterInMM() const
    {
      return meterInMM;
    }

    /**
     * Convert a width in mm into the equivalent pixel size based on the given DPI
     *
     * @param width
     *    Width in mm
     * @return
     *    Width in screen pixel
     */
    [[nodiscard]] double ConvertWidthToPixel(double width) const
    {
      return width*dpi/25.4;
    }

    /**
     * Convert a width in pixel into the equivalent mm size based on the given DPI
     *
     * @param width
     *    Width in screen pixel
     * @return
     *    Width in mm
     */
    [[nodiscard]] double ConvertPixelToWidth(double pixel) const
    {
      return pixel*25.4/dpi;
    }

    /**
     * Converts a pixel coordinate to a geo coordinate.
     *
     * Return true on success,
     * false if returned coordinate is not valid
     * for this projection.
     */
    virtual bool PixelToGeo(double x, double y,
                            GeoCoord& coord) const = 0;

    /**
     * Converts a geo coordinate to a pixel coordinate.
     *
     * Return true on success,
     * false if given coordinate is not valid for this projection.
     */
    virtual bool GeoToPixel(const GeoCoord& coord,
                            Vertex2D& pixel) const = 0;

    /**
     * Converts a valid GeoBox to its on screen pixel coordinates
     *
     * Return true on success,
     * false if given coordinate is not valid for this projection.
     */
    bool BoundingBoxToPixel(const GeoBox& boundingBox,
                            ScreenBox& screenBox) const;

  protected:
    virtual void GeoToPixel(const BatchTransformer& transformData) const = 0;

    friend class BatchTransformer;
  };
}

#endif
