/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2017 Lukáš Karas

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

#include <osmscout/MapOverlay.h>

MapOverlay::MapOverlay(QQuickItem* parent):
    QQuickPaintedItem(parent)
{
  view=new MapView(this);

  setRenderTarget(RenderTarget::FramebufferObject);
  setPerformanceHints(PerformanceHint::FastFBOResizing);
}

MapOverlay::~MapOverlay()
{
  delete view;
}

void MapOverlay::changeView(const MapView &updated)
{
  bool changed = *view != updated;
  if (changed){
    (*view) = updated;
    redraw();
  }
}

void MapOverlay::redraw()
{
  update();
}
