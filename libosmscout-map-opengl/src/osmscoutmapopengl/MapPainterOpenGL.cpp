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

#include <utility>
#include <iostream>

#include <osmscoutmap/MapPainter.h>

#include <osmscoutmapopengl/MapPainterOpenGL.h>
#include <osmscoutmapopengl/Triangulate.h>
#include <osmscoutmapopengl/PNGLoaderOpenGL.h>

#include <GL/glew.h>

namespace osmscout {

  MapPainterOpenGL::MapPainterOpenGL(int width, int height, double dpi, int screenWidth, int screenHeight,
                                     const std::string &fontPath, const std::string &shaderDir,
                                     long defaultTextSize)
      : width(width),
        height(height),
        dpi(dpi),
        screenWidth(screenWidth),
        screenHeight(screenHeight),
        textLoader(fontPath, defaultTextSize, dpi)
  {
    if (!textLoader.IsInitialized()) {
      log.Error() << "Failed to initialize text loader!";
      return;
    }

    glewExperimental = GL_TRUE;
    GLenum res = glewInit();
    if (res != GLEW_OK) {
      log.Error() << "Glew init error: " << glewGetErrorString(res);
      return;
    }

    if (!(areaRenderer.LoadVertexShader(shaderDir, "AreaVertexShader.vert") &&
          areaRenderer.LoadFragmentShader(shaderDir, "AreaFragmentShader.frag") &&
          areaRenderer.InitContext())) {
      log.Error() << "Could not initialize context for area rendering!";
      return;
    }

    if (!(groundTileRenderer.LoadVertexShader(shaderDir, "GroundVertexShader.vert") &&
          groundTileRenderer.LoadFragmentShader(shaderDir, "GroundFragmentShader.frag") &&
          groundTileRenderer.InitContext())) {
      log.Error() << "Could not initialize context for ground tile rendering!";
      return;
    }

    if (!(groundRenderer.LoadVertexShader(shaderDir, "GroundVertexShader.vert") &&
          groundRenderer.LoadFragmentShader(shaderDir, "GroundFragmentShader.frag") &&
          groundRenderer.InitContext())) {
      log.Error() << "Could not initialize context for ground rendering!";
      return;
    }

    if (!(wayRenderer.LoadVertexShader(shaderDir, "PathVertexShader.vert") &&
          wayRenderer.LoadFragmentShader(shaderDir, "PathFragmentShader.frag") &&
          wayRenderer.InitContext())) {
      log.Error() << "Could not initialize context for area rendering!";
      return;
    }

    if (!(imageRenderer.LoadVertexShader(shaderDir, "QuadVertexShader.vert") &&
          imageRenderer.LoadFragmentShader(shaderDir, "QuadFragmentShader.frag") &&
          imageRenderer.InitContext())) {
      log.Error() << "Could not initialize context for image rendering!";
      return;
    }

    if (!(textRenderer.LoadVertexShader(shaderDir, "TextVertexShader.vert") &&
          textRenderer.LoadFragmentShader(shaderDir, "TextFragmentShader.frag") &&
          textRenderer.InitContext())) {
      log.Error() << "Could not initialize context for text rendering!";
      return;
    }

    areaRenderer.clearData();
    areaRenderer.SetVerticesSize(6);
    groundTileRenderer.clearData();
    groundTileRenderer.SetVerticesSize(5);
    groundRenderer.clearData();
    groundRenderer.SetVerticesSize(5);
    wayRenderer.clearData();
    wayRenderer.SetVerticesSize(23);
    imageRenderer.clearData();
    imageRenderer.SetVerticesSize(5);
    imageRenderer.SetTextureHeight(7);
    textRenderer.clearData();
    textRenderer.SetVerticesSize(11);

    initialized = true;
  }

  void osmscout::MapPainterOpenGL::ProcessData(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                               const osmscout::Projection &projection,
                                               const osmscout::StyleConfigRef &styleConfig) {
    landFill=styleConfig.get()->GetLandFillStyle(projection);
    seaFill=styleConfig.get()->GetSeaFillStyle(projection);

    textLoader.SetDefaultFontSize(parameter.GetFontSize());

    this->magnification = projection.GetMagnification();
    this->center = projection.GetCenter();
    this->parameter = parameter;

    ProcessAreas(data, parameter, projection, styleConfig);

    ProcessGround(data, parameter, projection, styleConfig);

    ProcessWays(data, parameter, projection, styleConfig);

    ProcessNodes(data, parameter, projection, styleConfig);
  }

  void osmscout::MapPainterOpenGL::SwapData() {

    SwapAreaData();
    SwapGroundData();
    SwapWayData();
    SwapNodeData();
  }

  void osmscout::MapPainterOpenGL::SwapAreaData() {
    areaRenderer.SwapData();

    areaRenderer.BindBuffers();
    areaRenderer.LoadProgram();
    areaRenderer.LoadVertices();

    areaRenderer.SetProjection(width, height);
    areaRenderer.SetModel();
    areaRenderer.SetView(lookX, lookY);
    areaRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    areaRenderer.AddAttrib("color", 4, GL_FLOAT, 2 * sizeof(GLfloat));

    areaRenderer.AddUniform("windowWidth", width);
    areaRenderer.AddUniform("windowHeight", height);
    areaRenderer.AddUniform("centerLat", center.GetLat());
    areaRenderer.AddUniform("centerLon", center.GetLon());
    areaRenderer.AddUniform("magnification", magnification.GetMagnification());
    areaRenderer.AddUniform("dpi", dpi);
  }

