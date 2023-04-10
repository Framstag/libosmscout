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

#include <osmscoutmapsvg/MapPainterSVG.h>

#include <iostream>
#include <iomanip>
#include <limits>
#include <list>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>
#include <osmscout/util/String.h>
#include <osmscout/util/File.h>
#include <osmscout/util/Base64.h>

namespace osmscout {

  static const char* valueChar="0123456789abcdef";

  MapPainterSVG::MapPainterSVG(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig),
    labelLayouter(this),
    stream(nullptr),
    typeConfig(nullptr)
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
      if (entry->second!=nullptr) {
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

  std::shared_ptr<MapPainterSVG::SvgLabel> MapPainterSVG::Layout(const Projection& projection,
                                                                 const MapParameter& parameter,
                                                                 const std::string& text,
                                                                 double fontSize,
                                                                 double objectWidth,
                                                                 bool enableWrapping,
                                                                 bool /*contourLabel*/)
  {
    auto label = std::make_shared<MapPainterSVG::SvgLabel>(
        std::shared_ptr<PangoLayout>(pango_layout_new(pangoContext), g_object_unref));

    PangoFontDescription* font=GetFont(projection, parameter, fontSize);

    pango_layout_set_font_description(label->label.get(),font);

    int proposedWidth=(int)std::ceil(objectWidth);

    pango_layout_set_text(label->label.get(),
                          text.c_str(),
                          (int)text.length());

    // layout 0,0 coordinate will be top-center
    pango_layout_set_alignment(label->label.get(), PANGO_ALIGN_CENTER);

    if (enableWrapping) {
      pango_layout_set_wrap(label->label.get(), PANGO_WRAP_WORD);
    }

    if (proposedWidth > 0) {
      pango_layout_set_width(label->label.get(), proposedWidth * PANGO_SCALE);
    }

    PangoRectangle extends;

    pango_layout_get_pixel_extents(label->label.get(),
                                   nullptr,
                                   &extends);

    label->text=text;
    label->fontSize=fontSize;
    label->width=extends.width;
    label->height=extends.height;

    return label;
  }

  DoubleRectangle MapPainterSVG::GlyphBoundingBox(const NativeGlyph &glyph) const
  {
    assert(glyph.glyphString->num_glyphs == 1);
    PangoRectangle extends;
    pango_font_get_glyph_extents(glyph.font.get(), glyph.glyphString->glyphs[0].glyph, nullptr, &extends);

    return DoubleRectangle((double)(extends.x) / (double)PANGO_SCALE,
                           (double)(extends.y) / (double)PANGO_SCALE,
                           (double)(extends.width) / (double)PANGO_SCALE,
                           (double)(extends.height) / (double)PANGO_SCALE);
  }

  template<>
  std::vector<Glyph<MapPainterSVG::NativeGlyph>> MapPainterSVG::SvgLabel::ToGlyphs() const
  {
    PangoRectangle extends;

    pango_layout_get_pixel_extents(label.get(),
                                   nullptr,
                                   &extends);

    // label is centered - we have to move its left horizontal offset
    double horizontalOffset = extends.x * -1.0;
    std::vector<Glyph<MapPainterSVG::NativeGlyph>> result;

    if constexpr (debugLabelLayouter) {
      std::cout << " = getting glyphs for label: " << text << std::endl;
    }

    for (PangoLayoutIter *iter = pango_layout_get_iter(label.get());
         iter != nullptr;){
      PangoLayoutRun *run = pango_layout_iter_get_run_readonly(iter);
      if (run == nullptr) {
        pango_layout_iter_free(iter);
        break; // nullptr signalise end of line, we don't expect more lines in contour label
      }
      std::wstring wstr = UTF8StringToWString(text.substr(run->item->offset, run->item->length));

      std::shared_ptr<PangoFont> font = std::shared_ptr<PangoFont>(run->item->analysis.font,
                                                                   g_object_unref);
      g_object_ref(font.get());

      if constexpr (debugLabelLayouter) {
        std::cout << "   run with " << run->glyphs->num_glyphs << " glyphs, " <<
                  wstr.size() << " length, " <<
                  run->item->num_chars << " chars " <<
                  "(font " << font.get() << "):" << std::endl;
      }

      for (int gi=0; gi < run->glyphs->num_glyphs; gi++){
        result.emplace_back();

        // new run with single glyph
        std::shared_ptr<PangoGlyphString> singleGlyphStr = std::shared_ptr<PangoGlyphString>(pango_glyph_string_new(), pango_glyph_string_free);

        pango_glyph_string_set_size(singleGlyphStr.get(), 1);

        // make glyph copy
        singleGlyphStr.get()->glyphs[0] = run->glyphs->glyphs[gi];
        PangoGlyphInfo &glyphInfo = singleGlyphStr.get()->glyphs[0];

        result.back().glyph.font = font;

        result.back().position.SetX(((double)glyphInfo.geometry.x_offset/(double)PANGO_SCALE) + horizontalOffset);
        result.back().position.SetY((double)glyphInfo.geometry.y_offset/(double)PANGO_SCALE);

        glyphInfo.geometry.x_offset = 0;
        glyphInfo.geometry.y_offset = 0;
        // TODO: it is correct to take x_offset into account? See pango_glyph_string_extents_range implementation...
        horizontalOffset += ((double)(glyphInfo.geometry.width + glyphInfo.geometry.x_offset)/(double)PANGO_SCALE);

        result.back().glyph.glyphString = singleGlyphStr;
        result.back().glyph.character = WStringToUTF8String(wstr.substr(gi, 1));

        if constexpr (debugLabelLayouter) {
          std::cout << "     " << glyphInfo.glyph << " \"" << result.back().glyph.character << "\": " <<
                    result.back().position.GetX() << " x " << result.back().position.GetY() << std::endl;
        }
      }

      if (!pango_layout_iter_next_run(iter)){
        pango_layout_iter_free(iter);
        iter = nullptr;
      }
    }

    return result;
  }

#else

  template<>
  std::vector<Glyph<MapPainterSVG::NativeGlyph>> MapPainterSVG::SvgLabel::ToGlyphs() const
  {
    std::vector<Glyph<MapPainterSVG::NativeGlyph>> result;
    double horizontalOffset = 0;
    for (size_t ch = 0; ch < label.length(); ch++){
      result.emplace_back();

      result.back().glyph.character = WStringToUTF8String(label.substr(ch,1));

      result.back().position=Vertex2D(horizontalOffset,
                                      0.0);

      horizontalOffset += (double)(height * MapPainterSVG::AverageCharacterWidth);
    }
    return result;
  }

  DoubleRectangle MapPainterSVG::GlyphBoundingBox(const NativeGlyph &glyph) const
  {
    return DoubleRectangle(0.0,
                           (double)(glyph.height * -1),
                           (double)glyph.width,
                           (double)glyph.height);
  }

  std::shared_ptr<MapPainterSVG::SvgLabel> MapPainterSVG::Layout(const Projection& projection,
                                                                 const MapParameter& parameter,
                                                                 const std::string& text,
                                                                 double fontSize,
                                                                 double /*objectWidth*/,
                                                                 bool /*enableWrapping*/,
                                                                 bool /*contourLabel*/)
  {
    auto label = std::make_shared<MapPainterSVG::SvgLabel>(UTF8StringToWString(text));

    label->text=text;
    label->fontSize=fontSize;
    label->height=projection.ConvertWidthToPixel(fontSize*parameter.GetFontSize());
    label->width=label->label.length() * label->height * AverageCharacterWidth;

    return label;
  }
#endif

  void MapPainterSVG::DrawLabel(const Projection &projection,
                                const MapParameter &parameter,
                                const DoubleRectangle &labelRectangle,
                                const LabelData &label,
                                const NativeLabel &/*layout*/)
  {
    if (const TextStyle* style = dynamic_cast<const TextStyle*>(label.style.get());
        style != nullptr) {

      // TODO: text x, y coordinate is text baseline, our placement is not precise

      stream << "    <text";
      stream << " x=\"" << labelRectangle.x << "\"";
      stream << " y=\"" << labelRectangle.y + projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " font-family=\"" << parameter.GetFontName() << "\"";
      stream << " font-size=\"" << projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " fill=\"" << GetColorValue(style->GetTextColor()) << "\"";

      if (label.alpha!=1.0) {
        stream << " fill-opacity=\"" << label.alpha << "\"";
      }

      stream << ">";
      stream << StrEscape(label.text);
      stream << "</text>" << std::endl;
    }
    else if (const ShieldStyle* style = dynamic_cast<const ShieldStyle*>(label.style.get());
             style != nullptr) {

      // Shield background
      stream << "    <rect";
      stream << " x=\"" << labelRectangle.x -2 << "\"";
      stream << " y=\"" << labelRectangle.y -0 << "\"";
      stream << " width=\"" << labelRectangle.width +4 << "\"";
      stream << " height=\"" << labelRectangle.height +1 << "\"";
      stream << " fill=\"" << GetColorValue(style->GetBgColor()) << "\"";
      stream << " stroke=\"none\"";
      stream <<  "/>" << std::endl;

      // Shield inner border
      stream << "    <rect";
      stream << " x=\"" << labelRectangle.x +0 << "\"";
      stream << " y=\"" << labelRectangle.y +2 << "\"";
      stream << " width=\"" << labelRectangle.width +4-4 << "\"";
      stream << " height=\"" << labelRectangle.height +1-4 << "\"";
      stream << " fill=\"none\"";
      stream << " stroke=\"" << GetColorValue(style->GetBorderColor()) << "\"";
      stream << " stroke-width=\"1\"";
      stream <<  "/>" << std::endl;

      // TODO: text x, y coordinate is text baseline, our placement is not precise

      stream << "    <text";
      stream << " x=\"" << labelRectangle.x << "\"";
      stream << " y=\"" << labelRectangle.y + projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " font-family=\"" << parameter.GetFontName() << "\"";
      stream << " font-size=\"" << projection.ConvertWidthToPixel(label.fontSize*parameter.GetFontSize()) << "\"";
      stream << " fill=\"" << GetColorValue(style->GetTextColor()) << "\"";

      if (label.alpha!=1.0) {
        stream << " fill-opacity=\"" << label.alpha << "\"";
      }

      stream << ">";
      stream << StrEscape(label.text);
      stream << "</text>" << std::endl;
    }

  }

  void MapPainterSVG::DrawGlyphs(const Projection &projection,
                                 const MapParameter &parameter,
                                 const osmscout::PathTextStyleRef style,
                                 const std::vector<SvgGlyph> &glyphs)
  {
    assert(!glyphs.empty());

    stream << "    <text";
    stream << " font-family=\"" << parameter.GetFontName() << "\"";
    stream << " font-size=\"" << projection.ConvertWidthToPixel(style->GetSize()*parameter.GetFontSize()) << "\"";
    stream << " fill=\"" << GetColorValue(style->GetTextColor()) << "\">";
    stream << std::endl;

    for (auto const &glyph:glyphs) {
      if (glyph.glyph.character.empty() ||
          (glyph.glyph.character.length()==1 &&
           (glyph.glyph.character==" " || glyph.glyph.character=="\t" || glyph.glyph.character==" "))) {
        continue;
      }

      stream << "        <tspan";
      stream << " x=\"" << glyph.position.GetX() << "\"";
      stream << " y=\"" << glyph.position.GetY() << "\"";
      stream << " rotate=\"" << RadToDeg(glyph.angle) << "\"";
      stream << ">" << StrEscape(glyph.glyph.character) << "</tspan>";
      stream << std::endl;
    }

    stream << "    </text>" << std::endl;
  }

  std::string MapPainterSVG::StrEscape(const std::string &str) const
  {
    std::string buffer;
    buffer.reserve(str.size());
    for(size_t pos = 0; pos != str.size(); ++pos) {
      switch(str[pos]) {
        case '&':  buffer.append("&amp;");      break;
        case '<':  buffer.append("&lt;");       break;
        case '>':  buffer.append("&gt;");       break;
        default:   buffer.append(&str[pos], 1); break;
      }
    }
    return buffer;
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
    stream << "  xmlns:xlink=\"http://www.w3.org/1999/xlink\"" << std::endl;
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

    stream << "        .area_fill_transparent { fill-opacity: 0.0; fillRule:nonzero }" << std::endl;

    stream << std::endl;

    size_t nextFillStyleId=0;
    size_t nextBorderStyleId=0;
    size_t nextWayId=0;

    auto addFillStyle = [&](const FillStyleRef &fillStyle){
      if (fillStyle) {
        std::map<FillStyle, std::string>::const_iterator entry = fillStyleNameMap.find(*fillStyle);

        if (entry == fillStyleNameMap.end()) {
          std::string name = "area_fill_" + std::to_string(nextFillStyleId);

          fillStyleNameMap.insert(std::make_pair(*fillStyle, name));

          nextFillStyleId++;

          stream << "        ." << name << " {";

          stream << "fill:" << GetColorValue(fillStyle->GetFillColor());

          if (!fillStyle->GetFillColor().IsSolid()) {
            stream << ";fill-opacity:" << fillStyle->GetFillColor().GetA();
          }

          stream << ";fillRule:nonzero";
          stream << "}" << std::endl;
        }
      }
    };

    auto addLineStyle = [&](const LineStyleRef &lineStyle){
      if (lineStyle){
        std::map<LineStyle,std::string>::const_iterator entry=lineStyleNameMap.find(*lineStyle);

        if (entry==lineStyleNameMap.end()) {
          std::string name="way_"+std::to_string(nextWayId);

          lineStyleNameMap.insert(std::make_pair(*lineStyle,name));

          nextWayId++;

          double lineWidth;

          if (lineStyle->GetWidth()==0) {
            lineWidth=projection.ConvertWidthToPixel(lineStyle->GetDisplayWidth());
          }
          else {
            lineWidth=GetProjectedWidth(projection,
                                        projection.ConvertWidthToPixel(lineStyle->GetDisplayWidth()),
                                        lineStyle->GetWidth());
          }

          stream << "        ." << name << " {";
          stream << "fill:none;";
          stream << "stroke:" << GetColorValue(lineStyle->GetLineColor());

          if (!lineStyle->GetLineColor().IsSolid()) {
            stream << ";stroke-opacity:" << lineStyle->GetLineColor().GetA();
          }

          if (lineStyle->HasDashes()) {
            stream << ";stroke-dasharray:";

            for (size_t i=0; i<lineStyle->GetDash().size(); i++) {

              if (i>0) {
                stream << " ";
              }

              stream << lineStyle->GetDash()[i]*lineWidth;
            }
          }

          stream << "}" << std::endl;
        }
      }
    };

    auto addBorderStyle = [&](const BorderStyleRef &borderStyle){
      if (borderStyle) {
        double borderWidth = projection.ConvertWidthToPixel(borderStyle->GetWidth());

        if (borderWidth > 0.0) {
          std::map<BorderStyle, std::string>::const_iterator borderEntry = borderStyleNameMap.find(*borderStyle);
          if (borderEntry == borderStyleNameMap.end()) {
            std::string name = "area_border_" + std::to_string(nextBorderStyleId);
            borderStyleNameMap.insert(std::make_pair(*borderStyle, name));
            assert( borderStyleNameMap.find(*borderStyle) != borderStyleNameMap.end());
            nextBorderStyleId++;

            stream << "        ." << name << " {";
            stream << "stroke:" << GetColorValue(borderStyle->GetColor());

            if (!borderStyle->GetColor().IsSolid()) {
              stream << ";stroke-opacity:" << borderStyle->GetColor().GetA();
            }

            stream << ";stroke-width:" << borderWidth;

            if (borderStyle->HasDashes()) {
              stream << ";stroke-dasharray:";

              for (size_t i = 0; i < borderStyle->GetDash().size(); i++) {
                if (i > 0) {
                  stream << ",";
                }

                stream << borderStyle->GetDash()[i] * borderWidth;
              }
            }
            stream << "}" << std::endl;
          }
        }
      }
    };

    // landFill
    addFillStyle(styleConfig->GetLandFillStyle(projection));
    // seaFill
    addFillStyle(styleConfig->GetSeaFillStyle(projection));
    //coastFill
    addFillStyle(styleConfig->GetCoastFillStyle(projection));
    // unknownFill
    addFillStyle(styleConfig->GetUnknownFillStyle(projection));
    // fallbackSeaFill
    addFillStyle(this->seaFill);
    // fallbackLandFill
    addFillStyle(this->landFill);

    for (const auto& area: GetAreaData()) {
      addFillStyle(area.fillStyle);
      addBorderStyle(area.borderStyle);
    }

    stream << std::endl;

    // coastlineLine
    addLineStyle(styleConfig->GetCoastlineLineStyle(projection));

    for (const auto& way : GetWayData()) {
      addLineStyle(way.lineStyle);
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
                                    const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& /*data*/)
  {
    stream << "  <g id=\"map\">" << std::endl;

    DoubleRectangle viewport(0.0,0.0,
                             (double)projection.GetWidth(), (double)projection.GetHeight());

    labelLayouter.SetViewport(viewport);
    labelLayouter.SetLayoutOverlap(projection.ConvertWidthToPixel(parameter.GetLabelLayouterOverlap()));
  }

  void MapPainterSVG::AfterDrawing(const StyleConfig& /*styleConfig*/,
                    const Projection& /*projection*/,
                    const MapParameter& /*parameter*/,
                    const MapData& /*data*/)
  {
    stream << "  </g>" << std::endl;
  }

  bool MapPainterSVG::HasIcon(const StyleConfig& /*styleConfig*/,
                              const Projection& projection,
                              const MapParameter& parameter,
                              IconStyle& style)
  {
    if (style.GetIconId()==0) {
      return false;
    }

    size_t idx=style.GetIconId()-1;

    // there is possible that exists multiple IconStyle instances with same iconId (point and area icon with same icon name)
    // setup dimensions for all of them
    double dimension;
    if (parameter.GetIconMode()==MapParameter::IconMode::Scalable) {
      dimension = projection.ConvertWidthToPixel(parameter.GetIconSize());
    } else if( parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap){
      dimension = std::round(projection.ConvertWidthToPixel(parameter.GetIconSize()));
    }else{
      dimension = std::round(parameter.GetIconPixelSize());
    }

    style.SetWidth(dimension);
    style.SetHeight(dimension);

    if (idx<images.size() &&
        !images[idx].empty()) {
      return true;
    }

    for (const auto& path : parameter.GetIconPaths()) {
      std::string filename;
      std::string mime;
      if (parameter.GetIconMode()==MapParameter::IconMode::Scalable) {
        filename = AppendFileToDir(path, style.GetIconName() + ".svg");
        mime = "image/svg+xml";
      } else {
        filename = AppendFileToDir(path, style.GetIconName() + ".png");
        mime = "image/png";
      }
      std::vector<char> content;
      if (ReadFile(filename, content)){
        // std::cout << filename << " Base64Encode: " << Base64Encode(content) << std::endl;
        if (idx >= images.size()) {
          images.resize(idx + 1);
        }

        images[idx] = "data:" + mime + ";base64," + Base64Encode(content);

        return true;
      }
    }

    style.SetIconId(0);

    return false;
  }

  double MapPainterSVG::GetFontHeight(const Projection& projection,
                                      const MapParameter& parameter,
                                      double fontSize)
  {
#if defined(OSMSCOUT_MAP_SVG_HAVE_LIB_PANGO)
    PangoFontDescription *font;

    font=GetFont(projection,
                 parameter,
                 fontSize);

    return pango_font_description_get_size(font)/PANGO_SCALE;
#else
    return projection.ConvertWidthToPixel(fontSize*parameter.GetFontSize());;
#endif
  }

  void MapPainterSVG::DrawContourSymbol(const Projection& /*projection*/,
                                        const MapParameter& /*parameter*/,
                                        const Symbol& /*symbol*/,
                                        const ContourSymbolData& /*data*/)
  {
    // Not implemented
  }

  void MapPainterSVG::SetupFillAndStroke(const FillStyleRef &fillStyle,
                                         const BorderStyleRef &borderStyle)
  {
    stream << " fill=\"" << (fillStyle && fillStyle->GetFillColor().IsVisible() ? GetColorValue(fillStyle->GetFillColor()) : "none") << "\"";
    stream << " stroke=\"" << (borderStyle && borderStyle->GetColor().IsVisible() ? GetColorValue(borderStyle->GetColor()) : "none") << "\"";

    if (borderStyle) {
      if (!borderStyle->GetColor().IsSolid()) {
        stream << " stroke-opacity=\"" << borderStyle->GetColor().GetA() << "\"";
      }
      stream << " stroke-width=\"" << borderStyle->GetWidth() << "\"";
    }
  }

  void MapPainterSVG::RegisterRegularLabel(const Projection &projection,
                                           const MapParameter &parameter,
                                           const std::vector<LabelData> &labels,
                                           const Vertex2D &position,
                                           double objectWidth)
  {
    labelLayouter.RegisterLabel(projection, parameter, position, labels, objectWidth);
  }

  /**
   * Register contour label
   */
  void MapPainterSVG::RegisterContourLabel(const Projection &projection,
                                           const MapParameter &parameter,
                                           const PathLabelData &label,
                                           const LabelPath &labelPath)
  {
    labelLayouter.RegisterContourLabel(projection,
                                       parameter,
                                       label,
                                       labelPath);
  }

  void MapPainterSVG::IconData(const Projection& projection,
                               const MapParameter& parameter)
  {
    double dimension;

    if (parameter.GetIconMode()==MapParameter::IconMode::Scalable) {
      dimension = projection.ConvertWidthToPixel(parameter.GetIconSize());
    } else if( parameter.GetIconMode()==MapParameter::IconMode::ScaledPixmap){
      dimension = std::round(projection.ConvertWidthToPixel(parameter.GetIconSize()));
    }else{
      dimension = std::round(parameter.GetIconPixelSize());
    }

    stream << "    <defs>\n";
    for (size_t i = 0; i < images.size(); i++) {
      if (images[i].empty()){
        continue;
      }
      stream << "        <image id=\"icon_" << (i+1) << "\" with=\"" << dimension <<
             "\" height=\"" << dimension << "\" xlink:href=\"" << images[i] << "\" />\n";
    }
    stream << "    </defs>\n";
  }

  void MapPainterSVG::DrawLabels(const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& /*data*/)
  {
    // insert data of icons
    IconData(projection, parameter);

    labelLayouter.Layout(projection, parameter);

    labelLayouter.DrawLabels(projection,
                             parameter,
                             this);

    labelLayouter.Reset();
  }

  void MapPainterSVG::DrawSymbol(const Projection& projection,
                                 const MapParameter& /*parameter*/,
                                 const Symbol& symbol,
                                 const Vertex2D& screenPos,
                                 double /*scaleFactor*/)
  {
    ScreenBox boundingBox=symbol.GetBoundingBox(projection);
    Vertex2D center=boundingBox.GetCenter();

    stream << "    <!-- symbol: " << symbol.GetName() << " -->" << std::endl;
    for (const auto& primitive : symbol.GetPrimitives()) {
      const DrawPrimitive *primitivePtr = primitive.get();

      const auto *polygon=dynamic_cast<const PolygonPrimitive*>(primitivePtr);
      const auto *rectangle=dynamic_cast<const RectanglePrimitive*>(primitivePtr);
      const auto *circle=dynamic_cast<const CirclePrimitive*>(primitivePtr);

      if (polygon != nullptr) {
        const FillStyleRef   fillStyle=polygon->GetFillStyle();
        const BorderStyleRef borderStyle=polygon->GetBorderStyle();

        stream << "    <polyline";
        SetupFillAndStroke(fillStyle, borderStyle);
        stream << std::endl;

        stream << "              points=\"";

        for (auto pixel=polygon->GetCoords().begin();
             pixel!=polygon->GetCoords().end();
             ++pixel) {
          if (pixel!=polygon->GetCoords().begin()) {
            stream << " ";
          }

          stream << (screenPos.GetX()+projection.ConvertWidthToPixel(pixel->GetX())-center.GetX())
                 << "," << (screenPos.GetY()+projection.ConvertWidthToPixel(pixel->GetY())-center.GetY());
        }

        stream << "\" />" << std::endl;

      } else if (rectangle != nullptr) {
        const FillStyleRef   fillStyle=rectangle->GetFillStyle();
        const BorderStyleRef borderStyle=rectangle->GetBorderStyle();

        stream << "    <rect";
        stream << " x=\"" << ((screenPos.GetX()+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX())-center.GetX())) << "\"";
        stream << " y=\"" << ((screenPos.GetY()+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetY())-center.GetY())) << "\"";
        stream << " width=\"" << (projection.ConvertWidthToPixel(rectangle->GetWidth())) << "\"";
        stream << " height=\"" << (projection.ConvertWidthToPixel(rectangle->GetHeight())) << "\"";

        SetupFillAndStroke(fillStyle, borderStyle);
        stream << " />" << std::endl;

      } else if (circle != nullptr) {
        const FillStyleRef   fillStyle=circle->GetFillStyle();
        const BorderStyleRef borderStyle=circle->GetBorderStyle();

        stream << "    <circle";
        stream << " cx=\"" << (screenPos.GetX()+projection.ConvertWidthToPixel(circle->GetCenter().GetX())-center.GetX()) << "\"";
        stream << " cy=\"" << (screenPos.GetY()+projection.ConvertWidthToPixel(circle->GetCenter().GetY())-center.GetY()) << "\"";
        stream << " r=\"" << (projection.ConvertWidthToPixel(circle->GetRadius())) << "\"";

        SetupFillAndStroke(fillStyle, borderStyle);
        stream << " />" << std::endl;
      }
    }
  }

