#ifndef OSMSCOUT_MAP_MAPPAINTERAGG_H
#define OSMSCOUT_MAP_MAPPAINTERAGG_H

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

#include <climits>
#include <cstdlib>

#include <agg2/agg_rendering_buffer.h>
#include <agg2/agg_pixfmt_rgb.h>
#include <agg2/agg_renderer_base.h>
#include <agg2/agg_rasterizer_scanline_aa.h>
#include <agg2/agg_scanline_p.h>
#include <agg2/agg_renderer_scanline.h>

#include <osmscout/Private/MapImportExport.h>

#include <osmscout/MapPainter.h>

namespace osmscout {

  class OSMSCOUT_MAP_API MapPainterAgg : public MapPainter
  {
  private:
    agg::pixfmt_rgb24                                                       *pf;
    agg::renderer_base<agg::pixfmt_rgb24>                                   *renderer_base;
    agg::rasterizer_scanline_aa<>                                           *rasterizer;
    agg::scanline_p8                                                        *scanlineP8;
    agg::renderer_scanline_aa_solid<agg::renderer_base<agg::pixfmt_rgb24> > *renderer_aa;


  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style);

    bool HasPattern(const StyleConfig& styleConfig,
                    const MapParameter& parameter,
                    PatternStyle& style);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelStyle& style,
                   const std::string& text,
                   double x, double y);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const LabelStyle& style,
                          const std::string& text,
                          const std::vector<Point>& nodes);

    void DrawIcon(const IconStyle* style,
                          double x, double y);

    void DrawSymbol(const SymbolStyle* style,
                            double x, double y);

    void DrawPath(const LineStyle::Style& style,
                  const Projection& projection,
                  const MapParameter& parameter,
                  double r,
                  double g,
                  double b,
                  double a,
                  double width,
                  CapStyle startCap,
                  CapStyle endCap,
                  const std::vector<Point>& nodes);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  TypeId type,
                  const FillStyle& fillStyle,
                  const LineStyle* lineStyle,
                  const std::vector<Point>& nodes);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  TypeId type,
                  const PatternStyle& patternStyle,
                  const LineStyle* lineStyle,
                  const std::vector<Point>& nodes);

    void DrawArea(const FillStyle& style,
                  const MapParameter& parameter,
                  double x,
                  double y,
                  double width,
                  double height);

  public:
    MapPainterAgg();
    ~MapPainterAgg();


    bool DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 agg::pixfmt_rgb24* pf);
  };
}

#endif