  void osmscout::MapPainterOpenGL::SwapGroundData() {
    groundTileRenderer.SwapData();

    groundTileRenderer.BindBuffers();
    groundTileRenderer.LoadProgram();
    groundTileRenderer.LoadVertices();

    groundTileRenderer.SetProjection(width, height);
    groundTileRenderer.SetModel();
    groundTileRenderer.SetView(lookX, lookY);
    groundTileRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    groundTileRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    groundTileRenderer.AddUniform("windowWidth", width);
    groundTileRenderer.AddUniform("windowHeight", height);
    groundTileRenderer.AddUniform("centerLat", center.GetLat());
    groundTileRenderer.AddUniform("centerLon", center.GetLon());
    groundTileRenderer.AddUniform("magnification", magnification.GetMagnification());
    groundTileRenderer.AddUniform("dpi", dpi);

    groundRenderer.SwapData();

    groundRenderer.BindBuffers();

    groundRenderer.LoadProgram();
    groundRenderer.LoadVertices();

    groundRenderer.SetProjection(width, height);
    groundRenderer.SetModel();
    groundRenderer.SetView(lookX, lookY);
    groundRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    groundRenderer.AddAttrib("color", 3, GL_FLOAT, 2 * sizeof(GLfloat));

    groundRenderer.AddUniform("centerLat", center.GetLat());
    groundRenderer.AddUniform("centerLon", center.GetLon());
    groundRenderer.AddUniform("magnification", magnification.GetMagnification());
    groundRenderer.AddUniform("dpi", dpi);
  }

  void osmscout::MapPainterOpenGL::SwapNodeData() {
    imageRenderer.SwapData();

    imageRenderer.BindBuffers();
    imageRenderer.LoadProgram();
    imageRenderer.LoadVertices();
    imageRenderer.LoadTextures();

    imageRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    imageRenderer.AddAttrib("index", 1, GL_FLOAT, 2 * sizeof(GLfloat));
    imageRenderer.AddAttrib("textureStart", 1, GL_FLOAT, 3 * sizeof(GLfloat));
    imageRenderer.AddAttrib("textureWidth", 1, GL_FLOAT, 4 * sizeof(GLfloat));
    imageRenderer.AddUniform("windowWidth", width);
    imageRenderer.AddUniform("windowHeight", height);
    imageRenderer.AddUniform("centerLat", center.GetLat());
    imageRenderer.AddUniform("centerLon", center.GetLon());
    imageRenderer.AddUniform("quadWidth", 14);
    imageRenderer.AddUniform("magnification", magnification.GetMagnification());
    imageRenderer.AddUniform("textureWidthSum", imageRenderer.GetTextureWidth());
    imageRenderer.AddUniform("dpi", dpi);
    imageRenderer.AddUniform("z", 0.001);

    imageRenderer.SetProjection(width, height);
    imageRenderer.SetModel();
    imageRenderer.SetView(lookX, lookY);

    textRenderer.SetTextureHeight(textLoader.GetHeight());
    textRenderer.SwapData();

    textRenderer.BindBuffers();
    textRenderer.LoadProgram();
    textRenderer.LoadVertices();
    textRenderer.LoadTextures();

    textRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    textRenderer.AddAttrib("color", 4, GL_FLOAT, 2 * sizeof(GLfloat));
    textRenderer.AddAttrib("index", 1, GL_FLOAT, 6 * sizeof(GLfloat));
    textRenderer.AddAttrib("textureStart", 1, GL_FLOAT, 7 * sizeof(GLfloat));
    textRenderer.AddAttrib("textureWidth", 1, GL_FLOAT, 8 * sizeof(GLfloat));
    textRenderer.AddAttrib("positionOffset", 1, GL_FLOAT, 9 * sizeof(GLfloat));
    textRenderer.AddAttrib("startOffset", 1, GL_FLOAT, 10 * sizeof(GLfloat));
    textRenderer.AddUniform("windowWidth", width);
    textRenderer.AddUniform("windowHeight", height);
    textRenderer.AddUniform("centerLat", center.GetLat());
    textRenderer.AddUniform("centerLon", center.GetLon());
    textRenderer.AddUniform("textureHeight", textLoader.GetHeight());
    textRenderer.AddUniform("magnification", magnification.GetMagnification());
    textRenderer.AddUniform("textureWidthSum", imageRenderer.GetTextureWidth());
    textRenderer.AddUniform("dpi", dpi);
    textRenderer.AddUniform("z", 0.001);

    textRenderer.SetProjection(width, height);
    textRenderer.SetModel();
    textRenderer.SetView(lookX, lookY);
  }

  void osmscout::MapPainterOpenGL::SwapWayData() {
    wayRenderer.SwapData();

    wayRenderer.BindBuffers();
    wayRenderer.LoadProgram();
    wayRenderer.LoadVertices();

    wayRenderer.SetProjection(width, height);
    wayRenderer.SetModel();
    wayRenderer.SetView(lookX, lookY);
    wayRenderer.AddAttrib("position", 2, GL_FLOAT, 0);
    wayRenderer.AddAttrib("previous", 2, GL_FLOAT, 2 * sizeof(GLfloat));
    wayRenderer.AddAttrib("next", 2, GL_FLOAT, 4 * sizeof(GLfloat));
    wayRenderer.AddAttrib("color", 4, GL_FLOAT, 6 * sizeof(GLfloat));
    wayRenderer.AddAttrib("gapcolor", 4, GL_FLOAT, 10 * sizeof(GLfloat));
    wayRenderer.AddAttrib("index", 1, GL_FLOAT, 14 * sizeof(GLfloat));
    wayRenderer.AddAttrib("thickness", 1, GL_FLOAT, 15 * sizeof(GLfloat));
    wayRenderer.AddAttrib("border", 1, GL_FLOAT, 16 * sizeof(GLfloat));
    wayRenderer.AddAttrib("barycentric", 3, GL_FLOAT, 17 * sizeof(GLfloat));
    wayRenderer.AddAttrib("z", 1, GL_FLOAT, 20 * sizeof(GLfloat));
    wayRenderer.AddAttrib("dashsize", 1, GL_FLOAT, 21 * sizeof(GLfloat));
    wayRenderer.AddAttrib("length", 1, GL_FLOAT, 22 * sizeof(GLfloat));
    wayRenderer.AddUniform("windowWidth", width);
    wayRenderer.AddUniform("windowHeight", height);
    wayRenderer.AddUniform("centerLat", center.GetLat());
    wayRenderer.AddUniform("centerLon", center.GetLon());
    wayRenderer.AddUniform("magnification", magnification.GetMagnification());
    wayRenderer.AddUniform("dpi", dpi);
  }

