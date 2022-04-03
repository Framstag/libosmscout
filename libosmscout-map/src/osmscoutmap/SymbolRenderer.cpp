/*
  This source is part of the libosmscout-map library
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2022  Lukas Karas

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

#include <osmscout/util/Logger.h>
#include <osmscoutmap/SymbolRenderer.h>

#include <cassert>

namespace osmscout {

void SymbolRenderer::Render(const Symbol &symbol, const Vertex2D &zeroCoord,
                            double groundMeterInPixel, double screenMmInPixel) const
{
  for (const auto& primitive : symbol.GetPrimitives()) {
    const DrawPrimitive *primitivePtr=primitive.get();

    if (const auto *polygon = dynamic_cast<const PolygonPrimitive*>(primitivePtr);
      polygon != nullptr) {

      FillStyleRef   fillStyle=polygon->GetFillStyle();
      BorderStyleRef borderStyle=polygon->GetBorderStyle();

      SetFill(fillStyle);
      SetBorder(borderStyle, screenMmInPixel);

      std::vector<Vertex2D> polygonPixels;
      polygonPixels.reserve(polygon->GetCoords().size());
      if (polygon->GetProjectionMode()==DrawPrimitive::ProjectionMode::MAP) {
        for (auto const &pixel: polygon->GetCoords()) {
          polygonPixels.emplace_back(zeroCoord.GetX() + pixel.GetX() * screenMmInPixel,
                                     zeroCoord.GetY() + pixel.GetY() * screenMmInPixel);
        }
      } else {
        assert(polygon->GetProjectionMode()==DrawPrimitive::ProjectionMode::GROUND);
        for (auto const &pixel: polygon->GetCoords()) {
          polygonPixels.emplace_back(zeroCoord.GetX() + pixel.GetX() * groundMeterInPixel,
                                     zeroCoord.GetY() + pixel.GetY() * groundMeterInPixel);
        }
      }
      DrawPolygon(polygonPixels);
    }
    else if (const auto *rectangle = dynamic_cast<const RectanglePrimitive*>(primitivePtr);
      rectangle != nullptr) {

      FillStyleRef   fillStyle=rectangle->GetFillStyle();
      BorderStyleRef borderStyle=rectangle->GetBorderStyle();

      SetFill(fillStyle);
      SetBorder(borderStyle, screenMmInPixel);

      if (rectangle->GetProjectionMode()==DrawPrimitive::ProjectionMode::MAP) {
        DrawRect(zeroCoord.GetX() + rectangle->GetTopLeft().GetX() * screenMmInPixel,
                 zeroCoord.GetY() + rectangle->GetTopLeft().GetY() * screenMmInPixel,
                 rectangle->GetWidth() * screenMmInPixel,
                 rectangle->GetHeight() * screenMmInPixel);
      }
      else {
        DrawRect(zeroCoord.GetX() + rectangle->GetTopLeft().GetX() * groundMeterInPixel,
                 zeroCoord.GetY() + rectangle->GetTopLeft().GetY() * groundMeterInPixel,
                 rectangle->GetWidth() * groundMeterInPixel,
                 rectangle->GetHeight() * groundMeterInPixel);
      }
    }
    else if (const auto *circle = dynamic_cast<const CirclePrimitive*>(primitivePtr);
      circle != nullptr) {

      FillStyleRef   fillStyle=circle->GetFillStyle();
      BorderStyleRef borderStyle=circle->GetBorderStyle();

      SetFill(fillStyle);
      SetBorder(borderStyle, screenMmInPixel);

      if (circle->GetProjectionMode()==DrawPrimitive::ProjectionMode::MAP) {
        DrawCircle(zeroCoord.GetX() + circle->GetCenter().GetX() * screenMmInPixel,
                   zeroCoord.GetY() + circle->GetCenter().GetY() * screenMmInPixel,
                   circle->GetRadius() * screenMmInPixel);
      } else {
        DrawCircle(zeroCoord.GetX() + circle->GetCenter().GetX() * groundMeterInPixel,
                   zeroCoord.GetY() + circle->GetCenter().GetY() * groundMeterInPixel,
                   circle->GetRadius() * groundMeterInPixel);
      }
    }
  }
}

void SymbolRenderer::Render(const Symbol &symbol, const Vertex2D &center, const Projection& projection) const
{
  double minX;
  double minY;
  double maxX;
  double maxY;
  double centerX;
  double centerY;

  symbol.GetBoundingBox(projection,minX,minY,maxX,maxY);

  centerX=(minX+maxX)/2;
  centerY=(minY+maxY)/2;

  Render(symbol, Vertex2D(center.GetX()-centerX, center.GetY()-centerY), projection.GetMeterInPixel(), projection.ConvertWidthToPixel(1));
}

}
