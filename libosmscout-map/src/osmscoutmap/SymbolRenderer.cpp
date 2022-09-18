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

void SymbolRenderer::Render(const Symbol &symbol,
                            const Vertex2D &center,
                            double groundMeterInPixel,
                            double screenMmInPixel,
                            double scaleFactor) const
{
  if (symbol.GetProjectionMode()==Symbol::ProjectionMode::MAP) {
    scaleFactor*=screenMmInPixel;
  }
  else {
    scaleFactor*=groundMeterInPixel;
  }

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
      for (auto const &pixel: polygon->GetCoords()) {
        polygonPixels.emplace_back(center.GetX()+pixel.GetX()*scaleFactor,
                                   center.GetY()+pixel.GetY()*scaleFactor);
      }
      DrawPolygon(polygonPixels);
    }
    else if (const auto *rectangle = dynamic_cast<const RectanglePrimitive*>(primitivePtr);
      rectangle != nullptr) {

      FillStyleRef   fillStyle=rectangle->GetFillStyle();
      BorderStyleRef borderStyle=rectangle->GetBorderStyle();

      SetFill(fillStyle);
      SetBorder(borderStyle, screenMmInPixel);

      DrawRect(center.GetX()+rectangle->GetTopLeft().GetX()*scaleFactor,
               center.GetY()+rectangle->GetTopLeft().GetY()*scaleFactor,
               rectangle->GetWidth() * scaleFactor,
               rectangle->GetHeight() * scaleFactor);
    }
    else if (const auto *circle = dynamic_cast<const CirclePrimitive*>(primitivePtr);
      circle != nullptr) {

      FillStyleRef   fillStyle=circle->GetFillStyle();
      BorderStyleRef borderStyle=circle->GetBorderStyle();

      SetFill(fillStyle);
      SetBorder(borderStyle, screenMmInPixel);

      DrawCircle(center.GetX()+circle->GetCenter().GetX()*scaleFactor,
                 center.GetY()+circle->GetCenter().GetY()*scaleFactor,
                 circle->GetRadius() * scaleFactor);
    }
  }
}

void SymbolRenderer::Render(const Symbol &symbol,
                            const Vertex2D &center,
                            const Projection& projection,
                            double scaleFactor) const
{
  ScreenBox boundingBox = symbol.GetBoundingBox(projection);

  Vertex2D boxCenter = boundingBox.GetCenter();

  Render(symbol,
         Vertex2D(center.GetX()-boxCenter.GetY(),
                  center.GetY()-boxCenter.GetY()),
         projection.GetMeterInPixel(),
         projection.ConvertWidthToPixel(1),
         scaleFactor);
}

}
