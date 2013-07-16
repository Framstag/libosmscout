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

#include <osmscout/MapPainterAgg.h>

#include <iostream>
#include <limits>

#include <agg2/agg_conv_bspline.h>
#include <agg2/agg_conv_dash.h>
#include <agg2/agg_conv_segmentator.h>
#include <agg2/agg_conv_stroke.h>
#include <agg2/agg_ellipse.h>
#include <agg2/agg_trans_single_path.h>

#include <osmscout/system/Assert.h>
#include <osmscout/system/Math.h>

#include <osmscout/util/Geometry.h>
#include <osmscout/util/String.h>

namespace osmscout {

  MapPainterAgg::MapPainterAgg()
  {
    // no code
  }

  MapPainterAgg::~MapPainterAgg()
  {
    // no code
    // TODO: Clean up fonts
  }

  void MapPainterAgg::SetFont(const MapParameter& parameter,
                              double size)
  {
    if (!fontEngine->load_font(parameter.GetFontName().c_str(),
                               0,
                               agg::glyph_ren_native_gray8)) {
      std::cout << "Cannot load font '" << parameter.GetFontName() << "'" << std::endl;
      return;

    }

    //fontEngine->resolution(72);
    fontEngine->width(size*ConvertWidthToPixel(parameter,parameter.GetFontSize()));
    fontEngine->height(size*ConvertWidthToPixel(parameter,parameter.GetFontSize()));
    fontEngine->hinting(true);
    fontEngine->flip_y(true);
  }

  void MapPainterAgg::SetOutlineFont(const MapParameter& parameter,
                                     double size)
  {
    if (!fontEngine->load_font(parameter.GetFontName().c_str(),
                               0,
                               agg::glyph_ren_outline)) {
      std::cout << "Cannot load font '" << parameter.GetFontName() << "'" << std::endl;
      return;

    }

    //fontEngine->resolution(72);
    fontEngine->width(size*ConvertWidthToPixel(parameter,parameter.GetFontSize()));
    fontEngine->height(size*ConvertWidthToPixel(parameter,parameter.GetFontSize()));
    fontEngine->hinting(true);
    fontEngine->flip_y(true);
  }

  void MapPainterAgg::GetTextDimension(const std::wstring& text,
                                       double& width,
                                       double& height)
  {

    width=0;
    height=fontEngine->height();

    for (size_t i=0; i<text.length(); i++) {
      const agg::glyph_cache* glyph=fontCacheManager->glyph(text[i]);

      if (glyph!=NULL) {
        width+=glyph->advance_x;
      }
    }
  }

  void MapPainterAgg::GetTextDimension(const MapParameter& parameter,
                                       double fontSize,
                                       const std::string& text,
                                       double& xOff,
                                       double& yOff,
                                       double& width,
                                       double& height)
  {
    std::wstring wideText(UTF8StringToWString(text));

    xOff=0;
    yOff=0;
    width=0;
    height=fontEngine->height();

    SetFont(parameter,fontSize);

    for (size_t i=0; i<wideText.length(); i++) {
      const agg::glyph_cache* glyph=fontCacheManager->glyph(wideText[i]);

      if (glyph!=NULL) {
        width+=glyph->advance_x;
      }
    }
  }

  void MapPainterAgg::DrawText(double x,
                               double y,
                               const std::wstring& text)
  {
    for (size_t i=0; i<text.length(); i++) {
      const agg::glyph_cache* glyph = fontCacheManager->glyph(text[i]);

      if (glyph!=NULL) {
        if (true) {
          fontCacheManager->add_kerning(&x, &y);
        }

        fontCacheManager->init_embedded_adaptors(glyph,x,y);

        switch (glyph->data_type) {
        default:
          break;
        case agg::glyph_data_mono:
          agg::render_scanlines(fontCacheManager->mono_adaptor(),
                                fontCacheManager->mono_scanline(),
                                *renderer_bin);
          break;

        case agg::glyph_data_gray8:
          agg::render_scanlines(fontCacheManager->gray8_adaptor(),
                                fontCacheManager->gray8_scanline(),
                                *renderer_aa);
          break;

        case agg::glyph_data_outline:
          rasterizer->reset();

          if(convTextContours->width() <= 0.01) {
            rasterizer->add_path(*convTextCurves);
          }
          else {
            rasterizer->add_path(*convTextContours);
          }
          agg::render_scanlines(*rasterizer,
                                *scanlineP8,
                                *renderer_aa);
          break;
        }

        // increment pen position
        x += glyph->advance_x;
        y += glyph->advance_y;
      }
    }
  }

