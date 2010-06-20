/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/MapPainterQt.h>

#include <cassert>
#include <iostream>

namespace osmscout {

  MapPainterQt::MapPainterQt()
  {
    // no code
  }

  MapPainterQt::~MapPainterQt()
  {
    // no code
  }

  bool MapPainterQt::HasIcon(const StyleConfig& styleConfig,
                             IconStyle& style)
  {
    return false;
  }                           
  
  void MapPainterQt::ClearArea(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data)
  {
    painter->fillRect(0,
                      0,
                      projection.GetWidth(),
                      projection.GetHeight(),
                      QColor::fromRgbF(241.0/255,238.0/255,233.0/255,1.0));
  }                             
                                                    
  void MapPainterQt::DrawLabel(const Projection& projection,
                               const LabelStyle& style,
                               const std::string& text,
                               double x, double y)
  {
  }                             

  void MapPainterQt::DrawContourLabel(const Projection& projection,
                                      const LabelStyle& style,
                                      const std::string& text,
                                      const std::vector<Point>& nodes)
  {
  }                             
                         
  void MapPainterQt::DrawIcon(const IconStyle* style,
                              double x, double y)
  {
  }                             
                        
  void MapPainterQt::DrawSymbol(const SymbolStyle* style,
                                double x, double y)
  {
  }                             

  void MapPainterQt::DrawPath(LineStyle::Style style,
                              const Projection& projection,
                              double r,
                              double g,
                              double b,
                              double a,
                              double width,
                              const std::vector<Point>& nodes)
  {
    QPen pen;

    pen.setColor(QColor::fromRgbF(r,g,b,a));
    pen.setWidthF(width);
    
    switch (style) {
    case LineStyle::none:
      // way should not be visible in this case!
      assert(false);
      break;
    case LineStyle::normal:
      pen.setStyle(Qt::SolidLine);
      pen.setCapStyle(Qt::RoundCap);
      break;
    case LineStyle::longDash:
      pen.setStyle(Qt::DashLine);
      pen.setCapStyle(Qt::FlatCap);
      break;
    case LineStyle::dotted:
      pen.setStyle(Qt::DotLine);
      pen.setCapStyle(Qt::FlatCap);
      break;
    case LineStyle::lineDot:
      pen.setStyle(Qt::DashDotLine);
      pen.setCapStyle(Qt::FlatCap);
      break;
    }

    TransformWay(projection,nodes);

    QPainterPath path;

    bool start=true;
    for (size_t i=0; i<nodes.size(); i++) {
      if (drawNode[i]) {
        if (start) {
          path.moveTo(nodeX[i],nodeY[i]);
          start=false;
        }
        else {
          path.lineTo(nodeX[i],nodeY[i]);
        }
      }
    }

    painter->strokePath(path,pen);
  }
                        
  void MapPainterQt::DrawWay(const StyleConfig& styleConfig,
                             const Projection& projection,
                             TypeId type,
                             const SegmentAttributes& attributes,
                             const std::vector<Point>& nodes)
  {
  }                             
               
  void MapPainterQt::DrawWayOutline(const StyleConfig& styleConfig,
                                    const Projection& projection,
                                    TypeId type,
                                    const SegmentAttributes& attributes,
                                    const std::vector<Point>& nodes)
  {
  }                             

  void MapPainterQt::DrawArea(const StyleConfig& styleConfig,
                              const Projection& projection,
                              TypeId type,
                              int layer,
                              const SegmentAttributes& attributes,
                              const std::vector<Point>& nodes)
  {
  }                             

  bool MapPainterQt::DrawMap(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const MapData& data,
                             QPainter* painter)
  {
    this->painter=painter;
  
    Draw(styleConfig,
         projection,
         parameter,
         data);
  
    return true;
  }
}

