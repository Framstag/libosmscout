/*
  This source is part of the libosmscout-map library
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

#include <osmscout/MapPainterNoOp.h>

namespace osmscout {

  static double FONT_HEIGHT_FACTOR=10; //!< Height of the font in pixel in relation to the given fontSize
  static double FONT_WIDTH_HEIGHT_FACTOR=1; //!< Width of an individual character in relation to its height

  MapPainterNoOp::MapPainterNoOp(const StyleConfigRef& styleConfig)
          : MapPainter(styleConfig,
                       new CoordBuffer())
  {
    // no code
  }

  bool MapPainterNoOp::HasIcon(const StyleConfig& /*styleConfig*/,
                               const MapParameter& /*parameter*/,
                               IconStyle& /*style*/)
  {
    return false;
  }

  double MapPainterNoOp::GetFontHeight(const Projection& /*projection*/,
                                     const MapParameter& /*parameter*/,
                                     double fontSize)
  {
    return FONT_HEIGHT_FACTOR*fontSize;
  }

  MapPainter::TextDimension MapPainterNoOp::GetTextDimension(const Projection& /*projection*/,
                                                             const MapParameter& /*parameter*/,
                                                             double /*objectWidth*/,
                                                             double fontSize,
                                                             const std::string& text)
  {
    double height=FONT_HEIGHT_FACTOR*FONT_WIDTH_HEIGHT_FACTOR*fontSize;

    return TextDimension(0.0,
                         0.0,
                         text.length()*height,
                         height);
  }

  void MapPainterNoOp::DrawGround(const Projection& /*projection*/,
                                  const MapParameter& /*parameter*/,
                                  const FillStyle& /*style*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawLabel(const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const LabelData& /*label*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawIcon(const IconStyle* /*style*/,
                                double /*x*/,
                                double /*y*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawSymbol(const Projection& /*projection*/,
                                  const MapParameter& /*parameter*/,
                                  const Symbol& /*symbol*/,
                                  double /*x*/,
                                  double /*y*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawPath(const Projection& /*projection*/,
                                const MapParameter& /*parameter*/,
                                const Color& /*color*/,
                                double /*width*/,
                                const std::vector<double>& /*dash*/,
                                LineStyle::CapStyle /*startCap*/,
                                LineStyle::CapStyle /*endCap*/,
                                size_t /*transStart*/,
                                size_t /*transEnd*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawContourLabel(const Projection& /*projection*/,
                                        const MapParameter& /*parameter*/,
                                        const PathTextStyle& /*style*/,
                                        const std::string& /*text*/,
                                        size_t /*transStart*/,
                                        size_t /*transEnd*/,
                                        ContourLabelHelper& /*helper*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawContourSymbol(const Projection& /*projection*/,
                                         const MapParameter& /*parameter*/,
                                         const Symbol& /*symbol*/,
                                         double /*space*/,
                                         size_t /*transStart*/,
                                         size_t /*transEnd*/)
  {
    // no code
  }


  void MapPainterNoOp::DrawArea(const Projection& /*projection*/,
                                const MapParameter& /*parameter*/,
                                const AreaData& /*area*/)
  {
    // no code
  }

  bool MapPainterNoOp::DrawMap(const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    Draw(projection,
         parameter,
         data);

    return true;
  }
}
