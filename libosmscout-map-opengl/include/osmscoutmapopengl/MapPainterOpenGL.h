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

#include <GL/glew.h>

#include <osmscoutmapopengl/MapOpenGLFeatures.h>
#include <osmscoutmapopengl/OpenGLMapData.h>
#include <osmscoutmapopengl/MapOpenGLImportExport.h>
#include <osmscoutmapopengl/TextLoader.h>
#include <osmscoutmapopengl/OpenGLProjection.h>

namespace osmscout {
  class OSMSCOUT_MAP_OPENGL_API MapPainterOpenGL
  {
  private:
    /**
     * Type of the line vertex.
     * It is necessary for joining triangles correctly.
     *
     *  1 ----------- 3 5 ----------- 3 5 ----------- 7
     *  |       _____/| |       _____/| |       _____/|
     *  |  ____/      | |  ____/      | |  ____/      |
     *  2 /__________ 4 6 /---------- 4 6 /---------- 8
     */
    enum PathVertexType: int {
      TStart = 1,
      BStart = 2,
      TR = 3,
      BR = 4,
      TL = 5,
      BL = 6,
      TEnd = 7,
      BEnd = 8
    };
  private:
    bool initialized = false;

    OpenGLProjection mapProjection;

    float lookX;
    float lookY;

    GLuint projectionShader=0;

  private:
    /**
     * Areas of the last ProcessAreas call, merged from the areas and the POI areas of the data and
     * sorted by the size of their bounding box, which is the order they are drawn in. The list is a
     * member so the step reuses its capacity across calls instead of allocating for every loaded
     * area; it is cleared at the start of every ProcessAreas call.
     *
     * Like the render buffers of this painter, the list assumes that one painter is driven by one
     * thread per frame.
     */
    std::vector<AreaRef> sortedAreas;

    /**
     * Number of rings the visibility decision of ProcessAreas was applied to, since the start of
     * that call. Diagnostic for tests.
     */
    size_t examinedRingCount=0;

    /**
     * Number of rings the last ProcessAreas call kept and whose geometry it went on to prepare. The
     * counter exists so a test can observe that the per-ring work follows the rings the view keeps;
     * it is reset at the start of every ProcessAreas call.
     */
    size_t keptRingCount=0;

    OpenGLMapData<GL_RGBA, 4> areaRenderer;
    OpenGLMapData<GL_RGBA, 4> groundTileRenderer;
    OpenGLMapData<GL_RGBA, 4> groundRenderer;
    OpenGLMapData<GL_RGBA, 4> wayRenderer;
    OpenGLMapData<GL_RGBA, 4> imageRenderer;
    OpenGLMapData<GL_RED, 1> textRenderer;

    TextLoader textLoader;

    osmscout::MapData mapData;
    osmscout::StyleConfigRef styleConfig;
    osmscout::StyleConfigRef dataStyleConfig;
    osmscout::MapParameter parameter;

    /**
     * Processes OSM area data, and converts to the format required by the OpenGL pipeline
     */
    void ProcessAreas(const osmscout::MapData &data,
                      const osmscout::Projection &loadProjection,
                      const osmscout::StyleConfigRef &styleConfig);

    /**
    * Processes OSM ground data, and converts to the format required by the OpenGL pipeline
    */
    void ProcessGround(const osmscout::MapData &data,
                       const osmscout::Projection &loadProjection,
                       const osmscout::StyleConfigRef &styleConfig);

    void ProcessWay(const osmscout::WayRef &way,
                    const osmscout::Projection &loadProjection,
                    const osmscout::StyleConfigRef &styleConfig,
                    const WidthFeatureValueReader &widthReader);

    /**
    * Processes OSM way data, and converts to the format required by the OpenGL pipeline
    */
    void ProcessWays(const osmscout::MapData &data,
                     const osmscout::Projection &loadProjection,
                     const osmscout::StyleConfigRef &styleConfig);

    void ProcessNode(const osmscout::NodeRef &node,
                     const osmscout::Projection &loadProjection,
                     const osmscout::StyleConfigRef &styleConfig,
                     std::vector<int> &icons);

    /**
    * Processes OSM node data, and converts to the format required by the OpenGL pipeline
    */
    void ProcessNodes(const osmscout::MapData &data,
                      const osmscout::Projection &loadProjection,
                      const osmscout::StyleConfigRef &styleConfig);

    /**
     * Swaps currently drawn area data and processed data
     */
    void SwapAreaData();

    /**
     * Swaps currently drawn ground data and processed data
     */
    void SwapGroundData();

    /**
     * Swaps currently drawn node data and processed data
     */
    void SwapNodeData();

    /**
     * Swaps currently drawn way data and processed data
     */
    void SwapWayData();

    void AddPathVertex(osmscout::Point current, osmscout::Point previous, osmscout::Point next,
                       osmscout::Color color, PathVertexType type, float width, glm::vec3 barycentric, int border = 0,
                       double z = 0, float dashsize = 0.0, float length = 1,
                       osmscout::Color gapcolor = osmscout::Color(1.0, 1.0, 1.0, 1.0));

  public:

    MapPainterOpenGL(int width, int height, double dpi,
                     const std::string &fontPath, const std::string &shaderDir,
                     const osmscout::MapParameter &parameter);

    ~MapPainterOpenGL();

    bool IsInitialized() const
    {
      return initialized;
    }

    /**
     * Number of rings the visibility decision of the last ProcessAreas call was applied to. The
     * counter exists so a test can observe that a ring the decision discards costs no per-ring
     * geometry work; it is reset at the start of every ProcessAreas call.
     */
    size_t GetExaminedRingCount() const
    {
      return examinedRingCount;
    }

    /**
     * Number of rings the last ProcessAreas call kept and prepared geometry for. The counter exists
     * so a test can observe that the per-ring work follows the rings the view keeps; it is reset at
     * the start of every ProcessAreas call.
     */
    size_t GetKeptRingCount() const
    {
      return keptRingCount;
    }

    /**
     * Zooms on the map.
     */
    void OnZoom(float zoomDirection);

    void SetSize(int width, int height);

    /**
     *  Translates the map to the given direction.
     */
    void OnTranslation(int startPointX, int startPointY, int endPointX, int endPointY);

    /**
     * Returns the visual center of the map.
     */
    osmscout::GeoCoord GetCenter() const;

    void SetCenter(const osmscout::GeoCoord &center);

    osmscout::Magnification GetMagnification() const;

    void SetMagnification(const osmscout::Magnification &magnification);

    MercatorProjection GetProjection() const;

    /**
     * Processes all OSM data, and converts to the format required by the OpenGL pipeline.
     */
    void ProcessData(const osmscout::MapData &data,
                     const osmscout::Projection &loadProjection,
                     const osmscout::StyleConfigRef &styleConfig);

    /**
     * Swaps currently drawn data and processed data.
     */
    void SwapData();

    /**
     * OpenGL draw call. Draws all feature of the map to the context.
     */
    void DrawMap(RenderSteps startStep=RenderSteps::FirstStep,
                 RenderSteps endStep=RenderSteps::LastStep);

  };
}

#endif
