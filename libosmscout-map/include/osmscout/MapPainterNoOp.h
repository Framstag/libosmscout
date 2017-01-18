#ifndef OSMSCOUT_MAP_MAPPAINTERNOOP_H
#define OSMSCOUT_MAP_MAPPAINTERNOOP_H
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

#include <osmscout/MapPainter.h>

namespace osmscout {
  /**
   * \ingroup Renderer
   *
   * Simple renderer that does nothing. All required callback methods are implemented
   * in one of the following ways:
   * * Signal that Feature not implemented
   * * Does nothing if this does not influence the rendering algorithms
   * * Return some sensible default, if required
   *
   */
  class OSMSCOUT_MAP_API MapPainterNoOp : public MapPainter
  {
  private:
    CoordBufferImpl<Vertex2D>* coordBuffer;

  protected:
    bool HasIcon(const StyleConfig& styleConfig,const MapParameter& parameter,IconStyle& style);

    void GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize,
                       double& height);

    void GetTextDimension(const Projection& projection,
                          const MapParameter& parameter,
                          double objectWidth,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height);

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label);

    void DrawIcon(const IconStyle* style,
                  double x,
                  double y);

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x,
                    double y);

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart,
                  size_t transEnd);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart,
                          size_t transEnd);

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart,
                           size_t transEnd);

    void DrawArea(const Projection& projection,const MapParameter& parameter,const AreaData& area);

  public:
    MapPainterNoOp(const StyleConfigRef& styleConfig);

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data);
  };

}

#endif
