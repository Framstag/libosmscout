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

#include <osmscout/Util.h>

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

  bool MapPainterAgg::HasIcon(const StyleConfig& styleConfig,
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
      double fontSize=style.GetSize();
      double r=style.GetTextR();
      double g=style.GetTextG();
      double b=style.GetTextB();
      double a=style.GetTextA();

      if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
        double factor=Log2(projection.GetMagnification())-Log2(style.GetScaleAndFadeMag());
        fontSize=fontSize*pow(2,factor);
        a=a/factor;
      }

      // TODO
    }
    else if (style.GetStyle()==LabelStyle::plate) {
      static const double outerWidth = 4;
      static const double innerWidth = 2;

      // TODO
    }
    else if (style.GetStyle()==LabelStyle::emphasize) {
      double fontSize=style.GetSize();
      double r=style.GetTextR();
      double g=style.GetTextG();
      double b=style.GetTextB();
      double a=style.GetTextA();

      if (projection.GetMagnification()>style.GetScaleAndFadeMag()) {
        double factor=Log2(projection.GetMagnification())-Log2(style.GetScaleAndFadeMag());

        fontSize=fontSize*pow(2,factor);
        if (factor>=1) {
          a=a/factor;
        }
      }

      // TODO
    }
  }

  void MapPainterAgg::DrawContourLabel(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyle& style,
                                      const std::string& text,
                                      const std::vector<Point>& nodes)
  {
    double fontSize=style.GetSize();
    double r=style.GetTextR();
    double g=style.GetTextG();
    double b=style.GetTextB();
    double a=style.GetTextA();

    // TODO
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
    // TODO
  }

  void MapPainterAgg::DrawArea(const Projection& projection,
                              const MapParameter& parameter,
                              TypeId type,
                              const FillStyle& fillStyle,
                              const LineStyle* lineStyle,
                              const std::vector<Point>& nodes)
  {
    agg::renderer_base<agg::pixfmt_rgb24>                                   renderer_base(*pf);
    agg::renderer_scanline_aa_solid<agg::renderer_base<agg::pixfmt_rgb24> > renderer_aa(renderer_base);

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

    renderer_aa.color(agg::rgba(fillStyle.GetFillR(),
                                fillStyle.GetFillG(),
                                fillStyle.GetFillB(),
                                1));

    rasterizer->add_path(path);

    if (lineStyle!=NULL) {
      // TODO: Draw outline
    }

    agg::render_scanlines(*rasterizer,*scanlineP8,renderer_aa);
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
    agg::renderer_base<agg::pixfmt_rgb24>                                   renderer_base(*pf);
    agg::renderer_scanline_aa_solid<agg::renderer_base<agg::pixfmt_rgb24> > renderer_aa(renderer_base);

    agg::path_storage path;

    path.move_to(x,y);
    path.line_to(x+width-1,y);
    path.line_to(x+width-1,y+height-1);
    path.line_to(x, y+height-1);
    path.close_polygon();

    renderer_aa.color(agg::rgba(style.GetFillR(),
                                style.GetFillG(),
                                style.GetFillB(),
                                1));

    rasterizer->add_path(path);
    agg::render_scanlines(*rasterizer,*scanlineP8,renderer_aa);
  }

  bool MapPainterAgg::DrawMap(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             agg::pixfmt_rgb24* pf)
  {
    this->pf=pf;

    renderer_base=new agg::renderer_base<agg::pixfmt_rgb24>(*pf);
    rasterizer=new agg::rasterizer_scanline_aa<>();
    scanlineP8=new agg::scanline_p8();
    renderer_aa=new agg::renderer_scanline_aa_solid<agg::pixfmt_rgb24>(*pf);

    Draw(styleConfig,
         projection,
         parameter,
         data);

    delete renderer_aa;
    delete scanlineP8;
    delete rasterizer;
    delete renderer_base;

    return true;
  }
}

