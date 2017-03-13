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

  MapPainterSVG::MapPainterSVG(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig,
               new CoordBufferImpl<Vertex2D>()),
    coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer),
    stream(NULL),
    typeConfig(NULL)
  {
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
#if !defined(GLIB_VERSION_2_36)
      /* Compatibility call for glibc versions prior to 2.36 */
    g_type_init();
#endif

    pangoContext=pango_context_new();
    pangoFontMap=pango_ft2_font_map_new();
    pango_context_set_font_map(pangoContext,
                               pangoFontMap);
#endif
  }

  MapPainterSVG::~MapPainterSVG()
  {
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
    for (FontMap::const_iterator entry=fonts.begin();
         entry!=fonts.end();
         ++entry) {
      if (entry->second!=NULL) {
        pango_font_description_free(entry->second);
      }
    }
#endif
  }

#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
  PangoFontDescription* MapPainterSVG::GetFont(const Projection& projection,
                                               const MapParameter& parameter,
                                               double fontSize)
  {
    FontMap::const_iterator f;

    fontSize=fontSize*projection.ConvertWidthToPixel(parameter.GetFontSize());

    f=fonts.find(fontSize);

    if (f!=fonts.end()) {
      return f->second;
    }

    PangoFontDescription* font=pango_font_description_new();

    pango_font_description_set_family(font,parameter.GetFontName().c_str());
    pango_font_description_set_absolute_size(font,fontSize*PANGO_SCALE);

    return fonts.insert(std::make_pair(fontSize,font)).first->second;
  }
