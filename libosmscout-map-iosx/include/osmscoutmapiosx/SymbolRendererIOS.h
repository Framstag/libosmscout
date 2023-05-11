#ifndef OSMSCOUT_MAP_IOSX_SYMBOLRENDERERIOSX_H
#define OSMSCOUT_MAP_IOSX_SYMBOLRENDERERIOSX_H

/*
  This source is part of the libosmscout-map-iOSX library
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2023  Vladimir Vyskocil

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

#include <osmscoutmap/SymbolRenderer.h>

#include <osmscoutmap/Styles.h>

#ifdef __APPLE__
#include <TargetConditionals.h>
#endif

#if TARGET_OS_IPHONE
#import <UIKit/UIKit.h>
#else
#import <AppKit/AppKit.h>
#endif

namespace osmscout {

/**
 * \ingroup Renderer
 */
class SymbolRendererIOS: public SymbolRenderer {
private:
    
  CGContextRef cg;
  FillStyleRef fillStyle;
  BorderStyleRef borderStyle;
  double screenMmInPixel;
    
public:
  explicit SymbolRendererIOS(CGContextRef cg);
  SymbolRendererIOS(const SymbolRendererIOS&) = default;
  SymbolRendererIOS(SymbolRendererIOS&&) = default;

  ~SymbolRendererIOS() override = default;

  SymbolRendererIOS& operator=(const SymbolRendererIOS&) = default;
  SymbolRendererIOS& operator=(SymbolRendererIOS&&) = default;

protected:
  void BeginPrimitive() override;
  void SetFill(const FillStyleRef &fillStyle) override;
  void SetBorder(const BorderStyleRef &borderStyle, double screenMmInPixel) override;
  void DrawPolygon(const std::vector<Vertex2D> &polygonPixels) override;
  void DrawRect(double x, double y, double w, double h) override;
  void DrawCircle(double x, double y, double radius) override;
  void EndPrimitive() override;
};
}

#endif // OSMSCOUT_MAP_IOSX_SYMBOLRENDERERIOSX_H