  void MapPainterSVG::DrawIcon(const IconStyle* style,
                               const Vertex2D& centerPos,
                               double width, double height)
  {
    stream << "    <use x=\"" << (centerPos.GetX() - width/2) << "\" y=\"" << (centerPos.GetY() - height/2) << "\" " <<
      "width=\"" << width << "\" height=\"" << height << "\" " <<
      "xlink:href=\"#icon_" << style->GetIconId() << "\" />\n";
  }

  void MapPainterSVG::DrawPath(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const Color& color,
                               double width,
                               const std::vector<double>& dash,
                               LineStyle::CapStyle /*startCap*/,
                               LineStyle::CapStyle /*endCap*/,
                               const CoordBufferRange& coordRange)
  {
    stream << "    <polyline";
    stream << " fill=\"none\"";
    stream << " stroke=\"" << GetColorValue(color) << "\"";

    if (!color.IsSolid()) {
      stream << " stroke-opacity=\"" << color.GetA() << "\"";
    }

    stream << " stroke-width=\"" << width << "\"";

    if (!dash.empty()) {
      stream << " style=\"stroke-dasharray:";
      for (const auto &size:dash) {
        stream << " " << (size*width);
      }
      stream << "\"";
    }

    stream << std::endl;

    stream << "              points=\"";

    for (size_t i=coordRange.GetStart(); i<=coordRange.GetEnd(); i++) {
      if (i!=coordRange.GetStart()) {
        stream << " ";
      }

      stream << coordBuffer.buffer[i].GetX() << "," << coordBuffer.buffer[i].GetY();

    }

    stream << "\" />" << std::endl;
  }

