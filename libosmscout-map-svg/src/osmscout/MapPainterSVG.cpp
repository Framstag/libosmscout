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

#include <iostream>
#include <iomanip>
#include <limits>
#include <list>

#include <osmscout/util/String.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

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
    // No code
  }

  std::string MapPainterSVG::GetColorValue(const Color& color)
  {
    std::string result;

    if (color.IsSolid()) {
      result.reserve(7);
    }
    else {
      result.reserve(9);
    }

    result.append("#");

    if (!color.IsSolid()) {
      result.append(1,valueChar[(unsigned int)(color.GetA()*255)/16]);
      result.append(1,valueChar[(unsigned int)(color.GetA()*255)%16]);
    }

    result.append(1,valueChar[(unsigned int)(color.GetR()*255)/16]);
    result.append(1,valueChar[(unsigned int)(color.GetR()*255)%16]);
    result.append(1,valueChar[(unsigned int)(color.GetG()*255)/16]);
    result.append(1,valueChar[(unsigned int)(color.GetG()*255)%16]);
    result.append(1,valueChar[(unsigned int)(color.GetB()*255)/16]);
    result.append(1,valueChar[(unsigned int)(color.GetB()*255)%16]);

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

  void MapPainterSVG::AfterPreprocessing(const StyleConfig& styleConfig,
                                         const Projection& projection,
                                         const MapParameter& parameter,
                                         const MapData& data)
  {
    stream << "  <defs>" << std::endl;
    stream << "    <style type=\"text/css\">" << std::endl;
    stream << "       <![CDATA[" << std::endl;

    size_t nextAreaId=0;
    for (std::list<AreaData>::const_iterator area=areaData.begin();
        area!=areaData.end();
        ++area) {
      std::map<FillStyle,std::string>::const_iterator entry=fillStyleNameMap.find(*area->fillStyle);

      if (entry==fillStyleNameMap.end()) {
        std::string name="area_"+NumberToString(nextAreaId);

        fillStyleNameMap.insert(std::make_pair(*area->fillStyle,name));

        nextAreaId++;

        stream << "        ." << name << " {";

        stream << "fill:" << GetColorValue(area->fillStyle->GetFillColor());
        stream << ";fillRule:nonzero";

        double borderWidth=ConvertWidthToPixel(parameter,
                                               area->fillStyle->GetBorderWidth());

        if (borderWidth>0.0) {
          stream << ";stroke:" << GetColorValue(area->fillStyle->GetBorderColor());
          stream << ";stroke-width:" << borderWidth;

          if (area->fillStyle->HasBorderDashes()) {
            stream << ";stroke-dasharray:";

            for (size_t i=0; i<area->fillStyle->GetBorderDash().size(); i++) {
              if (i>0) {
                stream << ",";
              }

              stream << area->fillStyle->GetBorderDash()[i]*borderWidth;
            }
          }
        }


        stream << "}" << std::endl;

      }
    }

    stream << std::endl;

    size_t nextWayId=0;
    for (std::list<WayData>::const_iterator way=wayData.begin();
        way!=wayData.end();
        ++way) {
      std::map<LineStyle,std::string>::const_iterator entry=lineStyleNameMap.find(*way->lineStyle);

      if (entry==lineStyleNameMap.end()) {
        std::string name="way_"+NumberToString(nextWayId);

        lineStyleNameMap.insert(std::make_pair(*way->lineStyle,name));

        nextWayId++;

        double lineWidth;

        if (way->lineStyle->GetWidth()==0) {
          lineWidth=ConvertWidthToPixel(parameter,way->lineStyle->GetDisplayWidth());
        }
        else {
          lineWidth=GetProjectedWidth(projection,
                                      ConvertWidthToPixel(parameter,way->lineStyle->GetDisplayWidth()),
                                      way->lineStyle->GetWidth());
        }

        stream << "        ." << name << " {";
        stream << "fill:none;";
        stream << "stroke:" << GetColorValue(way->lineStyle->GetLineColor());

        if (way->lineStyle->HasDashes()) {
          stream << ";stroke-dasharray:";

          for (size_t i=0; i<way->lineStyle->GetDash().size(); i++) {

            if (i>0) {
              stream << " ";
            }

            stream << way->lineStyle->GetDash()[i]*lineWidth;
          }
        }

        stream << "}" << std::endl;
      }
    }

    stream << std::endl;

    stream << "       ]]>" << std::endl;
    stream << "    </style>" << std::endl;
    stream << "  </defs>" << std::endl;
    stream << std::endl;
  }

  void MapPainterSVG::WriteFooter()
  {
    stream << "</svg>" << std::endl;
  }

  void MapPainterSVG::BeforeDrawing(const StyleConfig& styleConfig,
                                    const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& data)
  {
    stream << "  <g id=\"map\">" << std::endl;
  }

  void MapPainterSVG::AfterDrawing(const StyleConfig& styleConfig,
                    const Projection& projection,
                    const MapParameter& parameter,
                    const MapData& data)
  {
    stream << "  </g>" << std::endl;
  }

  bool MapPainterSVG::HasIcon(const StyleConfig& styleConfig,
                                const MapParameter& parameter,
                                IconStyle& style)
  {
    // Not implemented

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
    // Not implemented
  }

  void MapPainterSVG::DrawLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const LabelData& label)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawContourLabel(const Projection& projection,
                                       const MapParameter& parameter,
                                       const PathTextStyle& style,
                                       const std::string& text,
                                       size_t transStart, size_t transEnd)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawContourSymbol(const Projection& projection,
                                        const MapParameter& parameter,
                                        const Symbol& symbol,
                                        double space,
                                        size_t transStart, size_t transEnd)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawSymbol(const Projection& projection,
                                 const MapParameter& parameter,
                                 const Symbol& style,
                                 double x, double y)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawIcon(const IconStyle* style,
                                 double x, double y)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawPath(const Projection& projection,
                               const MapParameter& parameter,
                               const Color& color,
                               double width,
                               const std::vector<double>& dash,
                               LineStyle::CapStyle startCap,
                               LineStyle::CapStyle endCap,
                               size_t transStart, size_t transEnd)
  {
    stream << "    <polyline fill=\"none\" stroke=\"" << GetColorValue(color) << "\" stroke-width=\"" << width << "\"" << std::endl;
    stream << "              points=\"";

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i!=transStart) {
        stream << " ";
      }

      stream << transBuffer.buffer[i].x << "," << transBuffer.buffer[i].y;

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawPath(const Projection& projection,
                               const MapParameter& parameter,
                               const std::string& styleName,
                               double width,
                               LineStyle::CapStyle startCap,
                               LineStyle::CapStyle endCap,
                               size_t transStart, size_t transEnd)
  {
    stream << "    <polyline class=\"" << styleName  << "\" stroke-width=\"" << width << "\"" << std::endl;
    stream << "              points=\"";

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i!=transStart) {
        stream << " ";
      }

      stream << transBuffer.buffer[i].x << "," << transBuffer.buffer[i].y;

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawWay(const StyleConfig& styleConfig,
                              const Projection& projection,
                              const MapParameter& parameter,
                              const WayData& data)
  {
    std::map<LineStyle,std::string>::const_iterator styleNameEntry=lineStyleNameMap.find(*data.lineStyle);

    assert(styleNameEntry!=lineStyleNameMap.end());

    if (!data.lineStyle->GetDash().empty() &&
        data.lineStyle->GetGapColor().GetA()>0.0) {
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapColor(),
               data.lineWidth,
               emptyDash,
               data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.transStart,data.transEnd);
    }

    DrawPath(projection,
             parameter,
             styleNameEntry->second,
             data.lineWidth,
             data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.transStart,data.transEnd);

    waysDrawn++;
  }

  void MapPainterSVG::DrawArea(const Projection& projection,
                               const MapParameter& parameter,
                               const MapPainter::AreaData& area)
  {
    std::map<FillStyle,std::string>::const_iterator styleNameEntry=fillStyleNameMap.find(*area.fillStyle);

    assert(styleNameEntry!=fillStyleNameMap.end());

    stream << "    <path class=\"" << styleNameEntry->second << "\"" << std::endl;

    if (!area.clippings.empty()) {
      stream << "          fillRule=\"evenodd\"";
    }

    stream << "          d=\"";

    stream << "M " << transBuffer.buffer[area.transStart].x << " " << transBuffer.buffer[area.transStart].y;
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      stream << " L " << transBuffer.buffer[i].x << " " << transBuffer.buffer[i].y;
    }
    stream << " Z";

    for (std::list<PolyData>::const_iterator c=area.clippings.begin();
        c!=area.clippings.end();
        c++) {
      const PolyData    &data=*c;

      stream << "M " << transBuffer.buffer[data.transStart].x << " " << transBuffer.buffer[data.transStart].y;
      for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
        stream << " L " << transBuffer.buffer[i].x << " " << transBuffer.buffer[i].y;
      }
      stream << " Z";
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
    stream << "          fill=\"" << GetColorValue(style.GetFillColor()) << "\"" << "/>" << std::endl;
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

    Draw(styleConfig,
         projection,
         parameter,
         data);

    WriteFooter();

    fillStyleNameMap.clear();
    lineStyleNameMap.clear();

    return true;
  }
}