  void MapPainterAgg::DrawOutlineText(double x,
                                      double y,
                                      const std::wstring& text,
                                      double width)
  {
    convTextContours->width(width);

    for (size_t i=0; i<text.length(); i++) {
      const agg::glyph_cache* glyph = fontCacheManager->glyph(text[i]);

      if (glyph!=NULL) {
        if (true) {
          fontCacheManager->add_kerning(&x, &y);
        }

        fontCacheManager->init_embedded_adaptors(glyph,x,y);

        switch (glyph->data_type) {
        default:
          break;
        case agg::glyph_data_mono:
          agg::render_scanlines(fontCacheManager->mono_adaptor(),
                                fontCacheManager->mono_scanline(),
                                *renderer_bin);
          break;
        case agg::glyph_data_gray8:
          agg::render_scanlines(fontCacheManager->gray8_adaptor(),
                                fontCacheManager->gray8_scanline(),
                                *renderer_aa);
          break;
        case agg::glyph_data_outline:
          rasterizer->reset();

          if(convTextContours->width() <= 0.01) {
            rasterizer->add_path(*convTextCurves);
          }
          else {
            rasterizer->add_path(*convTextContours);
          }
          agg::render_scanlines(*rasterizer,
                                *scanlineP8,
                                *renderer_aa);
          break;
        }

        // increment pen position
        x += glyph->advance_x;
        y += glyph->advance_y;
      }
    }
  }

  void MapPainterAgg::DrawFill(const Projection& projection,
                               const MapParameter& parameter,
                               const FillStyle& fillStyle,
                               agg::path_storage& path)
  {
    if (fillStyle.GetFillColor().IsVisible()) {
      renderer_aa->color(agg::rgba(fillStyle.GetFillColor().GetR(),
                                   fillStyle.GetFillColor().GetG(),
                                   fillStyle.GetFillColor().GetB(),
                                   fillStyle.GetFillColor().GetA()));

      agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
    }

    double borderWidth=ConvertWidthToPixel(parameter,
                                           fillStyle.GetBorderWidth());

    if (borderWidth>=parameter.GetLineMinWidthPixel()) {
      renderer_aa->color(agg::rgba(fillStyle.GetBorderColor().GetR(),
                                   fillStyle.GetBorderColor().GetG(),
                                   fillStyle.GetBorderColor().GetB(),
                                   fillStyle.GetBorderColor().GetA()));

      if (fillStyle.GetBorderDash().empty()) {
        agg::conv_stroke<agg::path_storage> stroke(path);

        stroke.width(borderWidth);
        stroke.line_cap(agg::round_cap);

        rasterizer->add_path(stroke);

        agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
      }
      else {
        agg::conv_dash<agg::path_storage>                    dasher(path);
        agg::conv_stroke<agg::conv_dash<agg::path_storage> > stroke(dasher);

        stroke.width(borderWidth);

        stroke.line_cap(agg::butt_cap);

        for (size_t i=0; i<fillStyle.GetBorderDash().size(); i+=2) {
          dasher.add_dash(fillStyle.GetBorderDash()[i]*borderWidth,
                          fillStyle.GetBorderDash()[i+1]*borderWidth);
        }

        rasterizer->add_path(stroke);

        agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
      }
    }
  }

  bool MapPainterAgg::HasIcon(const StyleConfig& styleConfig,
                             const MapParameter& parameter,
                             IconStyle& style)
  {
    //TODO

    return false;
  }

