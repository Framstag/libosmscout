/*
  This source is part of the libosmscout-map library
  Copyright (C) 2023  Tim Teulings

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

#include <osmscoutmapsvg/SymbolRendererSVG.h>

#include <sstream>

#include <osmscout/util/Color.h>

namespace osmscout {

SymbolRendererSVG::SymbolRendererSVG(std::ostream &stream):
  stream(stream),
  hasFill(false),
  hasStroke(false),
  strokeWidth(1.0),
  strokeAlpha(false)
{
}

void SymbolRendererSVG::WriteFillAndStroke()
{
  if (hasFill) {
    stream << " fill=\"" << fillColor << "\"";
  }
  else {
    stream << " fill=\"none\"";
  }

  if (hasStroke) {
    stream << " stroke=\"" << strokeColor << "\"";
    if (strokeAlpha) {
      // alpha handled via opacity attribute below
    }
    stream << " stroke-width=\"" << strokeWidth << "\"";
    if (!strokeDashArray.empty()) {
      stream << " stroke-dasharray=\"" << strokeDashArray << "\"";
    }
  }
  else {
    stream << " stroke=\"none\"";
  }
}

void SymbolRendererSVG::SetFill(const FillStyleRef &fillStyle)
{
  if (fillStyle && fillStyle->GetFillColor().IsVisible()) {
    hasFill = true;
    fillColor = fillStyle->GetFillColor().ToHexString();
  }
  else {
    hasFill = false;
  }
}

void SymbolRendererSVG::SetBorder(const BorderStyleRef &borderStyle,
                                 double screenMmInPixel)
{
  if (borderStyle) {
    hasStroke = true;
    strokeColor = borderStyle->GetColor().ToHexString();
    strokeWidth = borderStyle->GetWidth() * screenMmInPixel;
    strokeAlpha = !borderStyle->GetColor().IsSolid();

    strokeDashArray.clear();

    if (borderStyle->HasDashes()) {
      std::ostringstream pattern;

      for (size_t i=0;
           i<borderStyle->GetDash().size();
           ++i) {
        if (i>0) {
          pattern << " ";
        }

        pattern << borderStyle->GetDash()[i] * strokeWidth;
      }

      strokeDashArray=pattern.str();
    }
  }
  else {
    hasStroke = false;
  }
}

void SymbolRendererSVG::DrawPolygon(const std::vector<Vertex2D> &polygonPixels)
{
  stream << "    <polygon";
  WriteFillAndStroke();
  stream << std::endl;
  stream << "      points=\"";

  for (auto pixel = polygonPixels.begin();
       pixel != polygonPixels.end();
       ++pixel) {
    if (pixel != polygonPixels.begin()) {
      stream << " ";
    }
    stream << pixel->GetX() << "," << pixel->GetY();
  }

  stream << "\" />" << std::endl;
}

void SymbolRendererSVG::DrawRect(double x, double y, double w, double h)
{
  stream << "    <rect";
  stream << " x=\"" << x << "\"";
  stream << " y=\"" << y << "\"";
  stream << " width=\"" << w << "\"";
  stream << " height=\"" << h << "\"";
  WriteFillAndStroke();
  stream << " />" << std::endl;
}

void SymbolRendererSVG::DrawCircle(double x, double y, double radius)
{
  stream << "    <circle";
  stream << " cx=\"" << x << "\"";
  stream << " cy=\"" << y << "\"";
  stream << " r=\"" << radius << "\"";
  WriteFillAndStroke();
  stream << " />" << std::endl;
}

}