  void
  osmscout::MapPainterOpenGL::ProcessAreas(const osmscout::MapData &data,
                                           const osmscout::MapParameter &/*parameter*/,
                                           const osmscout::Projection &projection,
                                           const osmscout::StyleConfigRef &styleConfig) {

    //osmscout::log.Info() << "Area: " << data.areas.size();

    std::vector<AreaRef> areas = data.areas;

    std::sort(areas.begin(), areas.end(),
              [](const AreaRef &a, const AreaRef &b) -> bool {
                GeoBox b1=a->GetBoundingBox();
                GeoBox b2=b->GetBoundingBox();
                return b1.GetHeight() * b1.GetWidth() > b2.GetHeight() * b2.GetWidth();
              });

    for (const auto &area : areas) {
      size_t ringId = Area::outerRingId;
      bool foundRing = true;

      while (foundRing) {
        foundRing = false;

        for (size_t i = 0; i < area->rings.size(); i++) {
          const Area::Ring &ring = area->rings[i];

          if (ring.IsMaster()) {
            continue;
          }

          if (ring.GetRing() != ringId) {
            continue;
          }

          if (!ring.IsTopOuter() &&
              ring.GetType()->GetIgnore()) {
            continue;
          }

          TypeInfoRef type;
          FillStyleRef fillStyle;
          std::vector<BorderStyleRef> borderStyles;

          if (ring.IsTopOuter()) {
            type = area->GetType();
          } else {
            type = ring.GetType();
          }

          fillStyle=styleConfig->GetAreaFillStyle(type,
                                                  ring.GetFeatureValueBuffer(),
                                                  projection);

          styleConfig->GetAreaBorderStyles(type,
                                           ring.GetFeatureValueBuffer(),
                                           projection,
                                           borderStyles);

          if (!fillStyle && borderStyles.empty()) {
            continue;
          }

          foundRing = true;

          std::vector<Point> p = area->rings[i].nodes;
          std::vector<osmscout::Area::Ring> r;

          for (int i = p.size() - 1; i >= 0; i--) {
            for (int j = 0; j < i; j++) {
              if (fabs(p[i].GetLat() - p[j].GetLat()) < 0.000000001 &&
                  fabs(p[i].GetLon() - p[j].GetLon()) < 0.0000000001) {
                p.erase(p.begin() + i);
              }
            }
          }

          if (p.size() < 3) {
            continue;
          }

          osmscout::GeoBox ringBoundingBox;
          ring.GetBoundingBox(ringBoundingBox);

          size_t j = i + 1;
          int hasClippings = 0;
          while (j < area->rings.size() &&
                 area->rings[j].GetRing() == ringId + 1 &&
                 area->rings[j].GetType()->GetIgnore()) {
            r.push_back(area->rings[j]);
            j++;
            hasClippings = 1;
          }

          std::vector<GLfloat> points;

          if (!fillStyle) {
            continue;
          }

          Color c = fillStyle->GetFillColor();

          BorderStyleRef borderStyle;
          size_t borderStyleIndex = 0;

          if (!borderStyles.empty() &&
              borderStyles.front()->GetDisplayOffset() == 0.0 &&
              borderStyles.front()->GetOffset() == 0.0) {
            borderStyle = borderStyles[borderStyleIndex];
            borderStyleIndex++;
          }

          double borderWidth = borderStyle ? borderStyle->GetWidth() : 0.0;

          if (!IsVisibleArea(projection,
                             ringBoundingBox,
                             borderWidth / 2.0)) {
            continue;
          }

          try {
            if (hasClippings == 1) {
              for (auto &ring: r) {
                for (int i = ring.nodes.size() - 1; i >= 0; i--) {
                  for (int j = 0; j < i; j++) {
                    if (fabs(ring.nodes[i].GetLat() - ring.nodes[j].GetLat()) < 0.000000001 &&
                        fabs(ring.nodes[i].GetLon() - ring.nodes[j].GetLon()) < 0.0000000001) {
                      ring.nodes.erase(ring.nodes.begin() + i);
                    }
                  }
                }
              }

              std::vector<std::vector<osmscout::Point>> polygons;
              polygons.push_back(p);
              for (const auto &ring: r) {
                if (ring.nodes.size() >= 3) {
                  polygons.push_back(ring.nodes);
                }
              }
              points = osmscout::Triangulate::TriangulateWithHoles(polygons);
            } else {
              points = osmscout::Triangulate::TriangulatePolygon(p);
            }
          } catch (const std::runtime_error &e) {
            log.Warn() << "Skip area " << area->GetFileOffset() << ", triangulation failed: " << e.what();
            continue;
          }

          for (size_t t = 0; t < points.size(); t++) {
            if (t % 2 == 0) {
              areaRenderer.AddNewVertex(points[t]);
            } else {
              areaRenderer.AddNewVertex(points[t]);
              areaRenderer.AddNewVertex(c.GetR());
              areaRenderer.AddNewVertex(c.GetG());
              areaRenderer.AddNewVertex(c.GetB());
              areaRenderer.AddNewVertex(c.GetA());

              if (areaRenderer.GetNumOfVertices() <= 6) {
                areaRenderer.AddNewElement(0);
              } else {
                areaRenderer.AddNewElement(areaRenderer.GetVerticesNumber() - 1);
              }
            }
          }

          p.push_back(p[0]);
          for (size_t idx = 0;
               idx < borderStyles.size();
               idx++) {
            borderStyle = borderStyles[idx];

            for (size_t t = 0; t < p.size() - 1; t++) {

              Color color = borderStyle->GetColor();
              //first triangle
              AddPathVertex(p[t],
                            t == 0 ? p[t] : p[t - 1],
                            p[t + 1],
                            color, t == 0 ? TStart : TL, borderWidth,
                            glm::vec3(1, 0, 0));
              AddPathVertex(p[t],
                            t == 0 ? p[t] : p[t - 1],
                            p[t + 1],
                            color, t == 0 ? BStart : BL, borderWidth,
                            glm::vec3(0, 1, 0));
              AddPathVertex(p[t + 1],
                            p[t],
                            (t == p.size() - 2 ? p[t + 1] : p[t + 2]),
                            color, (t == p.size() - 2 ? TEnd : TR), borderWidth,
                            glm::vec3(0, 0, 1));
              //second triangle
              AddPathVertex(p[t + 1],
                            p[t],
                            (t == p.size() - 2 ? p[t + 1] : p[t + 2]),
                            color, (t == p.size() - 2) ? TEnd : TR, borderWidth,
                            glm::vec3(1, 0, 0));
              AddPathVertex(p[t],
                            t == 0 ? p[t] : p[t - 1],
                            p[t + 1],
                            color, t == 0 ? BStart : BL, borderWidth,
                            glm::vec3(0, 1, 0));
              AddPathVertex(p[t + 1],
                            p[t],
                            (t == p.size() - 2 ? p[t + 1] : p[t + 2]),
                            color, t == p.size() - 2 ? BEnd : BR, borderWidth,
                            glm::vec3(0, 0, 1));

              int num;
              num = wayRenderer.GetVerticesNumber() - 6;
              wayRenderer.AddNewElement(num);
              wayRenderer.AddNewElement(num + 1);
              wayRenderer.AddNewElement(num + 2);
              wayRenderer.AddNewElement(num + 3);
              wayRenderer.AddNewElement(num + 4);
              wayRenderer.AddNewElement(num + 5);
            }
          }

        }
        ringId++;
      }
    }
  }

