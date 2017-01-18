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

#if defined(__APPLE__) && defined(__MACH__)
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#ifndef CALLBACK
#define CALLBACK
#endif

namespace osmscout {

  static void tessalatorBeginCallback(GLenum which)
  {
    glBegin(which);
  }

  static void tessalatorEndCallback()
  {
    glEnd();
  }

  static void tesselatorErrorCallback(GLenum errorCode)
  {
     const GLubyte *estring;

     estring = gluErrorString(errorCode);
     std::cerr << "Tessellation Error: " << estring << std::endl;
  }

  MapPainterOpenGL::MapPainterOpenGL(const StyleConfigRef& styleConfig)
  : MapPainter(styleConfig,
               new CoordBufferImpl<Vertex3D>()),
    coordBuffer((CoordBufferImpl<Vertex3D>*)transBuffer.buffer),
    tesselator(gluNewTess())
  {
    gluTessNormal(tesselator,
                  0,0,1);

    gluTessCallback(tesselator,
                    GLU_TESS_BEGIN,
                    (GLvoid(CALLBACK *)()) &tessalatorBeginCallback);

    gluTessCallback(tesselator,
                    GLU_TESS_VERTEX,
                    (GLvoid(CALLBACK *)()) &glVertex3dv);

    gluTessCallback(tesselator,
                    GLU_TESS_END,
                    (GLvoid(CALLBACK *)()) &tessalatorEndCallback);

    gluTessCallback(tesselator,
                    GLU_TESS_ERROR,
                    (GLvoid(CALLBACK *)()) &tesselatorErrorCallback);

  }

  MapPainterOpenGL::~MapPainterOpenGL()
  {
    gluDeleteTess(tesselator);
  }



  bool MapPainterOpenGL::HasIcon(const StyleConfig& /*styleConfig*/,
                                 const MapParameter& /*parameter*/,
                                 IconStyle& /*style*/)
  {
    // TODO
    return false;
  }

  bool MapPainterOpenGL::HasPattern(const MapParameter& /*parameter*/,
                                    const FillStyle& /*style*/)
  {
    // TODO
    return false;
  }

  void MapPainterOpenGL::GetFontHeight(const Projection& /*projection*/,
                                       const MapParameter& /*parameter*/,
                                       double /*fontSize*/,
                                       double& /*height*/)
  {
    // TODO
  }

  void MapPainterOpenGL::GetTextDimension(const Projection& /*projection*/,
                                          const MapParameter& /*parameter*/,
                                          double /*objectWidth*/,
                                          double /*fontSize*/,
                                          const std::string& /*text*/,
                                          double& /*xOff*/,
                                          double& /*yOff*/,
                                          double& /*width*/,
                                          double& /*height*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawContourSymbol(const Projection& /*projection*/,
                                           const MapParameter& /*parameter*/,
                                           const Symbol& /*symbol*/,
                                           double /*space*/,
                                           size_t /*transStart*/, size_t /*transEnd*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawLabel(const Projection& /*projection*/,
                                   const MapParameter& /*parameter*/,
                                   const LabelData& /*label*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawContourLabel(const Projection& /*projection*/,
                                          const MapParameter& /*parameter*/,
                                          const PathTextStyle& /*style*/,
                                          const std::string& /*text*/,
                                          size_t /*transStart*/, size_t /*transEnd*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawPrimitivePath(const Projection& /*projection*/,
                                           const MapParameter& /*parameter*/,
                                           const DrawPrimitiveRef& /*p*/,
                                           double /*x*/, double /*y*/,
                                           double /*minX*/,
                                           double /*minY*/,
                                           double /*maxX*/,
                                           double /*maxY*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawSymbol(const Projection& /*projection*/,
                                    const MapParameter& /*parameter*/,
                                    const Symbol& /*symbol*/,
                                    double /*x*/, double /*y*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawIcon(const IconStyle* /*style*/,
                                  double /*x*/, double /*y*/)
  {
    // TODO
  }

