#ifndef __MAP_PAINTER_CANVAS_H__
#define __MAP_PAINTER_CANVAS_H__

/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <osmscout/MapPainter.h>

namespace osmscout {

  class MapPainterCanvas : public osmscout::MapPainter
  {

  private:

    JNIEnv   *mJniEnv;
    jclass   mPainterClass;
    jobject  mPainterObject;

    double   mMinimumLineWidth; //! Minimum width a line must have to be visible

    std::vector<bool>    mIconLoaded;    // Vector for icon load status
    std::vector<bool>    mPatternLoaded; // Vector for pattern load status
    
    int GetColorInt(double r, double g, double b, double a);
    int GetColorInt(Color color);

  private:

    void DrawPrimitivePath(const Projection& projection,
                           const MapParameter& parameter,
                           const DrawPrimitiveRef& primitive,
                           double x, double y,
                           double minX,
                           double minY,
                           double maxX,
                           double maxY);

    void DrawPrimitivePath(const Projection& projection,
                           const MapParameter& parameter,
                           const DrawPrimitiveRef& primitive,
                           double x, double y,
                           double minX,
                           double minY,
                           double maxX,
                           double maxY,
                           double* onPathX, double* onPathY,
                           double* segmentLengths);

    void DrawFillStyle(const Projection& projection,
                       const MapParameter& parameter,
                       const FillStyle& fill);

    void SetPolygonFillPath(float* x, float* y, int numPoints);

    void MapPathOnPath(float* arrayX, float* arrayY,
                       int numPoints,
                       double* onPathX, double* onPathY,
                       double* segmentLengths);

  protected:

    bool HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style);

    bool HasPattern(const MapParameter& parameter,
                    const FillStyle& style);

    void GetTextDimension(const Projection& projection,
                          const MapParameter& parameter,
                          double objectWidth,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height);

    void DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label);

    void DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const PathTextStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd);

    void DrawSymbol(const Projection& projection,
                    const MapParameter& parameter,
                    const Symbol& symbol,
                    double x, double y);

    void DrawContourSymbol(const Projection& projection,
                           const MapParameter& parameter,
                           const Symbol& symbol,
                           double space,
                           size_t transStart, size_t transEnd);

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

    void DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area);

    void DrawArea(const FillStyle& style,
                  const MapParameter& parameter,
                  double x,
                  double y,
                  double width,
                  double height);

  public:
    MapPainterCanvas();
    virtual ~MapPainterCanvas();

    bool DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 JNIEnv *env,
                 jobject object);
  };
}

#endif	// __MAP_PAINTER_CANVAS_H__