  bool osmscout::MapPainterOpenGL::IsVisibleArea(const Projection &projection, const GeoBox &boundingBox,
                                                 double pixelOffset) {
    double x1;
    double x2;
    double y1;
    double y2;

    projection.GeoToPixel(boundingBox.GetMinCoord(),
                          x1,
                          y1);

    projection.GeoToPixel(boundingBox.GetMaxCoord(),
                          x2,
                          y2);

    double xMin = std::min(x1, x2) - pixelOffset;
    double xMax = std::max(x1, x2) + pixelOffset;
    double yMin = std::min(y1, y2) - pixelOffset;
    double yMax = std::max(y1, y2) + pixelOffset;

    osmscout::GeoBox gb;
    projection.GetDimensions(gb);
    double areaMinDimension = projection.ConvertWidthToPixel(parameter.GetAreaMinDimensionMM());

    if (xMax - xMin <= areaMinDimension &&
        yMax - yMin <= areaMinDimension) {
      return false;
    }

    return !(xMin >= projection.GetWidth() ||
             yMin >= projection.GetHeight() ||
             xMax < 0 ||
             yMax < 0);
  }

  void
  osmscout::MapPainterOpenGL::ProcessWays(const osmscout::MapData &data,
                                          const osmscout::MapParameter &/*parameter*/,
                                          const osmscout::Projection &projection,
                                          const osmscout::StyleConfigRef &styleConfig) {

    WidthFeatureValueReader widthReader(*styleConfig->GetTypeConfig());
    LayerFeatureValueReader layerReader(*styleConfig->GetTypeConfig());

    //osmscout::log.Info() << "Ways: " << data.ways.size();

    for (const auto &way: data.ways) {

      std::vector<LineStyleRef> lineStyles;

      styleConfig->GetWayLineStyles(way->GetFeatureValueBuffer(),
                                    projection,
                                    lineStyles);

      if (lineStyles.empty()) {
        continue;
      }

      FeatureValueBuffer buffer(way->GetFeatureValueBuffer());

      for (int l = lineStyles.size() - 1; l >= 0; l--) {
        Color color = lineStyles[l]->GetLineColor();

        int border = l;
        double lineWidth = 0.0;
        double lineOffset = 0.0;
        double z;
        if (l == 0)
          z = 0.001;
        else
          z = 0.0;

        if (lineStyles[l]->GetWidth() > 0.0) {
          WidthFeatureValue *widthValue = widthReader.GetValue(buffer);

          if (widthValue != nullptr) {
            lineWidth += widthValue->GetWidth() / projection.GetPixelSize();
          } else {
            lineWidth += lineStyles[l]->GetWidth() / projection.GetPixelSize();
          }
        }

        if (lineStyles[l]->GetDisplayWidth() > 0.0) {
          lineWidth += projection.ConvertWidthToPixel(lineStyles[l]->GetDisplayWidth());
        }

        if (lineWidth == 0.0) {
          return;
        }

        if (lineStyles[l]->GetOffset() != 0.0) {
          lineOffset += lineStyles[l]->GetOffset() / projection.GetPixelSize();
        }

        if (lineStyles[l]->GetDisplayOffset() != 0.0) {
          lineOffset += projection.ConvertWidthToPixel(lineStyles[l]->GetDisplayOffset());
        }

        osmscout::Color gapColor;
        if (!lineStyles[l]->GetDash().empty())
          gapColor = lineStyles[l]->GetGapColor();
        else
          gapColor = lineStyles[l]->GetLineColor();

        for (size_t i = 0; i < way->nodes.size() - 1; i++) {
          double length = 1;
          double dashSize = 0;
          if (!lineStyles[l]->GetDash().empty() && (l == 0)) {
            for (size_t d = 0; d < lineStyles[l]->GetDash().size(); d++) {
              if (lineStyles[l]->GetDash()[d] != 0) {
                dashSize = lineStyles[l]->GetDash()[d];
                break;
              }
            }
            double distance = sqrt(osmscout::DistanceSquare(way->nodes[i], way->nodes[i + 1]));
            double degreeToMeter = std::abs(0.00001 * std::cos(way->nodes[i].GetLat()));
            double distanceMeter = distance / degreeToMeter;
            double result = projection.GetMeterInPixel() * distanceMeter;
            length = result;
          }
          //first triangle
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? TStart : TL, lineWidth,
                        glm::vec3(1, 0, 1),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? BStart : BL, lineWidth,
                        glm::vec3(0, 1, 1),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        (i == way->nodes.size() - 2 ? way->nodes[i + 1] : way->nodes[i + 2]),
                        color, (i == way->nodes.size() - 2 ? TEnd : TR), lineWidth,
                        glm::vec3(0, 0, 1),
                        border, z, dashSize, length, gapColor);
          //second triangle
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        (i == way->nodes.size() - 2 ? way->nodes[i + 1] : way->nodes[i + 2]),
                        color, (i == way->nodes.size() - 2) ? TEnd : TR, lineWidth,
                        glm::vec3(1, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? BStart : BL, lineWidth,
                        glm::vec3(0, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        (i == way->nodes.size() - 2 ? way->nodes[i + 1] : way->nodes[i + 2]),
                        color, i == way->nodes.size() - 2 ? BEnd : BR, lineWidth,
                        glm::vec3(0, 1, 1),
                        border, z, dashSize, length, gapColor);

          int num;
          num = wayRenderer.GetVerticesNumber() - 6;
          for (unsigned int n = 0; n < 6; n++)
            wayRenderer.AddNewElement(num + n);

          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? TStart : TL, lineWidth,
                        glm::vec3(1, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        (i == way->nodes.size() - 2 ? way->nodes[i + 1] : way->nodes[i + 2]),
                        color, i == way->nodes.size() - 2 ? BEnd : BR, lineWidth,
                        glm::vec3(0, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? BStart : BL, lineWidth,
                        glm::vec3(0, 1, 1),
                        border, z, dashSize, length, gapColor);
          //
          AddPathVertex(way->nodes[i],
                        i == 0 ? way->nodes[i] : way->nodes[i - 1],
                        way->nodes[i + 1],
                        color, i == 0 ? TStart : TL, lineWidth,
                        glm::vec3(1, 0, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        (i == way->nodes.size() - 2 ? way->nodes[i + 1] : way->nodes[i + 2]),
                        color, i == way->nodes.size() - 2 ? BEnd : BR, lineWidth,
                        glm::vec3(1, 1, 0),
                        border, z, dashSize, length, gapColor);
          AddPathVertex(way->nodes[i + 1],
                        way->nodes[i],
                        (i == way->nodes.size() - 2 ? way->nodes[i + 1] : way->nodes[i + 2]),
                        color, i == way->nodes.size() - 2 ? TEnd : TR, lineWidth,
                        glm::vec3(1, 0, 1),
                        border, z, dashSize, length, gapColor);

          num = wayRenderer.GetVerticesNumber() - 6;
          for (unsigned int n = 0; n < 6; n++)
            wayRenderer.AddNewElement(num + n);
        }
      }
    }
  }

  void MapPainterOpenGL::AddPathVertex(osmscout::Point current, osmscout::Point previous, osmscout::Point next,
                                       osmscout::Color color, PathVertexType type, float width, glm::vec3 barycentric,
                                       int border, double z, float dashsize, float length,
                                       osmscout::Color gapcolor) {
    wayRenderer.AddNewVertex(current.GetLon());
    wayRenderer.AddNewVertex(current.GetLat());

    wayRenderer.AddNewVertex(previous.GetLon());
    wayRenderer.AddNewVertex(previous.GetLat());

    wayRenderer.AddNewVertex(next.GetLon());
    wayRenderer.AddNewVertex(next.GetLat());

    wayRenderer.AddNewVertex(color.GetR());
    wayRenderer.AddNewVertex(color.GetG());
    wayRenderer.AddNewVertex(color.GetB());
    wayRenderer.AddNewVertex(color.GetA());

    wayRenderer.AddNewVertex(gapcolor.GetR());
    wayRenderer.AddNewVertex(gapcolor.GetG());
    wayRenderer.AddNewVertex(gapcolor.GetB());
    wayRenderer.AddNewVertex(gapcolor.GetA());

    wayRenderer.AddNewVertex(int(type));

    wayRenderer.AddNewVertex(width);

    wayRenderer.AddNewVertex(border);

    wayRenderer.AddNewVertex(barycentric.x);
    wayRenderer.AddNewVertex(barycentric.y);
    wayRenderer.AddNewVertex(barycentric.z);

    wayRenderer.AddNewVertex(z);

    wayRenderer.AddNewVertex(dashsize);

    wayRenderer.AddNewVertex(length);
  }

  void
  osmscout::MapPainterOpenGL::ProcessGround(const osmscout::MapData &data, const osmscout::MapParameter &parameter,
                                            const osmscout::Projection &projection,
                                            const osmscout::StyleConfigRef &styleConfig) {
    FillStyleRef landFill=styleConfig->GetLandFillStyle(projection);

    if (!landFill) {
      landFill = this->landFill;
    }

    FillStyleRef seaFill=styleConfig->GetSeaFillStyle(projection);
    FillStyleRef coastFill=styleConfig->GetCoastFillStyle(projection);
    FillStyleRef unknownFill=styleConfig->GetUnknownFillStyle(projection);
    std::vector<Point> points;

    if (!seaFill) {
      seaFill = this->seaFill;
    }

    for (const auto &tile : data.groundTiles) {
      if (tile.type == GroundTile::unknown &&
          !parameter.GetRenderUnknowns()) {
        continue;
      }

      FillStyleRef fill;

      switch (tile.type) {
        case GroundTile::land:
          fill = landFill;
          break;
        case GroundTile::water:
          fill = seaFill;
          break;
        case GroundTile::coast:
          fill = seaFill;
          break;
        case GroundTile::unknown:
          fill = unknownFill;
          break;
      }

      GeoCoord minCoord(tile.yAbs * tile.cellHeight - 90.0,
                        tile.xAbs * tile.cellWidth - 180.0);
      GeoCoord maxCoord(minCoord.GetLat() + tile.cellHeight,
                        minCoord.GetLon() + tile.cellWidth);

      if (tile.coords.empty()) {
        groundTileRenderer.AddNewVertex(minCoord.GetLon());
        groundTileRenderer.AddNewVertex(minCoord.GetLat());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        groundTileRenderer.AddNewVertex(maxCoord.GetLon());
        groundTileRenderer.AddNewVertex(minCoord.GetLat());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        groundTileRenderer.AddNewVertex(maxCoord.GetLon());
        groundTileRenderer.AddNewVertex(maxCoord.GetLat());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());


        groundTileRenderer.AddNewVertex(minCoord.GetLon());
        groundTileRenderer.AddNewVertex(minCoord.GetLat());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        groundTileRenderer.AddNewVertex(minCoord.GetLon());
        groundTileRenderer.AddNewVertex(maxCoord.GetLat());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        groundTileRenderer.AddNewVertex(maxCoord.GetLon());
        groundTileRenderer.AddNewVertex(maxCoord.GetLat());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetR());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetG());
        groundTileRenderer.AddNewVertex(fill->GetFillColor().GetB());

        int num;
        if (groundTileRenderer.GetVerticesNumber() <= 6)
          num = 0;
        else
          num = groundTileRenderer.GetVerticesNumber();
        for (size_t i = 0; i < 6; i++)
          groundTileRenderer.AddNewElement(num + i);

      } else {

        std::vector<osmscout::Point> p;
        for (size_t i = 0; i < tile.coords.size(); i++) {
          double lat;
          double lon;
          lat = minCoord.GetLat() + tile.coords[i].y * tile.cellHeight / GroundTile::Coord::CELL_MAX;
          lon = minCoord.GetLon() + tile.coords[i].x * tile.cellWidth / GroundTile::Coord::CELL_MAX;

          osmscout::GeoCoord g = osmscout::GeoCoord(lat, lon);
          osmscout::Point pt;
          pt.SetCoord(g);
          p.push_back(pt);

        }

        for (int i = p.size() - 1; i >= 0; i--) {
          for (int j = 0; j < i; j++) {
            if (fabs(p[i].GetLat() - p[j].GetLat()) < 0.000000001 &&
                fabs(p[i].GetLon() - p[j].GetLon()) < 0.0000000001) {
              p.erase(p.begin() + i);
            }
          }
        }

        std::vector<GLfloat> points;

        points = osmscout::Triangulate::TriangulatePolygon(p);

        for (size_t t = 0; t < points.size(); t++) {
          if (t % 2 == 0) {
            groundRenderer.AddNewVertex(points[t]);
          } else {
            groundRenderer.AddNewVertex(points[t]);
            groundRenderer.AddNewVertex(fill->GetFillColor().GetR());
            groundRenderer.AddNewVertex(fill->GetFillColor().GetG());
            groundRenderer.AddNewVertex(fill->GetFillColor().GetB());

            if (groundRenderer.GetNumOfVertices() <= 5) {
              groundRenderer.AddNewElement(0);
            } else {
              groundRenderer.AddNewElement(groundRenderer.GetVerticesNumber() - 1);
            }

          }
        }

      }

    }
  }

  void
  osmscout::MapPainterOpenGL::ProcessNodes(const osmscout::MapData &data,
                                           const osmscout::MapParameter &parameter,
                                           const osmscout::Projection &projection,
                                           const osmscout::StyleConfigRef &styleConfig) {

    // osmscout::log.Info() << "Nodes: " << data.nodes.size();

    std::vector<int> icons;
    for (const auto &node: data.nodes) {
      FeatureValueBuffer buffer = node->GetFeatureValueBuffer();
      IconStyleRef iconStyle=styleConfig->GetNodeIconStyle(node->GetFeatureValueBuffer(),
                                                           projection);

      std::vector<TextStyleRef> textStyles;
      styleConfig->GetNodeTextStyles(node->GetFeatureValueBuffer(),
                                     projection,
                                     textStyles);

      bool hasIcon = false;
      if (iconStyle) {
        //has icon?
        OpenGLTextureRef image;
        int IconIndex = 0;
        for (std::list<std::string>::const_iterator path = parameter.GetIconPaths().begin();
             path != parameter.GetIconPaths().end();
             ++path) {
          std::string filename = *path + iconStyle->GetIconName() + ".png";

          int id = iconStyle->GetIconId();
          bool loaded = false;
          for (unsigned int i = 0; i < icons.size(); i++) {
            if (id == icons[i]) {
              IconIndex = i;
              hasIcon = true;
              loaded = true;
              break;
            }
          }

          if (loaded)
            break;

          image = osmscout::LoadPNGOpenGL(filename);

          if (image != nullptr) {
            imageRenderer.AddNewTexture(image);
            icons.push_back(id);
            hasIcon = true;
            IconIndex = icons.size() - 1;
            break;
          }
        }

        if (!iconStyle->GetIconName().empty() && hasIcon) {
          osmscout::GeoCoord coords = node->GetCoords();
          size_t textureWidth = imageRenderer.GetTextureWidth(IconIndex);
          size_t startWidth = imageRenderer.GetTextureWidthSum(IconIndex) - textureWidth;

          imageRenderer.AddNewVertex(coords.GetLon());
          imageRenderer.AddNewVertex(coords.GetLat());
          imageRenderer.AddNewVertex(1);
          imageRenderer.AddNewVertex(startWidth);
          imageRenderer.AddNewVertex(textureWidth);

          imageRenderer.AddNewVertex(coords.GetLon());
          imageRenderer.AddNewVertex(coords.GetLat());
          imageRenderer.AddNewVertex(2);
          imageRenderer.AddNewVertex(startWidth);
          imageRenderer.AddNewVertex(textureWidth);

          imageRenderer.AddNewVertex(coords.GetLon());
          imageRenderer.AddNewVertex(coords.GetLat());
          imageRenderer.AddNewVertex(3);
          imageRenderer.AddNewVertex(startWidth);
          imageRenderer.AddNewVertex(textureWidth);

          imageRenderer.AddNewVertex(coords.GetLon());
          imageRenderer.AddNewVertex(coords.GetLat());
          imageRenderer.AddNewVertex(3);
          imageRenderer.AddNewVertex(startWidth);
          imageRenderer.AddNewVertex(textureWidth);

          imageRenderer.AddNewVertex(coords.GetLon());
          imageRenderer.AddNewVertex(coords.GetLat());
          imageRenderer.AddNewVertex(1);
          imageRenderer.AddNewVertex(startWidth);
          imageRenderer.AddNewVertex(textureWidth);

          imageRenderer.AddNewVertex(coords.GetLon());
          imageRenderer.AddNewVertex(coords.GetLat());
          imageRenderer.AddNewVertex(4);
          imageRenderer.AddNewVertex(startWidth);
          imageRenderer.AddNewVertex(textureWidth);

          int num;
          if (imageRenderer.GetNumOfVertices() <= 30) {
            num = 0;
          } else {
            num = imageRenderer.GetVerticesNumber() - 6;
          }
          imageRenderer.AddNewElement(num);
          imageRenderer.AddNewElement(num + 1);
          imageRenderer.AddNewElement(num + 2);
          imageRenderer.AddNewElement(num + 3);
          imageRenderer.AddNewElement(num + 4);
          imageRenderer.AddNewElement(num + 5);


        } else if (iconStyle->GetSymbol()) {
          osmscout::SymbolRef symbol = iconStyle->GetSymbol();

          double minX;
          double minY;
          double maxX;
          double maxY;
          symbol->GetBoundingBox(projection, minX, minY, maxX, maxY);

          double centerX = (minX + maxX) / 2;
          double centerY = (minY + maxY) / 2;

          for (const auto &p : symbol->GetPrimitives()) {
            DrawPrimitive *primitive = p.get();
            FillStyleRef fillStyle = primitive->GetFillStyle();

            if (PolygonPrimitive *polygon = dynamic_cast<PolygonPrimitive *>(primitive);
                polygon !=nullptr) {

              double meterPerPixelLat = (40075.016686 * 1000) * std::cos(node->GetCoords().GetLat()) /
                                        (float) (std::pow(2, (magnification.GetLevel() + 9)));
              double meterPerPixel = (40075.016686 * 1000) / (float) (std::pow(2, (magnification.GetLevel() + 9)));
              std::vector<osmscout::Vertex2D> vertices;
              for (const auto &pixel : polygon->GetCoords()) {
                double meterToDegreeLat = std::cos(node->GetCoords().GetLat()) * 0.00001;
                double meterToDegree = 0.00001;
                double scale = -1 * meterPerPixel * meterToDegree;
                double scaleLat = -1 * meterPerPixelLat * meterToDegreeLat;

                double x =
                    node->GetCoords().GetLon() + ((projection.ConvertWidthToPixel(pixel.GetX()) - centerX) * scale);
                double y = node->GetCoords().GetLat() +
                           ((projection.ConvertWidthToPixel(pixel.GetY()) - centerY) * scaleLat);

                vertices.push_back(osmscout::Vertex2D(x, y));
              }

              std::vector<GLfloat> points = osmscout::Triangulate::TriangulatePolygon(vertices);

              Color color = fillStyle->GetFillColor();

              for (size_t t = 0; t < points.size(); t++) {
                if (t % 2 == 0) {
                  areaRenderer.AddNewVertex(points[t]);
                } else {
                  areaRenderer.AddNewVertex(points[t]);
                  areaRenderer.AddNewVertex(color.GetR());
                  areaRenderer.AddNewVertex(color.GetG());
                  areaRenderer.AddNewVertex(color.GetB());
                  areaRenderer.AddNewVertex(color.GetA());

                  if (areaRenderer.GetNumOfVertices() <= 6) {
                    areaRenderer.AddNewElement(0);
                  } else {
                    areaRenderer.AddNewElement(areaRenderer.GetVerticesNumber() - 1);
                  }
                }
              }
            }
          }
        }
      }

      for (const auto& textStyle : textStyles) {
        std::string label = textStyle->GetLabel()->GetLabel(parameter,
                                                            buffer);

        int offset = 0;

        if (label.empty()) {
          continue;
        }

        if (hasIcon) {
          offset = 15;
        }

        double alpha = 1.0;
        double fontSize = 1.0;

        if (projection.GetMagnification() > textStyle->GetScaleAndFadeMag() &&
            parameter.GetDrawFadings()) {
          double factor = projection.GetMagnification().GetLevel() - textStyle->GetScaleAndFadeMag().GetLevel();
          fontSize = textStyle->GetSize() * pow(1.5, factor);
          alpha = std::min(textStyle->GetAlpha() / factor, 1.0);

        } else if (textStyle->GetAutoSize()) {
          //fontSize = textStyle->GetSize();
          alpha = textStyle->GetAlpha();
          //TODO
          continue;
        } else {
          fontSize = textStyle->GetSize();
          alpha = textStyle->GetAlpha();
        }

        Color color = textStyle->GetTextColor();
        std::vector<int> textureAtlasIndices = textLoader.AddCharactersToTextureAtlas(label, fontSize);
        int widthSum = 0;
        for (int index: textureAtlasIndices) {
          osmscout::GeoCoord coords = node->GetCoords();
          size_t textureWidth = textLoader.GetWidth(index);
          size_t startWidth = textLoader.GetStartWidth(index);

          int shaderIndices[] = {1, 2, 3, 3, 1, 4};
          for (int i: shaderIndices) {
            textRenderer.AddNewVertex(coords.GetLon());
            textRenderer.AddNewVertex(coords.GetLat());
            textRenderer.AddNewVertex(color.GetR());
            textRenderer.AddNewVertex(color.GetG());
            textRenderer.AddNewVertex(color.GetB());
            textRenderer.AddNewVertex(alpha);
            textRenderer.AddNewVertex(i);
            textRenderer.AddNewVertex(startWidth);
            textRenderer.AddNewVertex(textureWidth);
            textRenderer.AddNewVertex(widthSum);
            textRenderer.AddNewVertex(offset);
          }

          widthSum += textureWidth + 1;

          int num;
          if (textRenderer.GetNumOfVertices() <= 60) {
            num = 0;
          } else {
            num = textRenderer.GetVerticesNumber() - 6;
          }
          textRenderer.AddNewElement(num);
          textRenderer.AddNewElement(num + 1);
          textRenderer.AddNewElement(num + 2);
          textRenderer.AddNewElement(num + 3);
          textRenderer.AddNewElement(num + 4);
          textRenderer.AddNewElement(num + 5);

        }
      }
    }

    OpenGLTextureRef t = textLoader.CreateTexture();
    textRenderer.AddNewTexture(t);

  }

  void osmscout::MapPainterOpenGL::OnZoom(float zoomDirection) {
    if (zoomDirection < 0)
      magnification.SetLevel(MagnificationLevel(magnification.GetLevel() - 1));
    else if (zoomDirection > 0)
      magnification.SetLevel(MagnificationLevel(magnification.GetLevel() + 1));
  }

  void osmscout::MapPainterOpenGL::OnTranslation(int startPointX, int startPointY, int endPointX, int endPointY) {
    double endLat, endLon, startLat, startLon;
    PixelToGeo(startPointX, startPointY, startLon, startLat);
    PixelToGeo(endPointX, endPointY, endLon, endLat);
    double offsetX = (startLon - endLon);
    double offsetY = (startLat - endLat);

    GeoCoord g = osmscout::GeoCoord(center.GetLat() + offsetY / 2, center.GetLon() + offsetX / 2);
    center = g;
  }

  bool osmscout::MapPainterOpenGL::PixelToGeo(double x, double y,
                                              double &lon, double &lat) {
    double tileDPI = 96.0;
    double gradtorad = 2 * M_PI / 360;
    double earthRadiusMeter = 6378137.0;
    double earthExtentMeter = 2 * M_PI * earthRadiusMeter;
    double tileWidthZoom0Aquator = earthExtentMeter;
    double equatorTileWidth = tileWidthZoom0Aquator / magnification.GetMagnification();
    double equatorTileResolution = equatorTileWidth / 256.0;
    double equatorCorrectedEquatorTileResolution = equatorTileResolution * tileDPI / dpi;
    double groundWidthEquatorMeter = width * equatorCorrectedEquatorTileResolution;
    double latOffset = atanh(sin(center.GetLat() * gradtorad));

    double scale = width / (2 * M_PI * groundWidthEquatorMeter / earthExtentMeter);
    double scaleGradtorad = scale * gradtorad;

    x -= width / 2;
    y = height / 2 - y;

    lon = center.GetLon() + x / scaleGradtorad;
    lat = atan(sinh(y / scale + latOffset)) / gradtorad;

    return true;
  }

  osmscout::GeoCoord osmscout::MapPainterOpenGL::GetCenter() {
    return center;
  }

  void osmscout::MapPainterOpenGL::DrawMap(RenderSteps startStep,
                                           RenderSteps /*endStep*/) {
    if (startStep!=RenderSteps::Initialize) {
      return;
    }

    glClearColor(this->landFill.get()->GetFillColor().GetR(),
                 this->landFill.get()->GetFillColor().GetB(),
                 this->landFill.get()->GetFillColor().GetG(),
                 this->landFill.get()->GetFillColor().GetA());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(groundTileRenderer.getVAO());
    glUseProgram(groundTileRenderer.getShaderProgram());

    groundTileRenderer.AddUniform("windowWidth", width);
    groundTileRenderer.AddUniform("windowHeight", height);
    groundTileRenderer.AddUniform("centerLat", center.GetLat());
    groundTileRenderer.AddUniform("centerLon", center.GetLon());
    groundTileRenderer.AddUniform("magnification", magnification.GetMagnification());
    groundTileRenderer.AddUniform("dpi", dpi);

    groundTileRenderer.SetProjection(width, height);
    groundTileRenderer.SetModel();
    groundTileRenderer.SetView(lookX, lookY);
    groundTileRenderer.Draw();

    glBindVertexArray(groundRenderer.getVAO());
    glUseProgram(groundRenderer.getShaderProgram());

    groundRenderer.AddUniform("windowWidth", width);
    groundRenderer.AddUniform("windowHeight", height);
    groundRenderer.AddUniform("centerLat", center.GetLat());
    groundRenderer.AddUniform("centerLon", center.GetLon());
    groundRenderer.AddUniform("magnification", magnification.GetMagnification());
    groundRenderer.AddUniform("dpi", dpi);

    groundRenderer.SetProjection(width, height);
    groundRenderer.SetModel();
    groundRenderer.SetView(lookX, lookY);
    groundRenderer.Draw();

    glBindVertexArray(areaRenderer.getVAO());
    glUseProgram(areaRenderer.getShaderProgram());

    areaRenderer.AddUniform("windowWidth", width);
    areaRenderer.AddUniform("windowHeight", height);
    areaRenderer.AddUniform("centerLat", center.GetLat());
    areaRenderer.AddUniform("centerLon", center.GetLon());
    areaRenderer.AddUniform("magnification", magnification.GetMagnification());
    areaRenderer.AddUniform("dpi", dpi);

    areaRenderer.SetProjection(width, height);
    areaRenderer.SetModel();
    areaRenderer.SetView(lookX, lookY);
    areaRenderer.Draw();

    glBindVertexArray(wayRenderer.getVAO());
    glUseProgram(wayRenderer.getShaderProgram());

    wayRenderer.AddUniform("windowWidth", width);
    wayRenderer.AddUniform("windowHeight", height);
    wayRenderer.AddUniform("centerLat", center.GetLat());
    wayRenderer.AddUniform("centerLon", center.GetLon());
    wayRenderer.AddUniform("magnification", magnification.GetMagnification());
    wayRenderer.AddUniform("dpi", dpi);

    wayRenderer.SetProjection(width, height);
    wayRenderer.SetModel();
    wayRenderer.SetView(lookX, lookY);
    wayRenderer.Draw();

    glBindVertexArray(imageRenderer.getVAO());
    glBindTexture(GL_TEXTURE_2D, imageRenderer.GetTexture());
    glUseProgram(imageRenderer.getShaderProgram());

    imageRenderer.AddUniform("windowWidth", width);
    imageRenderer.AddUniform("windowHeight", height);
    imageRenderer.AddUniform("centerLat", center.GetLat());
    imageRenderer.AddUniform("centerLon", center.GetLon());
    imageRenderer.AddUniform("quadWidth", 14);
    imageRenderer.AddUniform("magnification", magnification.GetMagnification());
    imageRenderer.AddUniform("textureWidthSum", imageRenderer.GetTextureWidth());
    imageRenderer.AddUniform("dpi", dpi);
    imageRenderer.AddUniform("z", 0.001);

    imageRenderer.SetProjection(width, height);
    imageRenderer.SetModel();
    imageRenderer.SetView(lookX, lookY);
    imageRenderer.Draw();

    glBindVertexArray(textRenderer.getVAO());
    glBindTexture(GL_TEXTURE_2D, textRenderer.GetTexture());
    glUseProgram(textRenderer.getShaderProgram());

    textRenderer.AddUniform("windowWidth", width);
    textRenderer.AddUniform("windowHeight", height);
    textRenderer.AddUniform("centerLat", center.GetLat());
    textRenderer.AddUniform("centerLon", center.GetLon());
    textRenderer.AddUniform("textureHeight", textRenderer.GetTextureHeight());
    textRenderer.AddUniform("magnification", magnification.GetMagnification());
    textRenderer.AddUniform("textureWidthSum", textRenderer.GetTextureWidth());
    textRenderer.AddUniform("dpi", dpi);
    textRenderer.AddUniform("z", 0.001);

    textRenderer.SetProjection(width, height);
    textRenderer.SetModel();
    textRenderer.SetView(lookX, lookY);
    textRenderer.Draw();

  }

}
