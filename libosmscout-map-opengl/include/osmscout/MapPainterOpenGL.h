#ifndef OSMSCOUT_MAP_MAPPAINTEROPENGL_H
#define OSMSCOUT_MAP_MAPPAINTEROPENGL_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2013  Tim Teulings
  Copyright (C) 2017  Fanny Monori

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

#define GLEW_STATIC

#include <GL/glew.h>

#include <osmscout/MapOpenGLFeatures.h>

#include <osmscout/OpenGLMapData.h>

#include <osmscout/private/MapOpenGLImportExport.h>

namespace osmscout {
  class OSMSCOUT_MAP_OPENGL_API MapPainterOpenGL {
  private:

    int width;
    int height;
    double dpi;

    int screenWidth;
    int screenHeight;

    float minLon;
    float minLat;
    float maxLon;
    float maxLat;

    float lookX;
    float lookY;

    OpenGLMapData AreaRenderer;
    OpenGLMapData GroundTileRenderer;
    OpenGLMapData GroundRenderer;
    OpenGLMapData PathRenderer;
    OpenGLMapData ImageRenderer;
    OpenGLMapData SymbolRenderer;
    OpenGLMapData LabelRenderer;

    osmscout::MapData MapData;
    osmscout::StyleConfigRef styleConfig;
    osmscout::MapParameter Parameter;
    osmscout::FillStyleRef landFill;
    osmscout::FillStyleRef seaFill;
    std::vector<osmscout::LineStyleRef> lineStyles;
    osmscout::GeoCoord Center;
    osmscout::Magnification Magnification;


    void ProcessAreaData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                         const osmscout::Projection &projection, const osmscout::StyleConfigRef &styleConfig);

    void ProcessGroundData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                           const osmscout::Projection &projection, const osmscout::StyleConfigRef &styleConfig);

    void ProcessPathData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                         const osmscout::Projection &projection,
                         const osmscout::StyleConfigRef &styleConfig);

    void ProcessNodeData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                          const osmscout::Projection &projection,
                          const osmscout::StyleConfigRef &styleConfig);

  public:
    MapPainterOpenGL(int width, int height, double dpi, int screenWidth, int screenHeight);

    ~MapPainterOpenGL();

    void LoadData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                  const osmscout::Projection &projection, const osmscout::StyleConfigRef &styleConfig);

    void SwapData();

    void DrawMap();

    void OnZoom(float zoomDirection);

    void OnTranslation(int startPointX, int startPointY, int endPointX, int endPointY);

    osmscout::GeoCoord GetCenter();

    bool PixelToGeo(double x, double y, double &lon, double &lat);

    bool IsVisibleArea(const Projection& projection, const GeoBox& boundingBox, double pixelOffset);

    //osmscout::GeoCoord PixelToGeo(double x, double y);

    //osmscout::Vertex2D GeoToPixel(osmscout::GeoCoord gc);

  };
}

#endif
