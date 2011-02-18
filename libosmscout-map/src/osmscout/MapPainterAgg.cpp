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

#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>

#include <agg2/agg_conv_bspline.h>
#include <agg2/agg_conv_dash.h>
#include <agg2/agg_conv_segmentator.h>
#include <agg2/agg_conv_stroke.h>
#include <agg2/agg_path_storage.h>
#include <agg2/agg_trans_single_path.h>

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
    fontEngine->width(size*8);  // We need to clearify this value, likely DPI depend?
    fontEngine->height(size*8); // We need to clearify this value, likely DPI depend?
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
    fontEngine->width(size*8);  // We need to clearify this value, likely DPI depend?
    fontEngine->height(size*8); // We need to clearify this value, likely DPI depend?
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

  bool MapPainterAgg::HasIcon(const StyleConfig& styleConfig,
                             const MapParameter& parameter,
                             IconStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    std::string filename=std::string("../libosmscout/data/icons/14x14/standard/")+
                         style.GetIconName()+".png";

    //TODO

    return false;
  }

  bool MapPainterAgg::HasPattern(const StyleConfig& styleConfig,
                                 const MapParameter& parameter,
                                 PatternStyle& style)
  {
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    std::string filename=std::string("../libosmscout/data/icons/14x14/standard/")+
                         style.GetPatternName()+".png";

    // TODO

    return false;
  }

  void MapPainterAgg::DrawLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const LabelStyle& style,
                               const std::string& text,
                               double x, double y)
  {
    if (style.GetStyle()==LabelStyle::normal) {
      double       fontSize=style.GetSize();
      double       r=style.GetTextR();
      double       g=style.GetTextG();
      double       b=style.GetTextB();
      double       a=style.GetTextA();
      std::wstring wideText(UTF8StringToWString(text));

      if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
        double factor=Log2(projection.GetMagnification())-Log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        a=a/factor;
      }

      SetFont(parameter,
              fontSize);

      double width;
      double height;

      GetTextDimension(wideText,width,height);

      x=x-width/2;
      y=y+-height/2+fontEngine->ascender();

      //renderer_bin->color(agg::rgba(r,g,b,a));
      renderer_aa->color(agg::rgba(r,g,b,a));

      DrawText(x,y,wideText);
    }
    else if (style.GetStyle()==LabelStyle::plate) {
      static const double outerWidth = 4;
      static const double innerWidth = 2;

      // TODO
    }
    else if (style.GetStyle()==LabelStyle::emphasize) {
      double       fontSize=style.GetSize();
      double       r=style.GetTextR();
      double       g=style.GetTextG();
      double       b=style.GetTextB();
      double       a=style.GetTextA();
      std::wstring wideText(UTF8StringToWString(text));

      if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
        double factor=Log2(projection.GetMagnification())-Log2(style.GetScaleAndFadeMag());

        fontSize=fontSize*pow(2,factor);
        if (factor>=1) {
          a=a/factor;
        }
      }

      SetOutlineFont(parameter,
                     fontSize);

      double width;
      double height;

      GetTextDimension(wideText,width,height);

      x=x-width/2;
      y=y+-height/2+fontEngine->ascender();

      //renderer_bin->color(agg::rgba(r,g,b,a));
      renderer_aa->color(agg::rgba(1,1,1,a));

      DrawOutlineText(x,y,wideText,2);

      SetFont(parameter,
              fontSize);

      //renderer_bin->color(agg::rgba(r,g,b,a));
      renderer_aa->color(agg::rgba(r,g,b,a));

      DrawText(x,y,wideText);

    }
  }

  void MapPainterAgg::DrawContourLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyle& style,
                                      const std::string& text,
                                      const std::vector<Point>& nodes)
  {
    double       fontSize=style.GetSize();
    double       r=style.GetTextR();
    double       g=style.GetTextG();
    double       b=style.GetTextB();
    double       a=style.GetTextA();
    std::wstring wideText(UTF8StringToWString(text));

    TransformWay(projection,parameter,nodes);

    SetOutlineFont(parameter,
                   fontSize);

    //renderer_bin->color(agg::rgba(r,g,b,a));
    renderer_aa->color(agg::rgba(r,g,b,a));

    agg::path_storage path;

    double length=0;
    double xs=0;
    double ys=0;
    double xo=0;
    double yo=0;

    if (nodes[0].lon<nodes[nodes.size()-1].lon) {
      bool start=true;

      for (size_t j=0; j<nodes.size(); j++) {
        if (drawNode[j]) {
          if (start) {
            path.move_to(nodeX[j],
                           nodeY[j]);
            xs=nodeX[j];
            ys=nodeY[j];
            start=false;
          }
          else {
            path.line_to(nodeX[j],
                           nodeY[j]);
            length+=sqrt(pow(nodeX[j]-xo,2)+
                         pow(nodeY[j]-yo,2));
          }

          xo=nodeX[j];
          yo=nodeY[j];
        }
      }
    }
    else {
      bool start=true;

      for (size_t j=0; j<nodes.size(); j++) {
        if (drawNode[nodes.size()-j-1]) {
          if (start) {
            path.move_to(nodeX[nodes.size()-j-1],
                           nodeY[nodes.size()-j-1]);
            xs=nodeX[j];
            ys=nodeY[j];
            start=false;
          }
          else {
            path.line_to(nodeX[nodes.size()-j-1],
                           nodeY[nodes.size()-j-1]);
            length+=sqrt(pow(nodeX[nodes.size()-j-1]-xo,2)+
                         pow(nodeY[nodes.size()-j-1]-yo,2));
          }

          xo=nodeX[nodes.size()-j-1];
          yo=nodeY[nodes.size()-j-1];
        }
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

  void MapPainterAgg::DrawIcon(const IconStyle* style,
                              double x, double y)
  {
    assert(style->GetId()>0);
    assert(style->GetId()!=std::numeric_limits<size_t>::max());
    //assert(style->GetId()<=images.size());
    //assert(!images[style->GetId()-1].isNull());

    // TODO
  }

  void MapPainterAgg::DrawSymbol(const SymbolStyle* style, double x, double y)
  {
    // TODO
  }

  void MapPainterAgg::DrawPath(const LineStyle::Style& style,
                              const Projection& projection,
                              const MapParameter& parameter,
                              double r,
                              double g,
                              double b,
                              double a,
                              double width,
                              CapStyle startCap,
                              CapStyle endCap,
                              const std::vector<Point>& nodes)
  {
    TransformWay(projection,parameter,nodes);

    agg::path_storage path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.move_to(nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          path.line_to(nodeX[i],nodeY[i]);
        }
        //nodesDrawnCount++;
      }

      //nodesAllCount++;
    }

    renderer_aa->color(agg::rgba(r,g,b,a));

    if (style==LineStyle::normal) {
      agg::conv_stroke<agg::path_storage> stroke(path);

      stroke.width(width);

      if (startCap==capRound &&
          endCap==capRound &&
          style==LineStyle::normal) {
        stroke.line_cap(agg::round_cap);
      }
      else {
        stroke.line_cap(agg::butt_cap);
      }

      rasterizer->add_path(stroke);

      agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
    }
    else {
      agg::conv_dash<agg::path_storage>                    dash(path);
      agg::conv_stroke<agg::conv_dash<agg::path_storage> > stroke(dash);

      stroke.width(width);

      if (startCap==capRound &&
          endCap==capRound &&
          style==LineStyle::normal) {
        stroke.line_cap(agg::round_cap);
      }
      else {
        stroke.line_cap(agg::butt_cap);
      }

      switch (style) {
      case LineStyle::none:
        break;
      case LineStyle::normal:
        dash.add_dash(1,0);
        break;
      case LineStyle::longDash:
        dash.add_dash(7,3);
        break;
      case LineStyle::dotted:
        dash.add_dash(1,2);
        break;
      case LineStyle::lineDot:
        dash.add_dash(7,3);
        dash.add_dash(1,3);
        break;
      }

      rasterizer->add_path(stroke);

      agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);
    }

    // TODO: End point caps "dots"
  }

  void MapPainterAgg::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              TypeId type,
                              const FillStyle& fillStyle,
                              const LineStyle* lineStyle,
                              const std::vector<Point>& nodes)
  {
    TransformArea(projection,parameter,nodes);

    agg::path_storage path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.move_to(nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          path.line_to(nodeX[i],nodeY[i]);
        }
        //nodesDrawnCount++;
      }

      //nodesAllCount++;
    }

    path.close_polygon();

    renderer_aa->color(agg::rgba(fillStyle.GetFillR(),
                                 fillStyle.GetFillG(),
                                 fillStyle.GetFillB(),
                                 1));

    rasterizer->add_path(path);

    agg::render_scanlines(*rasterizer,*scanlineP8,*renderer_aa);

    if (lineStyle!=NULL) {
      DrawPath(lineStyle->GetStyle(),
               projection,
               parameter,
               lineStyle->GetLineR(),
               lineStyle->GetLineG(),
               lineStyle->GetLineB(),
               lineStyle->GetLineA(),
               borderWidth[(size_t)type],
               capRound,
               capRound,
               nodes);
    }
  }

  void MapPainterAgg::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              TypeId type,
                              const PatternStyle& patternStyle,
                              const LineStyle* lineStyle,
                              const std::vector<Point>& nodes)
  {
    TransformArea(projection,parameter,nodes);

    // TODO
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
    path.line_to(x+width-1,y);
    path.line_to(x+width-1,y+height-1);
    path.line_to(x, y+height-1);
    path.close_polygon();

    renderer_aa->color(agg::rgba(style.GetFillR(),
                                 style.GetFillG(),
                                 style.GetFillB(),
                                 1));

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