  void MapPainterSVG::DrawPath(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const std::string& styleName,
                               const std::optional<Color> &colorOverride,
                               double width,
                               LineStyle::CapStyle /*startCap*/,
                               LineStyle::CapStyle /*endCap*/,
                               size_t transStart, size_t transEnd)
  {
    stream << "    <polyline";
    stream << " class=\"" << styleName  << "\"";
    if (colorOverride) {
      stream << " style=\"stroke:" << GetColorValue(*colorOverride);
      if (!colorOverride->IsSolid()) {
        stream << " stroke-opacity:" << colorOverride->GetA();
      }
      stream << "\"";
    }
    stream << " stroke-width=\"" << width << "\"";
    stream << std::endl;

    stream << "              points=\"";

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i!=transStart) {
        stream << " ";
      }

      stream << coordBuffer.buffer[i].GetX() << "," << coordBuffer.buffer[i].GetY();

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

    std::optional<Color> colorOverride = data.color != data.lineStyle->GetLineColor() ?
        std::make_optional(data.color) : std::nullopt;

    if (!data.lineStyle->GetDash().empty() &&
        data.lineStyle->GetGapColor().GetA()>0.0) {
      DrawPath(projection,
               parameter,
               data.lineStyle->GetGapColor(),
               data.lineWidth,
               emptyDash,
               data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
               data.coordRange);
    }

