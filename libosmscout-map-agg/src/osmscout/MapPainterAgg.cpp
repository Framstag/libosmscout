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

  MapPainterAgg::MapPainterAgg(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig,
               new CoordBufferImpl<Vertex2D>()),
    coordBuffer((CoordBufferImpl<Vertex2D>*)transBuffer.buffer)
  {
    // no code
  }

  MapPainterAgg::~MapPainterAgg()
  {
    // no code
    // TODO: Clean up fonts
  }

  void MapPainterAgg::SetFont(const Projection& projection,
                              const MapParameter& parameter,
                              double size)
  {
    if (!fontEngine->load_font(parameter.GetFontName().c_str(),
                               0,
                               agg::glyph_ren_native_gray8)) {
      std::cout << "Cannot load font '" << parameter.GetFontName() << "'" << std::endl;
      return;

    }

    //fontEngine->resolution(72);
    fontEngine->width(size*projection.ConvertWidthToPixel(parameter.GetFontSize()));
    fontEngine->height(size*projection.ConvertWidthToPixel(parameter.GetFontSize()));
    fontEngine->hinting(true);
    fontEngine->flip_y(true);
  }

  void MapPainterAgg::SetOutlineFont(const Projection& projection,
                                     const MapParameter& parameter,
                                     double size)
  {
    if (!fontEngine->load_font(parameter.GetFontName().c_str(),
                               0,
                               agg::glyph_ren_outline)) {
      std::cout << "Cannot load font '" << parameter.GetFontName() << "'" << std::endl;
      return;

    }

    //fontEngine->resolution(72);
    fontEngine->width(size*projection.ConvertWidthToPixel(parameter.GetFontSize()));
    fontEngine->height(size*projection.ConvertWidthToPixel(parameter.GetFontSize()));
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

  void MapPainterAgg::GetFontHeight(const Projection& projection,
                                    const MapParameter& parameter,
                                    double fontSize,
                                    double& height)
  {
    SetFont(projection,
            parameter,
            fontSize);

    height=fontEngine->height();
  }

  void MapPainterAgg::GetTextDimension(const Projection& projection,
                                       const MapParameter& parameter,
                                       double fontSize,
                                       const std::string& text,
                                       double& xOff,
                                       double& yOff,
                                       double& width,
                                       double& height)
  {
    std::wstring wideText(UTF8StringToWString(text));

    SetFont(projection,
            parameter,
            fontSize);

    xOff=0;
    yOff=0;
    width=0;
    height=fontEngine->height();

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

    double borderWidth=projection.ConvertWidthToPixel(fillStyle.GetBorderWidth());

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

  bool MapPainterAgg::HasIcon(const StyleConfig& /*styleConfig*/,
                             const MapParameter& /*parameter*/,
                             IconStyle& /*style*/)
  {
    //TODO

    return false;
  }

  void MapPainterAgg::DrawLabel(const Projection& projection,
                                const MapParameter& parameter,
                                const LabelData& label)
  {
    if (dynamic_cast<const TextStyle*>(label.style.get())!=NULL) {
      const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.get());
      double           r=style->GetTextColor().GetR();
      double           g=style->GetTextColor().GetG();
      double           b=style->GetTextColor().GetB();
      std::wstring     wideText(UTF8StringToWString(label.text));

      if (style->GetStyle()==TextStyle::normal) {

        SetFont(projection,
                parameter,
                label.fontSize);

        //renderer_bin->color(agg::rgba(r,g,b,a));
        renderer_aa->color(agg::rgba(r,g,b,label.alpha));

        DrawText(label.x,
                 label.y+fontEngine->ascender(),
                 wideText);
      }
      else if (style->GetStyle()==TextStyle::emphasize) {
        SetOutlineFont(projection,
                       parameter,
                       label.fontSize);

        //renderer_bin->color(agg::rgba(r,g,b,a));
        renderer_aa->color(agg::rgba(1,1,1,label.alpha));

        DrawOutlineText(label.x,
                        label.y+fontEngine->ascender(),
                        wideText,
                        2);

        SetFont(projection,
                parameter,
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

    SetOutlineFont(projection,
                   parameter,
                   fontSize);

    //renderer_bin->color(agg::rgba(r,g,b,a));
    renderer_aa->color(agg::rgba(r,g,b,a));

    agg::path_storage path;

    double pathLength=0;
    double xo=0;
    double yo=0;

    if (coordBuffer->buffer[transStart].GetX()<coordBuffer->buffer[transEnd].GetX()) {
      for (size_t j=transStart; j<=transEnd; j++) {
        if (j==transStart) {
          path.move_to(coordBuffer->buffer[j].GetX(),
                       coordBuffer->buffer[j].GetY());
        }
        else {
          path.line_to(coordBuffer->buffer[j].GetX(),
                       coordBuffer->buffer[j].GetY());
          pathLength+=sqrt(pow(coordBuffer->buffer[j].GetX()-xo,2)+
                       pow(coordBuffer->buffer[j].GetY()-yo,2));
        }

        xo=coordBuffer->buffer[j].GetX();
        yo=coordBuffer->buffer[j].GetY();
      }
    }
    else {
      for (size_t j=0; j<=transEnd-transStart; j++) {
        size_t idx=transEnd-j;

        if (j==0) {
          path.move_to(coordBuffer->buffer[idx].GetX(),
                       coordBuffer->buffer[idx].GetY());
        }
        else {
          path.line_to(coordBuffer->buffer[idx].GetX(),
                       coordBuffer->buffer[idx].GetY());
          pathLength+=sqrt(pow(coordBuffer->buffer[idx].GetX()-xo,2)+
                       pow(coordBuffer->buffer[idx].GetY()-yo,2));
        }

        xo=coordBuffer->buffer[idx].GetX();
        yo=coordBuffer->buffer[idx].GetY();
      }
    }

    double textWidth;
    double textHeight;

    GetTextDimension(wideText,textWidth,textHeight);

    double spaceLeft=pathLength-textWidth-2*contourLabelOffset;

    // If space around labels left is < offset on both sides, do not render at all
    if (spaceLeft<0.0) {
      return;
    }

    spaceLeft=fmod(spaceLeft,textWidth+contourLabelSpace);

    double       labelInstanceOffset=spaceLeft/2+contourLabelOffset;
    double       offset=labelInstanceOffset;

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

    double y=-textHeight/2+fontEngine->ascender();

    while (offset<pathLength) {
      for (size_t i=0; i<wideText.length(); i++) {
        const agg::glyph_cache* glyph=fontCacheManager->glyph(wideText[i]);

        if (glyph!=NULL) {
          fontCacheManager->add_kerning(&offset,&y);
          fontCacheManager->init_embedded_adaptors(glyph,offset,y);

          if (glyph->data_type==agg::glyph_data_outline) {
            rasterizer->reset();
            rasterizer->add_path(ftrans);
            renderer_aa->color(agg::rgba(r,g,b,a));
            agg::render_scanlines(*rasterizer,
                                  *scanlineP8,
                                  *renderer_aa);
          }

          // increment pen position
          offset+=glyph->advance_x;
          y+=glyph->advance_y;
        }
      }

      offset+=contourLabelSpace;
    }
  }

  void MapPainterAgg::DrawContourSymbol(const Projection& /*projection*/,
                                        const MapParameter& /*parameter*/,
                                        const Symbol& /*symbol*/,
                                        double /*space*/,
                                        size_t /*transStart*/, size_t /*transEnd*/)
  {
    // Not implemented
  }

  void MapPainterAgg::DrawIcon(const IconStyle* /*style*/,
                              double /*x*/, double /*y*/)
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
      DrawPrimitive* primitive=p->get();

      if (dynamic_cast<PolygonPrimitive*>(primitive)!=NULL) {
        PolygonPrimitive* polygon=dynamic_cast<PolygonPrimitive*>(primitive);
        FillStyleRef      style=polygon->GetFillStyle();
        agg::path_storage path;

        for (std::list<Vertex2D>::const_iterator pixel=polygon->GetCoords().begin();
             pixel!=polygon->GetCoords().end();
             ++pixel) {
          if (pixel==polygon->GetCoords().begin()) {
            path.move_to(x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                         y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
          }
          else {
            path.line_to(x+projection.ConvertWidthToPixel(pixel->GetX()-centerX),
                         y+projection.ConvertWidthToPixel(maxY-pixel->GetY()-centerY));
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
        double              xPos=x+projection.ConvertWidthToPixel(rectangle->GetTopLeft().GetX()-centerX);
        double              yPos=y+projection.ConvertWidthToPixel(maxY-rectangle->GetTopLeft().GetY()-centerY);
        double              width=projection.ConvertWidthToPixel(rectangle->GetWidth());
        double              height=projection.ConvertWidthToPixel(rectangle->GetHeight());

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
        double            radius=projection.ConvertWidthToPixel(circle->GetRadius());

        agg::ellipse ellipse(x+projection.ConvertWidthToPixel(circle->GetCenter().GetX()-centerX),
                             y+projection.ConvertWidthToPixel(maxY-circle->GetCenter().GetY()-centerY),
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

  void MapPainterAgg::DrawPath(const Projection& /*projection*/,
                               const MapParameter& /*parameter*/,
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
        p.move_to(coordBuffer->buffer[i].GetX(),
                  coordBuffer->buffer[i].GetY());
      }
      else {
        p.line_to(coordBuffer->buffer[i].GetX(),
                  coordBuffer->buffer[i].GetY());
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

    path.move_to(coordBuffer->buffer[area.transStart].GetX(),
                 coordBuffer->buffer[area.transStart].GetY());
    for (size_t i=area.transStart+1; i<=area.transEnd; i++) {
      path.line_to(coordBuffer->buffer[i].GetX(),
                   coordBuffer->buffer[i].GetY());
    }
    path.close_polygon();

    rasterizer->add_path(path);

    if (!area.clippings.empty()) {
      for (std::list<PolyData>::const_iterator c=area.clippings.begin();
          c!=area.clippings.end();
          c++) {
        const PolyData    &data=*c;
        agg::path_storage clipPath;

         clipPath.move_to(coordBuffer->buffer[data.transStart].GetX(),
                          coordBuffer->buffer[data.transStart].GetY());
        for (size_t i=data.transStart+1; i<=data.transEnd; i++) {
          clipPath.line_to(coordBuffer->buffer[i].GetX(),
                           coordBuffer->buffer[i].GetY());
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

  void MapPainterAgg::DrawGround(const Projection& projection,
                                 const MapParameter& /*parameter*/,
                                 const FillStyle& style)
  {
    agg::path_storage path;

    path.move_to(0,0);
    path.line_to(projection.GetWidth(),0);
    path.line_to(projection.GetWidth(),projection.GetHeight());
    path.line_to(0, projection.GetHeight());
    path.close_polygon();

    renderer_aa->color(agg::rgba(style.GetFillColor().GetR(),
                                 style.GetFillColor().GetG(),
                                 style.GetFillColor().GetB(),
                                 1));

    rasterizer->filling_rule(agg::fill_non_zero);
    rasterizer->add_path(path);
    agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
  }

  bool MapPainterAgg::DrawMap(const Projection& projection,
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

    Draw(projection,
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

