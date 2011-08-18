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

#include <osmscout/MapPainterSVG.h>

#include <cassert>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <limits>
#include <list>
#include <cstdlib>

#include <osmscout/Util.h>

namespace osmscout {

  static const char* valueChar="0123456789abcdef";

  MapPainterSVG::MapPainterSVG()
  : stream(NULL)
  {
    // no code
  }

  MapPainterSVG::~MapPainterSVG()
  {
  }

  std::string MapPainterSVG::GetColorValue(double r, double g, double b)
  {
    std::string result;

    result.reserve(7);

    result.append("#");
    result.append(1,valueChar[(uint)(r*255)/16]);
    result.append(1,valueChar[(uint)(r*255)%16]);
    result.append(1,valueChar[(uint)(g*255)/16]);
    result.append(1,valueChar[(uint)(g*255)%16]);
    result.append(1,valueChar[(uint)(b*255)/16]);
    result.append(1,valueChar[(uint)(b*255)%16]);

    return result;
  }

  std::string MapPainterSVG::GetColorValue(double r, double g, double b, double a)
  {
    std::string result;

    result.reserve(9);

    result.append("#");
    result.append(1,valueChar[(uint)(a*255)/16]);
    result.append(1,valueChar[(uint)(a*255)%16]);
    result.append(1,valueChar[(uint)(r*255)/16]);
    result.append(1,valueChar[(uint)(r*255)%16]);
    result.append(1,valueChar[(uint)(g*255)/16]);
    result.append(1,valueChar[(uint)(g*255)%16]);
    result.append(1,valueChar[(uint)(b*255)/16]);
    result.append(1,valueChar[(uint)(b*255)%16]);

    return result;
  }

  void MapPainterSVG::WriteHeader(size_t width, size_t height)
  {
    stream << "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>" << std::endl;
    stream << "<!-- Created by the MapPainterSVG backend, part of libosmscout (http://libosmscout.sf.net) -->" << std::endl;
    stream << std::endl;

    stream << "<svg" << std::endl;
    stream << "  xmlns:svg=\"http://www.w3.org/2000/svg\"" << std::endl;
    stream << "  xmlns=\"http://www.w3.org/2000/svg\"" << std::endl;
    stream << "  width=\"" << width << "\"" << std::endl;
    stream << "  height=\"" << height << "\"" << std::endl;
    stream << "  id=\"map\"" << std::endl;
    stream << "  version=\"1.1\">" << std::endl;
    stream << std::endl;
  }

  void MapPainterSVG::WriteFooter()
  {
    stream << "</svg>" << std::endl;
  }

  void MapPainterSVG::StartMainGroup()
  {
    stream << "  <g id=\"map\">" << std::endl;
  }

  void MapPainterSVG::FinishMainGroup()
  {
    stream << "  </g>" << std::endl;
  }

  bool MapPainterSVG::HasIcon(const StyleConfig& styleConfig,
                                const MapParameter& parameter,
                                IconStyle& style)
  {
    return false;
  }

  bool MapPainterSVG::HasPattern(const StyleConfig& styleConfig,
                                   const MapParameter& parameter,
                                   PatternStyle& style)
  {
    return false;
  }

  void MapPainterSVG::GetTextDimension(const MapParameter& parameter,
                                         double fontSize,
                                         const std::string& text,
                                         double& xOff,
                                         double& yOff,
                                         double& width,
                                         double& height)
  {
  }

  void MapPainterSVG::DrawLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const Label& label)
  {
  }

  void MapPainterSVG::DrawPlateLabel(const Projection& projection,
                                       const MapParameter& parameter,
                                       const Label& label)
  {
  }

  void MapPainterSVG::DrawContourLabel(const Projection& projection,
                                         const MapParameter& parameter,
                                         const LabelStyle& style,
                                         const std::string& text,
                                         const TransPolygon& contour)
  {
  }

  void MapPainterSVG::DrawSymbol(const SymbolStyle* style,
                                   double x, double y)
  {
  }

  void MapPainterSVG::DrawIcon(const IconStyle* style,
                                 double x, double y)
  {
    assert(style->GetId()>0);
    assert(style->GetId()!=std::numeric_limits<size_t>::max());
  }

  void MapPainterSVG::DrawPath(const Projection& projection,
                                 const MapParameter& parameter,
                                 double r,
                                 double g,
                                 double b,
                                 double a,
                                 double width,
                                 const std::vector<double>& dash,
                                 CapStyle startCap,
                                 CapStyle endCap,
                                 const TransPolygon& path)
  {
    stream << "    <polyline fill=\"none\" stroke=\"" << GetColorValue(r,g,b,a) << "\" stroke-width=\"" << width << "\"" << std::endl;
    stream << "              points=\"";

    for (size_t i=path.GetStart(); i<=path.GetEnd(); i++) {
      if (i!=path.GetStart()) {
        stream << " ";
      }

      stream << path.points[i].x << "," << path.points[i].y;

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 TypeId type,
                                 const FillStyle& fillStyle,
                                 const LineStyle* lineStyle,
                                 const TransPolygon& area)
  {
    stream << "    <polygon fill=\"" << GetColorValue(fillStyle.GetFillR(),fillStyle.GetFillG(),fillStyle.GetFillB()) << "\"" << std::endl;

    if (lineStyle!=NULL) {
      stream << "             stroke=\"" << GetColorValue(lineStyle->GetLineR(),lineStyle->GetLineG(),lineStyle->GetLineB()) << "\"" << std::endl;
    }

    stream << "             points=\"";

    for (size_t i=area.GetStart(); i<=area.GetEnd(); i++) {
      if (i!=area.GetStart()) {
        stream << " ";
      }

      stream << area.points[i].x << "," << area.points[i].y;

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawArea(const Projection& projection,
                                 const MapParameter& parameter,
                                 TypeId type,
                                 const PatternStyle& patternStyle,
                                 const LineStyle* lineStyle,
                                 const TransPolygon& area)
  {
  }

  void MapPainterSVG::DrawArea(const FillStyle& style,
                                 const MapParameter& parameter,
                                 double x,
                                 double y,
                                 double width,
                                 double height)
  {
    stream << "    <rect x=\"" << x << "\" y=\"" << y << "\" width=\"" << width << "\" height=\"" << height << "\"" << std::endl;
    stream << "          fill=\"" << GetColorValue(style.GetFillR(),style.GetFillG(),style.GetFillB()) << "\"" << "/>" << std::endl;
  }

  bool MapPainterSVG::DrawMap(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data,
                                std::ostream& stream)
  {
    this->stream.rdbuf(stream.rdbuf());

    WriteHeader(projection.GetWidth(),projection.GetHeight());

    StartMainGroup();
    Draw(styleConfig,
         projection,
         parameter,
         data);
    FinishMainGroup();

    WriteFooter();

    return true;
  }
}

