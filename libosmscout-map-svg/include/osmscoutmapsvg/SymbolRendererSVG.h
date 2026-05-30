#ifndef OSMSCOUT_MAP_SVG_SYMBOLRENDERERSVG_H
#define OSMSCOUT_MAP_SVG_SYMBOLRENDERERSVG_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2023  Tim Teulings

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

#include <osmscoutmapsvg/MapSVGImportExport.h>

#include <osmscoutmap/SymbolRenderer.h>

#include <ostream>

namespace osmscout {

/**
 * \ingroup Renderer
 *
 * SVG backend implementation of SymbolRenderer.
 * Writes SVG elements to the given output stream.
 */
class OSMSCOUT_MAP_SVG_API SymbolRendererSVG: public SymbolRenderer
{
private:
  std::ostream &stream;

  bool        hasFill;
  std::string fillColor;

  bool        hasStroke;
  std::string strokeColor;
  double      strokeWidth;
  bool        strokeAlpha;

  void WriteFillAndStroke();

public:
  explicit SymbolRendererSVG(std::ostream &stream);

  SymbolRendererSVG(const SymbolRendererSVG&) = delete;
  SymbolRendererSVG(SymbolRendererSVG&&) = delete;
  SymbolRendererSVG& operator=(const SymbolRendererSVG&) = delete;
  SymbolRendererSVG& operator=(SymbolRendererSVG&&) = delete;

  ~SymbolRendererSVG() override = default;

protected:
  void SetFill(const FillStyleRef &fillStyle) override;
  void SetBorder(const BorderStyleRef &borderStyle,
                 double screenMmInPixel) override;

  void DrawPolygon(const std::vector<Vertex2D> &polygonPixels) override;
  void DrawRect(double x,
                double y,
                double w,
                double h) override;
  void DrawCircle(double x,
                  double y,
                  double radius) override;
};

}

#endif // OSMSCOUT_MAP_SVG_SYMBOLRENDERERSVG_H
