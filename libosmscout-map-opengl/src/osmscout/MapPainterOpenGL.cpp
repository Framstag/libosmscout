/*
  This source is part of the libosmscout-map library
  Copyright (C) 2013  Tim Teulings

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

#include <osmscout/MapPainterOpenGL.h>

#include <iostream>

#include <GL/gl.h>

namespace osmscout {

  MapPainterOpenGL::MapPainterOpenGL()
  {
    // no code
  }

  MapPainterOpenGL::~MapPainterOpenGL()
  {
  }



  bool MapPainterOpenGL::HasIcon(const StyleConfig& styleConfig,
                                 const MapParameter& parameter,
                                 IconStyle& style)
  {
    // TODO
    return false;
  }

  bool MapPainterOpenGL::HasPattern(const MapParameter& parameter,
                                    const FillStyle& style)
  {
    // TODO
    return false;
  }

  void MapPainterOpenGL::GetTextDimension(const MapParameter& parameter,
                                          double fontSize,
                                          const std::string& text,
                                          double& xOff,
                                          double& yOff,
                                          double& width,
                                          double& height)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawContourSymbol(const Projection& projection,
                                           const MapParameter& parameter,
                                           const Symbol& symbol,
                                           double space,
                                           size_t transStart, size_t transEnd)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawLabel(const Projection& projection,
                                   const MapParameter& parameter,
                                   const LabelData& label)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawContourLabel(const Projection& projection,
                                          const MapParameter& parameter,
                                          const PathTextStyle& style,
                                          const std::string& text,
                                          size_t transStart, size_t transEnd)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawPrimitivePath(const Projection& projection,
                                           const MapParameter& parameter,
                                           const DrawPrimitiveRef& p,
                                           double x, double y,
                                           double minX,
                                           double minY,
                                           double maxX,
                                           double maxY)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawSymbol(const Projection& projection,
                                    const MapParameter& parameter,
                                    const Symbol& symbol,
                                    double x, double y)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawIcon(const IconStyle* style,
                                  double x, double y)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawPath(const Projection& projection,
                                  const MapParameter& parameter,
                                  const Color& color,
                                  double width,
                                  const std::vector<double>& dash,
                                  LineStyle::CapStyle startCap,
                                  LineStyle::CapStyle endCap,
                                  size_t transStart, size_t transEnd)
  {
    // TODO:
    // There is a limit in the OpenGL lineWidth, we need to
    // fallback to using quads instead of lines for this.

    glColor4d(color.GetR(),
              color.GetG(),
              color.GetB(),
              color.GetA());

    glLineWidth(width);

    glBegin(GL_LINE_STRIP);

    for (size_t i=transStart; i<=transEnd; i++) {
      glVertex3d(transBuffer.buffer[i].x,transBuffer.buffer[i].y,0.0);
    }

    glEnd();
  }

  void MapPainterOpenGL::DrawArea(const Projection& projection,
                                  const MapParameter& parameter,
                                  const MapPainter::AreaData& area)
  {
    if (area.fillStyle->GetFillColor().IsVisible()) {
      glColor4d(area.fillStyle->GetFillColor().GetR(),
                area.fillStyle->GetFillColor().GetG(),
                area.fillStyle->GetFillColor().GetB(),
                area.fillStyle->GetFillColor().GetA());

      glBegin(GL_POLYGON);

      for (size_t i=area.transStart; i<=area.transEnd; i++) {
        glVertex3d(transBuffer.buffer[i].x,transBuffer.buffer[i].y,0.0);
      }

      glEnd();
    }

    if (area.fillStyle->GetBorderWidth()>0 &&
        area.fillStyle->GetBorderColor().IsVisible()) {
      double borderWidth=ConvertWidthToPixel(parameter,
                                             area.fillStyle->GetBorderWidth());

      glColor4d(area.fillStyle->GetBorderColor().GetR(),
                area.fillStyle->GetBorderColor().GetG(),
                area.fillStyle->GetBorderColor().GetB(),
                area.fillStyle->GetBorderColor().GetA());

      glLineWidth(borderWidth);

      glBegin(GL_LINE_LOOP);

      for (size_t i=area.transStart; i<=area.transEnd; i++) {
        glVertex3d(transBuffer.buffer[i].x,transBuffer.buffer[i].y,0.0);
      }

      glEnd();
    }
  }

  void MapPainterOpenGL::DrawArea(const FillStyle& style,
                                  const MapParameter& parameter,
                                  double x,
                                  double y,
                                  double width,
                                  double height)
  {
    glColor4d(style.GetFillColor().GetR(),
              style.GetFillColor().GetG(),
              style.GetFillColor().GetB(),
              style.GetFillColor().GetA());

    glBegin(GL_QUADS);
    glVertex3d(x, y+height, 0.0);       // Top Left
    glVertex3d(x+width, y+height, 0.0); // Top Right
    glVertex3d(x+width,y, 0.0);         // Bottom Right
    glVertex3d(x,y, 0.0);               // Bottom Left
    glEnd();
  }

  bool MapPainterOpenGL::DrawMap(const StyleConfig& styleConfig,
                                const Projection& projection,
                                const MapParameter& parameter,
                                const MapData& data)
  {
    Draw(styleConfig,
         projection,
         parameter,
         data);

    return true;
  }
}