  void MapPainterOpenGL::DrawPath(const Projection& /*projection*/,
                                  const MapParameter& /*parameter*/,
                                  const Color& color,
                                  double width,
                                  const std::vector<double>& /*dash*/,
                                  LineStyle::CapStyle /*startCap*/,
                                  LineStyle::CapStyle /*endCap*/,
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
      glVertex3d(coordBuffer->buffer[i].GetX(),
                 coordBuffer->buffer[i].GetY(),
                 0.0);
    }

    glEnd();
  }

  void MapPainterOpenGL::DrawArea(const Projection& projection,
                                  const MapParameter& /*parameter*/,
                                  const MapPainter::AreaData& area)
  {
    if (area.fillStyle->GetFillColor().IsVisible()) {
      glColor4d(area.fillStyle->GetFillColor().GetR(),
                area.fillStyle->GetFillColor().GetG(),
                area.fillStyle->GetFillColor().GetB(),
                area.fillStyle->GetFillColor().GetA());

      gluTessProperty(tesselator,
                      GLU_TESS_BOUNDARY_ONLY,
                      GL_FALSE);

      gluTessBeginPolygon(tesselator,
                          NULL);

      gluTessBeginContour(tesselator);

      for (size_t i=area.transStart; i<=area.transEnd; i++) {

        gluTessVertex(tesselator,
                      (GLdouble*)&coordBuffer->buffer[i],
                      (GLdouble*)&coordBuffer->buffer[i]);
      }

      gluTessEndContour(tesselator);

      if (!area.clippings.empty()) {
        // Clip areas within the area by using CAIRO_FILL_RULE_EVEN_ODD
        for (std::list<PolyData>::const_iterator c=area.clippings.begin();
            c!=area.clippings.end();
            c++) {
          const PolyData& data=*c;

          gluTessBeginContour(tesselator);

          for (size_t i=data.transStart; i<=data.transEnd; i++) {
            gluTessVertex(tesselator,
                          (GLdouble*)&coordBuffer->buffer[i],
                          (GLdouble*)&coordBuffer->buffer[i]);
          }

          gluTessEndContour(tesselator);
        }
      }

      gluTessEndPolygon(tesselator);
    }

    if (area.fillStyle->GetBorderWidth()>0 &&
        area.fillStyle->GetBorderColor().IsVisible()) {
      double borderWidth=projection.ConvertWidthToPixel(area.fillStyle->GetBorderWidth());

      glColor4d(area.fillStyle->GetBorderColor().GetR(),
                area.fillStyle->GetBorderColor().GetG(),
                area.fillStyle->GetBorderColor().GetB(),
                area.fillStyle->GetBorderColor().GetA());

      glLineWidth(borderWidth);

      glBegin(GL_LINE_LOOP);

      for (size_t i=area.transStart; i<=area.transEnd; i++) {
        glVertex3d(coordBuffer->buffer[i].GetX(),
                   coordBuffer->buffer[i].GetY(),
                   0.0);
      }

      glEnd();
    }
  }

  void MapPainterOpenGL::DrawGround(const Projection& projection,
                                    const MapParameter& /*parameter*/,
                                    const FillStyle& style)
  {
    glColor4d(style.GetFillColor().GetR(),
              style.GetFillColor().GetG(),
              style.GetFillColor().GetB(),
              style.GetFillColor().GetA());

    glBegin(GL_QUADS);
    glVertex3d(0,projection.GetHeight(),0.0);                     // Top Left
    glVertex3d(projection.GetWidth(),projection.GetHeight(),0.0); // Top Right
    glVertex3d(projection.GetWidth(),0.0,0.0);                    // Bottom Right
    glVertex3d(0,0,0.0);                                          // Bottom Left
    glEnd();
  }

  bool MapPainterOpenGL::DrawMap(const Projection& projection,
                                 const MapParameter& parameter,
                                 const MapData& data)
  {
    Draw(projection,
         parameter,
         data);

    return true;
  }
}
