#ifndef OSMSCOUT_MAP_SYMBOLRENDERER_H
#define OSMSCOUT_MAP_SYMBOLRENDERER_H

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

#include <osmscoutmap/MapImportExport.h>

#include <osmscoutmap/Styles.h>

namespace osmscout {

/**
 * \ingroup Renderer
 */
class OSMSCOUT_MAP_API SymbolRenderer
{
public:
  virtual ~SymbolRenderer() = default;

  /**
   * @param projection used projection for rendering
   * @param symbol
   * @param mapCenter screen coordinates where to render symbol center
   * @param afterRenderTransformer
   * @param afterEndTransformer
   * @param scaleFactor scale (on top of projection)
   */
  virtual void Render(const Projection &projection,
                      const Symbol &symbol,
                      const Vertex2D &mapCenter,
                      std::function<void()> afterRenderTransformer,
                      std::function<void()> afterEndTransformer,
                      double scaleFactor=1.0);

  /**
   * @param projection used projection for rendering
   * @param symbol
   * @param mapCenter screen coordinates where to render symbol center
   * @param scaleFactor scale (on top of projection)
   */
  virtual void Render(const Projection &projection,
                      const Symbol &symbol,
                      const Vertex2D &mapCenter,
                      double scaleFactor=1.0);

protected:
  virtual void SetFill(const FillStyleRef &fillStyle) = 0;

  virtual void SetBorder(const BorderStyleRef &borderStyle,
                         double screenMmInPixel) = 0;

  virtual void BeginPrimitive()
  {
    // Default implementation is empty
  };

  virtual void DrawPolygon(const std::vector<Vertex2D> &polygonPixels) = 0;

  virtual void DrawRect(double x,
                        double y,
                        double w,
                        double h) = 0;

  virtual void DrawCircle(double x,
                          double y,
                          double radius) = 0;

  virtual void EndPrimitive()
  {
    // Default implementation is empty
  };
};
}

#endif // OSMSCOUT_MAP_SYMBOLRENDERER_H
