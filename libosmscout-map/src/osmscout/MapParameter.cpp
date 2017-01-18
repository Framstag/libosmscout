/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009, 2015  Tim Teulings

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

#include <osmscout/MapParameter.h>

namespace osmscout {

  MapParameter::MapParameter()
  : fontName("sans-serif"),
    fontSize(2.0),
    lineMinWidthPixel(0.2),
    areaMinDimensionMM(1.0),
    optimizeWayNodes(TransPolygon::none),
    optimizeAreaNodes(TransPolygon::none),
    optimizeErrorToleranceMm(0.5),
    drawFadings(true),
    drawWaysWithFixedWidth(false),
    labelLineMinCharCount(15),
    labelLineMaxCharCount(1000/*20*/),
    labelLineFitToArea(true),
    labelLineFitToWidth(8000),
    labelSpace(1.0),
    plateLabelSpace(5.0),
    sameLabelSpace(40.0),
    dropNotVisiblePointLabels(true),
    contourLabelOffset(5.0),
    contourLabelSpace(30.0),
    renderBackground(true),
    renderSeaLand(false),
    renderUnknowns(false),
    debugData(false),
    debugPerformance(false),
    showAltLanguage(false)
  {
    // no code
  }

  MapParameter::~MapParameter()
  {
    // no code
  }

  void MapParameter::SetFontName(const std::string& fontName)
  {
    this->fontName=fontName;
  }

  void MapParameter::SetFontSize(double fontSize)
  {
    this->fontSize=fontSize;
  }

  void MapParameter::SetIconPaths(const std::list<std::string>& paths)
  {
    this->iconPaths=paths;
  }

  void MapParameter::SetPatternPaths(const std::list<std::string>& paths)
  {
    this->patternPaths=paths;
  }

  void MapParameter::SetLineMinWidthPixel(double lineMinWidthPixel)
  {
    this->lineMinWidthPixel=lineMinWidthPixel;
  }

  void MapParameter::SetAreaMinDimensionMM(double areaMinDimensionMM)
  {
    this->areaMinDimensionMM=areaMinDimensionMM;
  }

  void MapParameter::SetOptimizeWayNodes(TransPolygon::OptimizeMethod optimize)
  {
    optimizeWayNodes=optimize;
  }

  void MapParameter::SetOptimizeAreaNodes(TransPolygon::OptimizeMethod optimize)
  {
    optimizeAreaNodes=optimize;
  }

  void MapParameter::SetOptimizeErrorToleranceMm(double errorToleranceMm)
  {
    optimizeErrorToleranceMm=errorToleranceMm;
  }

  void MapParameter::SetDrawFadings(bool drawFadings)
  {
    this->drawFadings=drawFadings;
  }

  void MapParameter::SetDrawWaysWithFixedWidth(bool drawWaysWithFixedWidth)
  {
    this->drawWaysWithFixedWidth=drawWaysWithFixedWidth;
  }

  void MapParameter::SetLabelLineMinCharCount(size_t labelLineMinCharCount)
  {
    this->labelLineMinCharCount=labelLineMinCharCount;
  }

  void MapParameter::SetLabelLineMaxCharCount(size_t labelLineMaxCharCount)
  {
    this->labelLineMaxCharCount=labelLineMaxCharCount;
  }
  void MapParameter::SetLabelLineFitToArea(bool labelLineFitToArea)
  {
    this->labelLineFitToArea=labelLineFitToArea;
  }

  void MapParameter::SetLabelLineFitToWidth(double labelLineFitToWidth)
  {
    this->labelLineFitToWidth=labelLineFitToWidth;
  }

  void MapParameter::SetLabelSpace(double labelSpace)
  {
    this->labelSpace=labelSpace;
  }

  void MapParameter::SetPlateLabelSpace(double plateLabelSpace)
  {
    this->plateLabelSpace=plateLabelSpace;
  }

  void MapParameter::SetSameLabelSpace(double sameLabelSpace)
  {
    this->sameLabelSpace=sameLabelSpace;
  }

  void MapParameter::SetDropNotVisiblePointLabels(bool dropNotVisiblePointLabels)
  {
    this->dropNotVisiblePointLabels=dropNotVisiblePointLabels;
  }

  void MapParameter::SetContourLabelOffset(double contourLabelOffset)
  {
    this->contourLabelOffset=contourLabelOffset;
  }

  void MapParameter::SetContourLabelSpace(double contourLabelSpace)
  {
    this->contourLabelSpace=contourLabelSpace;
  }


  void MapParameter::SetRenderBackground(bool render)
  {
    this->renderBackground=render;
  }

  void MapParameter::SetRenderSeaLand(bool render)
  {
    this->renderSeaLand=render;
  }

  void MapParameter::SetRenderUnknowns(bool render)
  {
    this->renderUnknowns=render;
  }

  void MapParameter::SetDebugData(bool debug)
  {
    debugData=debug;
  }

  void MapParameter::SetDebugPerformance(bool debug)
  {
    debugPerformance=debug;
  }

  void MapParameter::SetBreaker(const BreakerRef& breaker)
  {
    this->breaker=breaker;
  }
}
