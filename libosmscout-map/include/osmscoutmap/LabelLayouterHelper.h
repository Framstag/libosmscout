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

#include <array>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <osmscoutmap/MapImportExport.h>

#include <osmscoutmap/MapParameter.h>
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
     * Rebuild the mask for the given rectangle, reusing the bitmask of the previous use. A mask
     * is only valid for one screen width and one rectangle, so it has to be reset before each
     * use.
     */
    void Reset(size_t screenWidth,
               const ScreenPixelRectangle &rect);

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
    size_t                rowLength=0;
    size_t                height=0;

  public:
    ScreenMask() = default;
    ScreenMask(size_t width, size_t height);

    /**
     * Reset the mask for a frame: clear all marks and adopt the given viewport size. The bitmask
     * is reused, so a repeated frame of the same size does not allocate it again.
     */
    void Reset(size_t width, size_t height);

    void AddMask(const ScreenRectMask& mask);
    bool HasCollision(const ScreenRectMask& mask) const;
  };

  /**
   * Number of words of a text, i.e. the number of sequences of characters separated by
   * whitespace. Used together with GetLabelExtentBound to bound the height of a wrapped
   * label, because text is wrapped at word boundaries.
   */
  OSMSCOUT_MAP_API size_t CountLabelWords(const std::string_view& text);

  /**
   * The widest padding [pixels] the label layout adds around a label element when it marks the
   * element in the overlap canvases of a frame. An element that lies outside the view can still
   * suppress a label inside the view through this padding, so a decision that removes a label
   * from the frame has to leave room for the widest padding of the frame.
   */
  OSMSCOUT_MAP_API double GetMaxLabelPaddingPixel(const Projection& projection,
                                                  const MapParameter& parameter);

  /**
   * The margin [pixels] outside the visible view in which a label element can still take part in
   * a frame: the label layout marks elements in a view that is enlarged by the layouter overlap,
   * and it marks them with a padding around their rectangle. An element outside the visible view
   * can therefore still suppress a label inside it, and a decision taken against the visible view
   * has to leave room for this margin.
   */
  OSMSCOUT_MAP_API double GetLabelLayoutMarginPixel(const Projection& projection,
                                                    const MapParameter& parameter);

  /**
   * Conservative upper bound [pixels] of half of the larger side of the rectangle a label
   * can occupy after layout, derived from the number of characters and words of its text and
   * from the pixel size of its font.
   *
   * The bound is conservative: a glyph advance is assumed to be at most
   * maxLabelAdvanceFactor times the font size (an em box is 1.0 times the font size), and
   * because text is wrapped at word boundaries a label carries at most one line more than it
   * has words. Callers may therefore use this value to reject a label that provably cannot
   * reach the viewport, but must not use it to accept a label.
   *
   * @param characterCount number of characters of the label text
   * @param wordCount number of words of the label text (see CountLabelWords)
   * @param fontSizePixel size of the font in pixels
   * @return the half extent [pixels], 0.0 for an empty text or a non-positive font size
   */
  OSMSCOUT_MAP_API double GetLabelExtentBound(size_t characterCount,
                                              size_t wordCount,
                                              double fontSizePixel);

  /**
   * Build the measurement environment of a map backend from the state every backend shares:
   * the font, the factor its size is scaled with, the resolution and the magnification of the
   * projection. A backend appends the state of its drawing target, for example the scale of a
   * cairo surface or the DPI of a Qt device.
   *
   * The label layouter drops its remembered measurements when the environment changes.
   */
  OSMSCOUT_MAP_API std::string BuildMeasurementEnvironment(const std::string& fontName,
                                                           double fontSize,
                                                           double dpi,
                                                           size_t magnification);
}

#endif