    DrawPath(projection,
             parameter,
             styleNameEntry->second,
             colorOverride,
             data.lineWidth,
             data.startIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.endIsClosed ? data.lineStyle->GetEndCap() : data.lineStyle->GetJoinCap(),
             data.coordRange.GetStart(),data.coordRange.GetEnd());
  }

  void MapPainterSVG::DrawArea(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
                               const MapPainter::AreaData& area)
  {

    std::string styleClasses;
    if (area.fillStyle) {
      std::map<FillStyle, std::string>::const_iterator styleNameEntry = fillStyleNameMap.find(*area.fillStyle);
      assert(styleNameEntry != fillStyleNameMap.end());
      styleClasses = styleNameEntry->second;
    }else{
      styleClasses="area_fill_transparent";
    }

    if (area.borderStyle) {
      std::map<BorderStyle, std::string>::const_iterator styleNameEntry = borderStyleNameMap.find(*area.borderStyle);
      if (styleNameEntry != borderStyleNameMap.end()) { // border style can be undefined when borderWidth==0
        styleClasses += " " + styleNameEntry->second;
      }
    }

    stream << "    <path ";
    if (!styleClasses.empty()) {
      stream << "class=\"" << styleClasses << "\"";
    }
    stream << std::endl;

    if (!area.clippings.empty()) {
      stream << "          fillRule=\"evenodd\"" << std::endl;
    }

    stream << "          d=\"";

    stream << "M " << coordBuffer.buffer[area.coordRange.GetStart()].GetX() << " " << coordBuffer.buffer[area.coordRange.GetStart()].GetY();
    for (size_t i=area.coordRange.GetStart()+1; i<=area.coordRange.GetEnd(); i++) {
      stream << " L " << coordBuffer.buffer[i].GetX() << " " << coordBuffer.buffer[i].GetY();
    }
    stream << " Z";

    for (const auto& data : area.clippings) {
      stream << "M " << coordBuffer.buffer[data.GetStart()].GetX() << " " << coordBuffer.buffer[data.GetStart()].GetY();
      for (size_t i=data.GetStart()+1; i<=data.GetEnd(); i++) {
        stream << " L " << coordBuffer.buffer[i].GetX() << " " << coordBuffer.buffer[i].GetY();
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
    std::lock_guard<std::mutex> guard(mutex);
    bool                        result=true;

    this->stream.rdbuf(stream.rdbuf());
    typeConfig=styleConfig->GetTypeConfig();

    WriteHeader(projection.GetWidth(),projection.GetHeight());

    result=Draw(projection,
                parameter,
                data);

    WriteFooter();

    fillStyleNameMap.clear();
    borderStyleNameMap.clear();
    lineStyleNameMap.clear();

    return result;
  }
}