  void MapPainterAgg::DrawLabel(const Projection& projection,
                                const MapParameter& parameter,
                                const LabelData& label)
  {
    if (dynamic_cast<const TextStyle*>(label.style.Get())!=NULL) {
      const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.Get());
      double           r=style->GetTextColor().GetR();
      double           g=style->GetTextColor().GetG();
      double           b=style->GetTextColor().GetB();
      std::wstring     wideText(UTF8StringToWString(label.text));

      if (style->GetStyle()==TextStyle::normal) {

        SetFont(parameter,
                label.fontSize);

        //renderer_bin->color(agg::rgba(r,g,b,a));
        renderer_aa->color(agg::rgba(r,g,b,label.alpha));

        DrawText(label.x,
                 label.y+fontEngine->ascender(),
                 wideText);
      }
      else if (style->GetStyle()==TextStyle::emphasize) {
        SetOutlineFont(parameter,
                       label.fontSize);

        //renderer_bin->color(agg::rgba(r,g,b,a));
        renderer_aa->color(agg::rgba(1,1,1,label.alpha));

        DrawOutlineText(label.x,
                        label.y+fontEngine->ascender(),
                        wideText,
                        2);

        SetFont(parameter,
                label.fontSize);

        //renderer_bin->color(agg::rgba(r,g,b,a));
        renderer_aa->color(agg::rgba(r,g,b,label.alpha));

        DrawText(label.x,
                 label.y+fontEngine->ascender(),
                 wideText);
      }
    }
  }

  void MapPainterAgg::DrawContourLabel(const Projection& projection,
                                       const MapParameter& parameter,
                                       const PathTextStyle& style,
                                       const std::string& text,
                                       size_t transStart, size_t transEnd)
  {
    double       fontSize=style.GetSize();
    double       r=style.GetTextColor().GetR();
    double       g=style.GetTextColor().GetG();
    double       b=style.GetTextColor().GetB();
    double       a=style.GetTextColor().GetA();
    std::wstring wideText(UTF8StringToWString(text));

    SetOutlineFont(parameter,
                   fontSize);

    //renderer_bin->color(agg::rgba(r,g,b,a));
    renderer_aa->color(agg::rgba(r,g,b,a));

    agg::path_storage path;

    double length=0;
    double xo=0;
    double yo=0;

    if (transBuffer.buffer[transStart].x<transBuffer.buffer[transEnd].x) {
      for (size_t j=transStart; j<=transEnd; j++) {
        if (j==transStart) {
          path.move_to(transBuffer.buffer[j].x,
                       transBuffer.buffer[j].y);
        }
        else {
          path.line_to(transBuffer.buffer[j].x,
                       transBuffer.buffer[j].y);
          length+=sqrt(pow(transBuffer.buffer[j].x-xo,2)+
                       pow(transBuffer.buffer[j].y-yo,2));
        }

        xo=transBuffer.buffer[j].x;
        yo=transBuffer.buffer[j].y;
      }
    }
    else {
      for (size_t j=0; j<=transEnd-transStart; j++) {
        size_t idx=transEnd-j;

        if (j==0) {
          path.move_to(transBuffer.buffer[idx].x,
                       transBuffer.buffer[idx].y);
        }
        else {
          path.line_to(transBuffer.buffer[idx].x,
                       transBuffer.buffer[idx].y);
          length+=sqrt(pow(transBuffer.buffer[idx].x-xo,2)+
                       pow(transBuffer.buffer[idx].y-yo,2));
        }

        xo=transBuffer.buffer[idx].x;
        yo=transBuffer.buffer[idx].y;
      }
    }

    double width;
    double height;

    GetTextDimension(wideText,width,height);

    if (width>length) {
      return;
    }

    /*
    typedef agg::conv_bspline<agg::path_storage> conv_bspline_type;

    conv_bspline_type bspline(path);
    bspline.interpolation_step(1.0 / path.total_vertices());*/

    agg::trans_single_path tcurve;
    tcurve.add_path(path); // bspline

    typedef agg::conv_segmentator<AggTextCurveConverter> conv_font_segm_type;
    typedef agg::conv_transform<conv_font_segm_type,
    agg::trans_single_path>                              conv_font_trans_type;

    conv_font_segm_type  fsegm(*convTextCurves);
    conv_font_trans_type ftrans(fsegm, tcurve);

    fsegm.approximation_scale(3.0);

    double x=(length-width)/2;
    double y=-height/2+fontEngine->ascender();

    for (size_t i=0; i<wideText.length(); i++) {
      const agg::glyph_cache* glyph = fontCacheManager->glyph(wideText[i]);

      if (glyph!=NULL) {
        fontCacheManager->add_kerning(&x, &y);
        fontCacheManager->init_embedded_adaptors(glyph,x,y);

        if (glyph->data_type==agg::glyph_data_outline) {
          rasterizer->reset();
          rasterizer->add_path(ftrans);
          renderer_aa->color(agg::rgba(r,g,b,a));
          agg::render_scanlines(*rasterizer,
                                *scanlineP8,
                                *renderer_aa);
        }

        // increment pen position
        x += glyph->advance_x;
        y += glyph->advance_y;
      }
    }
  }

  void MapPainterAgg::DrawContourSymbol(const Projection& projection,
                                        const MapParameter& parameter,
                                        const Symbol& symbol,
                                        double space,
                                        size_t transStart, size_t transEnd)
  {
    // Not implemented
  }

  void MapPainterAgg::DrawIcon(const IconStyle* style,
                              double x, double y)
  {
    // Not implemented
  }

  void MapPainterAgg::DrawSymbol(const Projection& projection,
                                 const MapParameter& parameter,
                                 const Symbol& symbol,
                                 double x, double y)
  {
    double minX;
    double minY;
    double maxX;
    double maxY;
    double centerX;
    double centerY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    centerX=maxX-minX;
    centerY=maxY-minY;

    for (std::list<DrawPrimitiveRef>::const_iterator p=symbol.GetPrimitives().begin();
         p!=symbol.GetPrimitives().end();
         ++p) {
      DrawPrimitive* primitive=p->Get();

      if (dynamic_cast<PolygonPrimitive*>(primitive)!=NULL) {
        PolygonPrimitive* polygon=dynamic_cast<PolygonPrimitive*>(primitive);
        FillStyleRef      style=polygon->GetFillStyle();
        agg::path_storage path;

        for (std::list<Coord>::const_iterator pixel=polygon->GetCoords().begin();
             pixel!=polygon->GetCoords().end();
             ++pixel) {
          if (pixel==polygon->GetCoords().begin()) {
            path.move_to(x+ConvertWidthToPixel(parameter,pixel->x-centerX),
                         y+ConvertWidthToPixel(parameter,maxY-pixel->y-centerY));
          }
          else {
            path.line_to(x+ConvertWidthToPixel(parameter,pixel->x-centerX),
                         y+ConvertWidthToPixel(parameter,maxY-pixel->y-centerY));
          }
        }

        path.close_polygon();

        rasterizer->add_path(path);

        DrawFill(projection,
                 parameter,
                 *style,
                 path);
      }
      else if (dynamic_cast<RectanglePrimitive*>(primitive)!=NULL) {
        RectanglePrimitive* rectangle=dynamic_cast<RectanglePrimitive*>(primitive);
        FillStyleRef        style=rectangle->GetFillStyle();
        agg::path_storage   path;
        double              xPos=x+ConvertWidthToPixel(parameter,rectangle->GetTopLeft().x-centerX);
        double              yPos=y+ConvertWidthToPixel(parameter,maxY-rectangle->GetTopLeft().y-centerY);
        double              width=ConvertWidthToPixel(parameter,rectangle->GetWidth());
        double              height=ConvertWidthToPixel(parameter,rectangle->GetHeight());

        path.move_to(xPos,yPos);
        path.line_to(xPos+width,yPos);
        path.line_to(xPos+width,yPos+height);
        path.line_to(xPos,yPos+height);

        path.close_polygon();

        rasterizer->add_path(path);

        DrawFill(projection,
                 parameter,
                 *style,
                 path);
      }
      else if (dynamic_cast<CirclePrimitive*>(primitive)!=NULL) {
        CirclePrimitive*  circle=dynamic_cast<CirclePrimitive*>(primitive);
        FillStyleRef      style=circle->GetFillStyle();
        agg::path_storage path;
        double            radius=ConvertWidthToPixel(parameter,circle->GetRadius());

        agg::ellipse ellipse(x+ConvertWidthToPixel(parameter,circle->GetCenter().x-centerX),
                             y+ConvertWidthToPixel(parameter,maxY-circle->GetCenter().y-centerY),
                             radius,
                             radius);

        path.concat_path(ellipse);

        rasterizer->add_path(path);

        DrawFill(projection,
                 parameter,
                 *style,
                 path);
      }
    }
  }

  void MapPainterAgg::DrawPath(const Projection& projection,
                               const MapParameter& parameter,
                               const Color& color,
                               double width,
                               const std::vector<double>& dash,
                               LineStyle::CapStyle startCap,
                               LineStyle::CapStyle endCap,
                               size_t transStart, size_t transEnd)
  {
    agg::path_storage p;

    for (size_t i=transStart; i<=transEnd; i++) {
      if (i==transStart) {
        p.move_to(transBuffer.buffer[i].x,transBuffer.buffer[i].y);
      }
      else {
        p.line_to(transBuffer.buffer[i].x,transBuffer.buffer[i].y);
      }
    }

    renderer_aa->color(agg::rgba(color.GetR(),
                                 color.GetG(),
                                 color.GetB(),
                                 color.GetA()));

    if (dash.empty()) {
      agg::conv_stroke<agg::path_storage> stroke(p);

      stroke.width(width);

      if (startCap==LineStyle::capButt ||
          endCap==LineStyle::capButt) {
        stroke.line_cap(agg::butt_cap);
      }
      else if (startCap==LineStyle::capSquare ||
               endCap==LineStyle::capSquare) {
        stroke.line_cap(agg::square_cap);
      }
      else {
        stroke.line_cap(agg::round_cap);
      }

      rasterizer->add_path(stroke);

      agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
    }
    else {
      agg::conv_dash<agg::path_storage>                    dasher(p);
      agg::conv_stroke<agg::conv_dash<agg::path_storage> > stroke(dasher);

      stroke.width(width);

      if (startCap==LineStyle::capButt ||
          endCap==LineStyle::capButt) {
        stroke.line_cap(agg::butt_cap);
      }
      else if (startCap==LineStyle::capSquare ||
               endCap==LineStyle::capSquare) {
        stroke.line_cap(agg::square_cap);
      }
      else {
        stroke.line_cap(agg::round_cap);
      }

      for (size_t i=0; i<dash.size(); i+=2) {
        dasher.add_dash(dash[i]*width,dash[i+1]*width);
      }

      rasterizer->add_path(stroke);

      agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
    }

    // TODO: End point caps "dots"
  }

  void MapPainterAgg::DrawArea(const Projection& projection,
                               const MapParameter& parameter,
                               const MapPainter::AreaData& area)
  {
    agg::path_storage path;

    if (!area.clippings.empty()) {
      rasterizer->filling_rule(agg::fill_even_odd);
    }
    else {
      rasterizer->filling_rule(agg::fill_non_zero);
    }

    path.move_to(transBuffer.buffer[area.transStart].x,transBuffer.buffer[area.transStart].y);
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      path.line_to(transBuffer.buffer[i].x,transBuffer.buffer[i].y);
    }
    path.close_polygon();

    rasterizer->add_path(path);

    if (!area.clippings.empty()) {
      for (std::list<PolyData>::const_iterator c=area.clippings.begin();
          c!=area.clippings.end();
          c++) {
        const PolyData    &data=*c;
        agg::path_storage clipPath;

         clipPath.move_to(transBuffer.buffer[data.transStart].x,transBuffer.buffer[data.transStart].y);
        for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
          clipPath.line_to(transBuffer.buffer[i].x,transBuffer.buffer[i].y);
        }
        clipPath.close_polygon();

        rasterizer->add_path(clipPath);
      }
    }

    DrawFill(projection,
             parameter,
             *area.fillStyle,
             path);
  }

  void MapPainterAgg::DrawArea(const FillStyle& style,
                              const MapParameter& parameter,
                              double x,
                              double y,
                              double width,
                              double height)
  {
    agg::path_storage path;

    path.move_to(x,y);
    path.line_to(x+width,y);
    path.line_to(x+width,y+height);
    path.line_to(x, y+height);
    path.close_polygon();

    renderer_aa->color(agg::rgba(style.GetFillColor().GetR(),
                                 style.GetFillColor().GetG(),
                                 style.GetFillColor().GetB(),
                                 1));

    rasterizer->filling_rule(agg::fill_non_zero);
    rasterizer->add_path(path);
    agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
  }

  bool MapPainterAgg::DrawMap(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             AggPixelFormat* pf)
  {
    this->pf=pf;

    renderer_base=new AggRenderBase(*pf);
    rasterizer=new AggScanlineRasterizer();
    scanlineP8=new AggScanline();
    renderer_aa=new AggScanlineRendererAA(*renderer_base);
    renderer_bin=new AggScanlineRendererBin(*renderer_base);
    fontEngine=new AggFontEngine();
    fontCacheManager=new AggFontManager(*fontEngine);

    convTextCurves=new AggTextCurveConverter(fontCacheManager->path_adaptor());
    convTextCurves->approximation_scale(2.0);

    convTextContours= new AggTextContourConverter(*convTextCurves);

    Draw(styleConfig,
         projection,
         parameter,
         data);

    delete convTextCurves;
    delete convTextContours;
    delete fontEngine;
    delete fontCacheManager;
    delete renderer_bin;
    delete renderer_aa;
    delete scanlineP8;
    delete rasterizer;
    delete renderer_base;

    return true;
  }
}

