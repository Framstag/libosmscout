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
#include <iostream>
#include <iomanip>
#include <limits>
#include <list>
#include <cstdlib>

#include <osmscout/Util.h>

#include <osmscout/private/Math.h>

namespace osmscout {

  static const char* valueChar="0123456789abcdef";

  MapPainterSVG::MapPainterSVG()
  : stream(NULL),
    typeConfig(NULL)
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
    result.append(1,valueChar[(unsigned int)(r*255)/16]);
    result.append(1,valueChar[(unsigned int)(r*255)%16]);
    result.append(1,valueChar[(unsigned int)(g*255)/16]);
    result.append(1,valueChar[(unsigned int)(g*255)%16]);
    result.append(1,valueChar[(unsigned int)(b*255)/16]);
    result.append(1,valueChar[(unsigned int)(b*255)%16]);

    return result;
  }

  std::string MapPainterSVG::GetColorValue(double r, double g, double b, double a)
  {
    std::string result;

    result.reserve(9);

    result.append("#");

    if (a!=1.0) {
      result.append(1,valueChar[(unsigned int)(a*255)/16]);
      result.append(1,valueChar[(unsigned int)(a*255)%16]);
    }

    result.append(1,valueChar[(unsigned int)(r*255)/16]);
    result.append(1,valueChar[(unsigned int)(r*255)%16]);
    result.append(1,valueChar[(unsigned int)(g*255)/16]);
    result.append(1,valueChar[(unsigned int)(g*255)%16]);
    result.append(1,valueChar[(unsigned int)(b*255)/16]);
    result.append(1,valueChar[(unsigned int)(b*255)%16]);

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

  void MapPainterSVG::DumpStyles(const StyleConfig& styleConfig,
                                 const Projection& projection)
  {
    stream << "  <defs>" << std::endl;
    stream << "    <style type=\"text/css\">" << std::endl;
    stream << "       <![CDATA[" << std::endl;


    for (std::vector<TypeInfo>::const_iterator typeInfo=styleConfig.GetTypeConfig()->GetTypes().begin();
        typeInfo!=styleConfig.GetTypeConfig()->GetTypes().end();
        typeInfo++) {
      const FillStyle *fillStyle=styleConfig.GetAreaFillStyle(typeInfo->GetId());

      if (fillStyle!=NULL) {
        stream << "        ." << typeInfo->GetName() << "_area {";

        stream << "fill:" << GetColorValue(fillStyle->GetFillR(),fillStyle->GetFillG(),fillStyle->GetFillB(),fillStyle->GetFillA());

        double borderWidth=GetProjectedWidth(projection, fillStyle->GetBorderMinPixel(), fillStyle->GetBorderWidth());

        if (borderWidth>0.0) {
          stream << ";stroke:" << GetColorValue(fillStyle->GetBorderR(),fillStyle->GetBorderG(),fillStyle->GetBorderB());
          stream << ";stroke-width:" << borderWidth;
        }

        stream << "}" << std::endl;
      }
    }

    stream << std::endl;

    for (std::vector<TypeInfo>::const_iterator typeInfo=styleConfig.GetTypeConfig()->GetTypes().begin();
        typeInfo!=styleConfig.GetTypeConfig()->GetTypes().end();
        typeInfo++) {
      const LineStyle *lineStyle=styleConfig.GetWayLineStyle(typeInfo->GetId());

      if (lineStyle!=NULL) {
        /*
        stream << "        ." << typeInfo->GetName() << "_way {";
        // TODO
        stream << "}" << std::endl;*/
      }
    }

    stream << "       ]]>" << std::endl;
    stream << "    </style>" << std::endl;
    stream << "  </defs>" << std::endl;
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
                                       size_t transStart, size_t transEnd)
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
                                 size_t transStart, size_t transEnd)
  {
    stream << "    <polyline fill=\"none\" stroke=\"" << GetColorValue(r,g,b,a) << "\" stroke-width=\"" << width << "\"" << std::endl;
    stream << "              points=\"";

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i!=transStart) {
        stream << " ";
      }

      stream << transBuffer.buffer[i].x << "," << transBuffer.buffer[i].y;

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawArea(const Projection& projection,
                               const MapParameter& parameter,
                               const MapPainter::AreaData& area)
  {
    stream << "    <polygon class=\"" << typeConfig->GetTypeInfo(area.attributes->GetType()).GetName() << "_area\"" << std::endl;
    stream << "             points=\"";

    for (size_t i=area.transStart; i<=area.transEnd; i++) {
      if (i!=area.transStart) {
        stream << " ";
      }

      stream << transBuffer.buffer[i].x << "," << transBuffer.buffer[i].y;

    }

    stream << "\" />" << std::endl;
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
    typeConfig=styleConfig.GetTypeConfig();

    WriteHeader(projection.GetWidth(),projection.GetHeight());

    DumpStyles(styleConfig,
               projection);

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

