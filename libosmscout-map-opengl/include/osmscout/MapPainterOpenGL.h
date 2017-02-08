#ifndef OSMSCOUT_MAP_MAPPAINTEROPENGL_H
#define OSMSCOUT_MAP_MAPPAINTEROPENGL_H

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

#include <osmscout/MapOpenGLFeatures.h>

#include <osmscout/private/MapOpenGLImportExport.h>

#include <osmscout/MapPainter.h>

#if defined(__APPLE__) && defined(__MACH__)
  #include <GLUT/glut.h>
#else
  #if defined(OSMSCOUT_MAP_OPENGL_HAVE_GL_GLUT_H)
    #include <GL/glut.h>
  #elif defined(OSMSCOUT_MAP_OPENGL_HAVE_GLUT_GLUT_H)
    #include <GLUT/glut.h>
  #else
    #error "no glut.h"
  #endif
#endif

namespace osmscout {

  class OSMSCOUT_MAP_OPENGL_API MapPainterOpenGL : public MapPainter
  {
  private:
    CoordBufferImpl<Vertex3D> *coordBuffer;
    GLUtesselator             *tesselator;

  protected:
    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style);

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    void GetFontHeight(const Projection& projection,
                       const MapParameter& parameter,
                       double fontSize,
                       double& height);

    void GetTextDimension(const Projection& projection,
                          const MapParameter& parameter,
                          double objectWidth,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height);

    void DrawGround(const Projection& projection,
                    const MapParameter& parameter,
                    const FillStyle& style);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label);

    void DrawPrimitivePath(const Projection& projection,
                           const MapParameter& parameter,
                           const DrawPrimitiveRef& primitive,
                           double x, double y,
                           double minX,
                           double minY,
                           double maxX,
                           double maxY);

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x, double y);

    void DrawIcon(const IconStyle* style,
                  double x, double y);

    void DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd);

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart, size_t transEnd);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd);

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area);

  public:
    MapPainterOpenGL(const StyleConfigRef& styleConfig);
    virtual ~MapPainterOpenGL();

    bool DrawMap(const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data);
  };
}

#endif
