#ifndef OSMSCOUT_MAP_QT_SYMBOLRENDERERQT_H
#define OSMSCOUT_MAP_QT_SYMBOLRENDERERQT_H

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

#include <QPainter>

#include <osmscoutmapqt/MapQtImportExport.h>
#include <osmscoutmap/SymbolRenderer.h>

#include <osmscoutmap/Styles.h>

namespace osmscout {

/**
 * \ingroup Renderer
 */
class OSMSCOUT_MAP_QT_API SymbolRendererQt: public SymbolRenderer {
private:
  QPainter *painter; // not owning
public:
  explicit SymbolRendererQt(QPainter *painter);
  SymbolRendererQt(const SymbolRendererQt&) = default;
  SymbolRendererQt(SymbolRendererQt&&) = default;

  ~SymbolRendererQt() override = default;

  SymbolRendererQt& operator=(const SymbolRendererQt&) = default;
  SymbolRendererQt& operator=(SymbolRendererQt&&) = default;

protected:
  void SetFill(const FillStyleRef &fillStyle) override;
  void SetBorder(const BorderStyleRef &borderStyle, double screenMmInPixel) override;
  void DrawPolygon(const std::vector<Vertex2D> &polygonPixels) override;
  void DrawRect(double x, double y, double w, double h) override;
  void DrawCircle(double x, double y, double radius) override;
};
}

#endif // OSMSCOUT_MAP_QT_SYMBOLRENDERERQT_H
