/*
  This source is part of the libosmscout-map library
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

#include <osmscoutmapiosx/SymbolRendererIOS.h>

#include <osmscout/log/Logger.h>

namespace osmscout {
  SymbolRendererIOS::SymbolRendererIOS(CGContextRef cg)
    : cg(cg) {
  }

  void SymbolRendererIOS::BeginPrimitive() {
    fillStyle=nullptr;
    borderStyle=nullptr;
  }

  void SymbolRendererIOS::SetFill(const FillStyleRef& fillStyle) {
    this->fillStyle=fillStyle;
  }

  void SymbolRendererIOS::SetBorder(const BorderStyleRef& borderStyle,
                                      double screenMmInPixel) {
    this->borderStyle=borderStyle;
    this->screenMmInPixel=screenMmInPixel;
  }

  void SymbolRendererIOS::DrawPolygon(const std::vector<Vertex2D>& polygonPixels) {
      CGContextBeginPath(cg);

      for (auto pixel=polygonPixels.begin();
           pixel!=polygonPixels.end();
           ++pixel) {
        if (pixel==polygonPixels.begin()) {
            CGContextMoveToPoint(cg,
                        pixel->GetX(),
                        pixel->GetY());
        } else {
            CGContextAddLineToPoint(cg,
                        pixel->GetX(),
                        pixel->GetY());
        }
      }
      
      CGContextClosePath(cg);
  }

  void SymbolRendererIOS::DrawRect(double x,
                                   double y,
                                   double w,
                                   double h) {
      CGRect rect = CGRectMake(x,y,w,h);
      CGContextAddRect(cg,rect);
  }

  void SymbolRendererIOS::DrawCircle(double x,
                                     double y,
                                     double radius) {
      CGRect rect = CGRectMake(x-radius/2, y-radius/2, radius, radius);
      CGContextAddEllipseInRect(cg, rect);
  }

  void SymbolRendererIOS::EndPrimitive() {
    bool   hasFill=false;
    bool   hasBorder=false;
    double borderWidth=borderStyle->GetWidth() * screenMmInPixel;

    if (fillStyle) {
        if (fillStyle->HasPattern()) {
          log.Warn() << "Pattern is not supported for symbols";
        } else if (fillStyle->GetFillColor().IsVisible()) {
          Color color = fillStyle->GetFillColor();
          CGContextSetRGBFillColor(cg,
                                 color.GetR(),
                                 color.GetG(),
                                 color.GetB(),
                                 color.GetA());
          hasFill = true;
        }
    }
      
    if (borderStyle) {
        hasBorder = borderWidth > 0 &&
                    borderStyle->GetColor().IsVisible();
        if (hasBorder) {
            CGContextSetLineWidth(cg, borderWidth);
            CGContextSetRGBStrokeColor(cg,borderStyle->GetColor().GetR(),
                                       borderStyle->GetColor().GetG(),
                                       borderStyle->GetColor().GetB(),
                                       borderStyle->GetColor().GetA());
            if (borderStyle->GetDash().empty()) {
                CGContextSetLineDash(cg, 0.0, NULL, 0);
                CGContextSetLineCap(cg, kCGLineCapRound);
            } else {
                CGFloat *dashes = (CGFloat *)malloc(sizeof(CGFloat)*borderStyle->GetDash().size());
                for (size_t i=0; i<borderStyle->GetDash().size(); i++) {
                    dashes[i] = borderStyle->GetDash()[i]*borderWidth;
                }
                CGContextSetLineDash(cg, 0.0, dashes, borderStyle->GetDash().size());
                free(dashes); dashes = NULL;
                CGContextSetLineCap(cg, kCGLineCapButt);
            }
        }
    }
      
    if (hasFill && hasBorder) {
        CGContextDrawPath(cg, kCGPathFillStroke);
    } else if (hasFill) {
        CGContextDrawPath(cg, kCGPathFill);
    } else {
        CGContextDrawPath(cg, kCGPathStroke);
    }
  }
}
