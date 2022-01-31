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

#include <osmscoutmap/MapPainterNoOp.h>

namespace osmscout {

  static double FONT_HEIGHT_FACTOR=10; //!< Height of the font in pixel in relation to the given fontSize
  //static double FONT_WIDTH_HEIGHT_FACTOR=1; //!< Width of an individual character in relation to its height

  MapPainterNoOp::MapPainterNoOp(const StyleConfigRef& styleConfig)
          : MapPainter(styleConfig)
  {
    // no code
  }

  bool MapPainterNoOp::HasIcon(const StyleConfig& /*styleConfig*/,
                               const Projection& /*projection*/,
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

  void MapPainterNoOp::DrawGround(const Projection& /*projection*/,
                                  const MapParameter& /*parameter*/,
                                  const FillStyle& /*style*/)
  {
    // no code
  }


  void MapPainterNoOp::RegisterRegularLabel(const Projection& /*projection*/,
                                            const MapParameter& /*parameter*/,
                                            const std::vector<LabelData> &/*labels*/,
                                            const Vertex2D& /*position*/,
                                            const double /*objectWidth*/)
  {
    // no code
  }


  void MapPainterNoOp::RegisterContourLabel(const Projection & /*projection*/,
                                            const MapParameter & /*parameter*/,
                                            const PathLabelData & /*label*/,
                                            const LabelPath & /*labelPath*/)
  {
    // no code
  }

  void MapPainterNoOp::DrawLabels(const osmscout::Projection&,
                                  const osmscout::MapParameter&,
                                  const osmscout::MapData&)
  {
    // no code
  }

  void MapPainterNoOp::DrawIcon(const IconStyle* /*style*/,
                                double /*centerX*/, double /*centerY*/,
                                double /*width*/, double /*height*/)
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
                               const MapData& data,
                               RenderSteps startStep,
                               RenderSteps endStep)
  {
    Draw(projection,
         parameter,
         data,
         startStep,
         endStep);

    return true;
  }
}
