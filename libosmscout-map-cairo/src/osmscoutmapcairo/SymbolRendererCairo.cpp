/*
  This source is part of the libosmscout-map library
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2022  Lukas Karas
  Copyright (C) 2022  Tim Teulings

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

#include <osmscoutmapcairo/SymbolRendererCairo.h>

#include <osmscout/util/Logger.h>

namespace osmscout {
  SymbolRendererCairo::SymbolRendererCairo(cairo_t* draw)
    :
    draw(draw)
  {}

  void SymbolRendererCairo::BeginPrimitive()
  {
    fillStyle=nullptr;
    borderStyle=nullptr;
  }

  void SymbolRendererCairo::SetFill(const FillStyleRef& fillStyle)
  {
    this->fillStyle=fillStyle;
  }

  void SymbolRendererCairo::SetBorder(const BorderStyleRef& borderStyle,
                                      double screenMmInPixel)
  {
    this->borderStyle=borderStyle;
    this->screenMmInPixel=screenMmInPixel;
  }

  void SymbolRendererCairo::DrawPolygon(const std::vector<Vertex2D>& polygonPixels)
  {
    cairo_new_path(draw);

    for (auto pixel=polygonPixels.begin();
         pixel!=polygonPixels.end();
         ++pixel) {
      if (pixel==polygonPixels.begin()) {
        cairo_move_to(draw,
                      pixel->GetX(),
                      pixel->GetY());
      }
      else {
        cairo_line_to(draw,
                      pixel->GetX(),
                      pixel->GetY());
      }
    }

    cairo_close_path(draw);
  }

  void SymbolRendererCairo::DrawRect(double x,
                                     double y,
                                     double w,
                                     double h)
  {
    cairo_new_path(draw);

    cairo_rectangle(draw,
                    x,y,
                    w,h);

    cairo_close_path(draw);
  }

  void SymbolRendererCairo::DrawCircle(double x,
                                       double y,
                                       double radius)
  {
    cairo_new_path(draw);

    cairo_arc(draw,
              x,
              y,
              radius,
              0,2*M_PI);

    cairo_close_path(draw);
  }

  void SymbolRendererCairo::EndPrimitive()
  {
    bool   hasFill=false;
    bool   hasBorder=false;
    double borderWidth=borderStyle->GetWidth() * screenMmInPixel;

    if (fillStyle) {
      if (fillStyle->HasPattern()) {
        log.Warn() << "Pattern is not supported for symbols";
      }
      else if (fillStyle->GetFillColor().IsVisible()) {
        Color color = fillStyle->GetFillColor();
        cairo_set_source_rgba(draw,
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
    }

    if (hasFill && hasBorder) {
      cairo_fill_preserve(draw);
    } else if (hasFill) {
      cairo_fill(draw);
    }

    if (hasBorder) {
      Color borderColor=borderStyle->GetColor();
      cairo_set_source_rgba(draw,
                            borderColor.GetR(),
                            borderColor.GetG(),
                            borderColor.GetB(),
                            borderColor.GetA());

      cairo_set_line_width(draw, borderWidth);

      if (!borderStyle->HasDashes()) {
        cairo_set_dash(draw, nullptr, 0, 0);
      }
      else {
        std::array<double,10> dashArray;

        for (size_t i = 0; i < borderStyle->GetDash().size(); i++) {
          dashArray[i] = borderStyle->GetDash()[i] * borderWidth;
        }
        cairo_set_dash(draw, dashArray.data(), static_cast<int>(borderStyle->GetDash().size()), 0.0);
      }

      cairo_set_line_cap(draw, CAIRO_LINE_CAP_BUTT);

      cairo_stroke(draw);
    }
  }
}