#endif

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

  void MapPainterSVG::AfterPreprocessing(const StyleConfig& /*styleConfig*/,
                                         const Projection& projection,
                                         const MapParameter& /*parameter*/,
                                         const MapData& /*data*/)
  {
    stream << "  <defs>" << std::endl;
    stream << "    <style type=\"text/css\">" << std::endl;
    stream << "       <![CDATA[" << std::endl;

    size_t nextAreaId=0;
    for (const auto& area: GetAreaData()) {
      std::map<FillStyle,std::string>::const_iterator entry=fillStyleNameMap.find(*area.fillStyle);

      if (entry==fillStyleNameMap.end()) {
        std::string name="area_"+NumberToString(nextAreaId);

        fillStyleNameMap.insert(std::make_pair(*area.fillStyle,name));

        nextAreaId++;

        stream << "        ." << name << " {";

        stream << "fill:" << GetColorValue(area.fillStyle->GetFillColor());

        if (!area.fillStyle->GetFillColor().IsSolid()) {
          stream << ";fill-opacity:" << area.fillStyle->GetFillColor().GetA();
        }

        stream << ";fillRule:nonzero";

        double borderWidth=area.borderStyle ? projection.ConvertWidthToPixel(area.borderStyle->GetWidth()) : 0.0;

        if (borderWidth>0.0) {
          stream << ";stroke:" << GetColorValue(area.borderStyle->GetColor());

          if (!area.borderStyle->GetColor().IsSolid()) {
            stream << ";stroke-opacity:" << area.borderStyle->GetColor().GetA();
          }

          stream << ";stroke-width:" << borderWidth;

          if (area.borderStyle->HasDashes()) {
            stream << ";stroke-dasharray:";

            for (size_t i=0; i<area.borderStyle->GetDash().size(); i++) {
              if (i>0) {
                stream << ",";
              }

              stream << area.borderStyle->GetDash()[i]*borderWidth;
            }
          }
        }


        stream << "}" << std::endl;

      }
    }

    stream << std::endl;

    size_t nextWayId=0;
    for (const auto& way : GetWayData()) {
      std::map<LineStyle,std::string>::const_iterator entry=lineStyleNameMap.find(*way.lineStyle);

      if (entry==lineStyleNameMap.end()) {
        std::string name="way_"+NumberToString(nextWayId);

        lineStyleNameMap.insert(std::make_pair(*way.lineStyle,name));

        nextWayId++;

        double lineWidth;

        if (way.lineStyle->GetWidth()==0) {
          lineWidth=projection.ConvertWidthToPixel(way.lineStyle->GetDisplayWidth());
        }
        else {
          lineWidth=GetProjectedWidth(projection,
                                      projection.ConvertWidthToPixel(way.lineStyle->GetDisplayWidth()),
                                      way.lineStyle->GetWidth());
        }

        stream << "        ." << name << " {";
        stream << "fill:none;";
        stream << "stroke:" << GetColorValue(way.lineStyle->GetLineColor());

        if (!way.lineStyle->GetLineColor().IsSolid()) {
          stream << ";stroke-opacity:" << way.lineStyle->GetLineColor().GetA();
        }

        if (way.lineStyle->HasDashes()) {
          stream << ";stroke-dasharray:";

          for (size_t i=0; i<way.lineStyle->GetDash().size(); i++) {

            if (i>0) {
              stream << " ";
            }

            stream << way.lineStyle->GetDash()[i]*lineWidth;
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

  void MapPainterSVG::BeforeDrawing(const StyleConfig& /*styleConfig*/,
                                    const Projection& /*projection*/,
                                    const MapParameter& /*parameter*/,
                                    const MapData& /*data*/)
  {
    stream << "  <g id=\"map\">" << std::endl;
  }

  void MapPainterSVG::AfterDrawing(const StyleConfig& /*styleConfig*/,
                    const Projection& /*projection*/,
                    const MapParameter& /*parameter*/,
                    const MapData& /*data*/)
  {
    stream << "  </g>" << std::endl;
  }

  bool MapPainterSVG::HasIcon(const StyleConfig& /*styleConfig*/,
                              const MapParameter& /*parameter*/,
                              IconStyle& /*style*/)
  {
    // Not implemented

    return false;
  }

  void MapPainterSVG::GetFontHeight(const Projection& projection,
                                    const MapParameter& parameter,
                                    double fontSize,
                                    double& height)
  {
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
    PangoFontDescription *font;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    height=pango_font_description_get_size(font)/PANGO_SCALE;
#endif
  }

  void MapPainterSVG::GetTextDimension(const Projection& projection,
                                       const MapParameter& parameter,
                                       double /*objectWidth*/,
                                       double fontSize,
                                       const std::string& text,
                                       double& xOff,
                                       double& yOff,
                                       double& width,
                                       double& height)
  {
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
    PangoFontDescription *font;
    PangoLayout          *layout=pango_layout_new(pangoContext);
    PangoRectangle       extends;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    pango_layout_set_font_description(layout,font);
    pango_layout_set_text(layout,text.c_str(),text.length());

    pango_layout_get_pixel_extents(layout,&extends,NULL);

    xOff=extends.x;
    yOff=extends.y;
    width=extends.width;
    height=pango_font_description_get_size(font)/PANGO_SCALE;

    g_object_unref(layout);
#endif
  }

  void MapPainterSVG::DrawLabel(const Projection& projection,
                                const MapParameter& parameter,
                                const LabelData& label)
  {
    if (dynamic_cast<const TextStyle*>(label.style.get())!=NULL) {
      const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.get());
  #if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
      /*
      PangoFontDescription* font=GetFont(projection,
                                         parameter,
                                         label.fontSize);
      */
  #endif

      // TODO: This is not the exact placement, we cannot just move vertical by fontSize, but we must move the actual
      // text height. For this we need the text bounding box in LabelData.

      stream << "    <text";
      stream << " x=\"" << label.x << "\"";
      stream << " y=\"" << label.y+projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " font-family=\"" << parameter.GetFontName() << "\"";
      stream << " font-size=\"" << projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " fill=\"" << GetColorValue(style->GetTextColor()) << "\"";

      if (label.alpha!=1.0) {
        stream << " fill-opacity=\"" << label.alpha << "\"";
      }

      stream << ">";
      stream << label.text;
      stream << "</text>" << std::endl;

      /*
      stream << "<rect x=\"" << label.bx1 << "\"" << " y=\"" << label.by1 << "\"" <<" width=\"" << label.bx2-label.bx1 << "\"" << " height=\"" << label.by2-label.by1 << "\""
              << " fill=\"none\" stroke=\"blue\"/>" << std::endl;*/
    }
    else if (dynamic_cast<const ShieldStyle*>(label.style.get())!=NULL) {
      const ShieldStyle* style=dynamic_cast<const ShieldStyle*>(label.style.get());
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
     /*
     PangoFontDescription* font=GetFont(projection,
                                        parameter,
                                        label.fontSize);
     */
#endif
     // Shield background
     stream << "    <rect";
     stream << " x=\"" << label.bx1 << "\"";
     stream << " y=\"" << label.by1 << "\"";
     stream << " width=\"" << label.bx2-label.bx1+1 << "\"";
     stream << " height=\"" << label.by2-label.by1+1 << "\"";
     stream << " fill=\"" << GetColorValue(style->GetBgColor()) << "\"";
     stream << " stroke=\"none\"";
     stream <<  "/>" << std::endl;

     // Shield inner border
     stream << "    <rect";
     stream << " x=\"" << label.bx1+2 << "\"";
     stream << " y=\"" << label.by1+2 << "\"";
     stream << " width=\"" << label.bx2-label.bx1+1-4 << "\"";
     stream << " height=\"" << label.by2-label.by1+1-4 << "\"";
     stream << " fill=\"none\"";
     stream << " stroke=\"" << GetColorValue(style->GetBorderColor()) << "\"";
     stream << " stroke-width=\"1\"";
     stream <<  "/>" << std::endl;

      // TODO: This is not the exact placement, we cannot just move vertical by fontSize, but we must move the actual
      // text height. For this we need the text bounding box in LabelData.

      stream << "    <text";
      stream << " x=\"" << label.x << "\"";
      stream << " y=\"" << label.y+projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " font-family=\"" << parameter.GetFontName() << "\"";
      stream << " font-size=\"" << projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " fill=\"" << GetColorValue(style->GetTextColor()) << "\"";

      if (label.alpha!=1.0) {
        stream << " fill-opacity=\"" << label.alpha << "\"";
      }

      stream << ">";
      stream << label.text;
      stream << "</text>" << std::endl;
    }
  }

  void MapPainterSVG::DrawContourLabel(const Projection& /*projection*/,
                                       const MapParameter& /*parameter*/,
                                       const PathTextStyle& /*style*/,
                                       const std::string& /*text*/,
                                       size_t /*transStart*/, size_t /*transEnd*/)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawContourSymbol(const Projection& /*projection*/,
                                        const MapParameter& /*parameter*/,
                                        const Symbol& /*symbol*/,
                                        double /*space*/,
                                        size_t /*transStart*/, size_t /*transEnd*/)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawSymbol(const Projection& /*projection*/,
                                 const MapParameter& /*parameter*/,
                                 const Symbol& /*style*/,
                                 double /*x*/, double /*y*/)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawIcon(const IconStyle* /*style*/,
                                 double /*x*/, double /*y*/)
  {
    // Not implemented
  }

  void MapPainterSVG::DrawPath(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const Color& color,
                               double width,
                               const std::vector<double>& /*dash*/,
                               LineStyle::CapStyle /*startCap*/,
                               LineStyle::CapStyle /*endCap*/,
                               size_t transStart, size_t transEnd)
  {
    stream << "    <polyline";
    stream << " fill=\"none\"";
    stream << " stroke=\"" << GetColorValue(color) << "\"";

    if (!color.IsSolid()) {
      stream << " stroke-opacity=\"" << color.GetA() << "\"";
    }

    stream << " stroke-width=\"" << width << "\"";
    stream << std::endl;

    stream << "              points=\"";

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i!=transStart) {
        stream << " ";
      }

      stream << coordBuffer->buffer[i].GetX() << "," << coordBuffer->buffer[i].GetY();

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawPath(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const std::string& styleName,
                               double width,
                               LineStyle::CapStyle /*startCap*/,
                               LineStyle::CapStyle /*endCap*/,
                               size_t transStart, size_t transEnd)
  {
    stream << "    <polyline";
    stream << " class=\"" << styleName  << "\"";
    stream << " stroke-width=\"" << width << "\"";
    stream << std::endl;

    stream << "              points=\"";

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i!=transStart) {
        stream << " ";
      }

      stream << coordBuffer->buffer[i].GetX() << "," << coordBuffer->buffer[i].GetY();

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawWay(const StyleConfig& /*styleConfig*/,
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
  }

  void MapPainterSVG::DrawArea(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const MapPainter::AreaData& area)
  {
    std::map<FillStyle,std::string>::const_iterator styleNameEntry=fillStyleNameMap.find(*area.fillStyle);

    assert(styleNameEntry!=fillStyleNameMap.end());

    stream << "    <path class=\"" << styleNameEntry->second << "\"" << std::endl;

    if (!area.clippings.empty()) {
      stream << "          fillRule=\"evenodd\"" << std::endl;
    }

    stream << "          d=\"";

    stream << "M " << coordBuffer->buffer[area.transStart].GetX() << " " << coordBuffer->buffer[area.transStart].GetY();
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      stream << " L " << coordBuffer->buffer[i].GetX() << " " << coordBuffer->buffer[i].GetY();
    }
    stream << " Z";

    for (std::list<PolyData>::const_iterator c=area.clippings.begin();
        c!=area.clippings.end();
        c++) {
      const PolyData    &data=*c;

      stream << "M " << coordBuffer->buffer[data.transStart].GetX() << " " << coordBuffer->buffer[data.transStart].GetY();
      for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
        stream << " L " << coordBuffer->buffer[i].GetX() << " " << coordBuffer->buffer[i].GetY();
      }
      stream << " Z";
    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawGround(const Projection& projection,
                                 const MapParameter& /*parameter*/,
                                 const FillStyle& style)
  {
    stream << "    <rect x=\"" << 0 << "\" y=\"" << 0 << "\" width=\"" << projection.GetWidth() << "\" height=\"" << projection.GetHeight() << "\"" << std::endl;
    stream << "          fill=\"" << GetColorValue(style.GetFillColor()) << "\"" << "/>" << std::endl;
  }

  bool MapPainterSVG::DrawMap(const Projection& projection,
                              const MapParameter& parameter,
                              const MapData& data,
                              std::ostream& stream)
  {
    this->stream.rdbuf(stream.rdbuf());
    typeConfig=styleConfig->GetTypeConfig();

    WriteHeader(projection.GetWidth(),projection.GetHeight());

    Draw(projection,
         parameter,
         data);

    WriteFooter();

    fillStyleNameMap.clear();
    lineStyleNameMap.clear();

    return true;
  }
}

