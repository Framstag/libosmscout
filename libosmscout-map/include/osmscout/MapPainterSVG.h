#ifndef OSMSCOUT_MAP_MAPPAINTERSVG_H
#define OSMSCOUT_MAP_MAPPAINTERSVG_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2011  Tim Teulings

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

#include <ostream>
#include <set>

#include <osmscout/Private/MapImportExport.h>

#include <osmscout/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapPainterSVG : public MapPainter
  {
  private:
     std::ostream stream;

  private:
    std::string GetColorValue(double r, double g, double b);
    std::string GetColorValue(double r, double g, double b, double a);

    void WriteHeader(size_t width,size_t height);
    void WriteFooter();

    void StartMainGroup();
    void FinishMainGroup();

  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style);

    bool HasPattern(const StyleConfig& styleConfig,
                   const MapParameter& parameter,
                   PatternStyle& style);

    void GetTextDimension(const MapParameter& parameter,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const Label& label);

    void DrawPlateLabel(const Projection& projection,
                        const MapParameter& parameter,
                        const Label& label);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const LabelStyle& style,
                          const std::string& text,
                          const TransPolygon& contour);

    void DrawSymbol(const SymbolStyle* style,
                    double x, double y);

    void DrawIcon(const IconStyle* style,
                  double x, double y);

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  double r, double g, double b, double a,
                  double width,
                  const std::vector<double>& dash,
                  CapStyle startCap,
                  CapStyle endCap,
                  const TransPolygon& path);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  TypeId type,
                  const FillStyle& fillStyle,
                  const TransPolygon& area);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  TypeId type,
                  const PatternStyle& patternStyle,
                  const TransPolygon& area);

    void DrawArea(const FillStyle& style,
                  const MapParameter& parameter,
                  double x,
                  double y,
                  double width,
                  double height);

  public:
    MapPainterSVG();
    virtual ~MapPainterSVG();


    bool DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 std::ostream& stream);
  };
}

#endif
