#ifndef OSMSCOUT_MAP_MAPPARAMETER_H
#define OSMSCOUT_MAP_MAPPARAMETER_H

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

#include <list>
#include <string>

#include <osmscoutmap/MapImportExport.h>

#include <osmscoutmap/StyleProcessor.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Transformation.h>

namespace osmscout {

  /**
   * \ingroup Renderer
   *
   * Collection of Parameter that parametrize and influence drawing of the map.
   */
  class OSMSCOUT_MAP_API MapParameter CLASS_FINAL
  {
  public:
    enum class IconMode
    {
      FixedSizePixmap,  // !< raster icons should be used, iconPixelSize will be used for rendering
      ScaledPixmap,     // !< raster icons should be used, icons will be scaled to iconSize
      OriginalPixmap,   // !< raster icons should be used, icons will keep dimensions of original image
      Scalable          // !< vector icons should be used, icons will be scaled to iconSize
    };

    enum class PatternMode
    {
      OriginalPixmap,   // !< raster pattern should be used, it will keep dimensions of original image
      Scalable          // !< vector pattern should be used, it will be scaled to patternSize
    };

  private:
    std::string                         fontName;                  //!< Name of the font to use
    double                              fontSize;                  //!< Metric size of base font (aka font size 100%) in millimeter

    std::list<std::string>              iconPaths;                 //!< List of paths to search for images for icons
    std::list<std::string>              patternPaths;              //!< List of paths to search for images for patterns

    double                              lineMinWidthPixel;         //!< Minimum width of an line to be drawn
    double                              areaMinDimensionMM;        //!< Minimum dimension (either width or height) of an area in mm

    double                              sidecarMaxDistanceMM;      //!< Screen maximum distance of sidecar relative offset in mm
    double                              sidecarDistance;           //!< Distance of sidecar relative offset in m
    double                              sidecarMinDistanceMM;      //!< Screen minimum distance of sidecar relative offset in mm

    TransPolygon::OptimizeMethod        optimizeWayNodes;          //!< Try to reduce the number of nodes for
    TransPolygon::OptimizeMethod        optimizeAreaNodes;         //!< Try to reduce the number of nodes for
    double                              optimizeErrorToleranceMm;  //!< The maximum error to allow when optimizing lines, in mm
    bool                                drawFadings;               //!< Draw label fadings (default: true)
    bool                                drawWaysWithFixedWidth;    //!< Draw ways using the size of the style sheet, if if the way has a width explicitly given

    // Node and area labels, icons
    size_t                              labelLineMinCharCount;     //!< Labels will be _never_ word wrapped if they are shorter then the given characters
    size_t                              labelLineMaxCharCount;     //!< Labels will be word wrapped if they are longer then the given characters
    bool                                labelLineFitToArea;        //!< Labels will be word wrapped to fit object area
    double                              labelLineFitToWidth;       //!< Labels will be word wrapped to fit given width in pixels

    double                              labelPadding;              //!< Space around point labels in mm (default 1).
    double                              plateLabelPadding;         //!< Space around plates in mm (default 5).
    double                              overlayLabelPadding;       //!< Space around overlay labels in mm (default 6).

    IconMode                            iconMode;                  //!< Mode of icons, it controls what type of files would be loaded and how icon dimensions will be calculated
    double                              iconSize;                  //!< Size of icons in mm (default 3.7)
    double                              iconPixelSize;             //!< Size of icons in px (default 14)
    double                              iconPadding;               //!< Space around icons and symbols in mm (default 1).

    PatternMode                         patternMode;               //!< Mode of pattern, it controls what type of files would be loaded and how pattern geometry will be calculated
    double                              patternSize;               //!< Size of pattern image in mm (default 3.7)

    double                              labelLayouterOverlap;      //!< Overlap of visible area used by label layouter in mm (default 30)

  private:
// Contour labels
    double                              contourLabelOffset;        //!< Offset in mm for beginning and end of an contour label in relation to contour begin and end
    double                              contourLabelSpace;         //!< Space in mm between repetitive labels on the same contour
    double                              contourLabelPadding;       //!< Space around contour labels in mm (default 1).

    std::string                         routeLabelSeparator;       //!< Separator of contour labels

    bool                                renderBackground;          //!< Render any background features, else render like the background should be transparent
    bool                                renderSeaLand;             //!< Rendering of sea/land tiles
    bool                                renderUnknowns;            //!< Unknown areas are not rendered (transparent)
    bool                                renderContourLines;        //!< Render ContourLines
    bool                                renderHillShading;         //!< Render hill shades

    bool                                debugData;                 //!< Print out some performance relevant information about the data
    bool                                debugPerformance;          //!< Print out some performance information

    size_t                              warnObjectCountLimit;      //!< Limit for objects/type. If limit is reached a warning is created
    size_t                              warnCoordCountLimit;       //!< Limit for coords/type. If limit is reached a warning is created

    bool                                showAltLanguage;           //!< if true, display alternative language (needs support by style sheet and import)

    Locale                              locale;                    //!< Locale used by the renderer, for example peak elevation

    std::vector<FillStyleProcessorRef > fillProcessors;            //!< List of processors for FillStyles for types

    BreakerRef                          breaker;                   //!< Breaker to abort processing on external request

  public:
    MapParameter();

    void SetFontName(const std::string& fontName);
    void SetFontSize(double fontSize);

    void SetIconPaths(const std::list<std::string>& paths);
    void SetPatternPaths(const std::list<std::string>& paths);

    void SetLineMinWidthPixel(double lineMinWidthPixel);
    void SetAreaMinDimensionMM(double areaMinDimensionMM);

    void SetSidecarMaxDistanceMM();
    void SetSidecarDistance();
    void SetSidecarMinDistanceMM();

    void SetOptimizeWayNodes(TransPolygon::OptimizeMethod optimize);
    void SetOptimizeAreaNodes(TransPolygon::OptimizeMethod optimize);
    void SetOptimizeErrorToleranceMm(double errorToleranceMm);

    void SetDrawFadings(bool drawFadings);
    void SetDrawWaysWithFixedWidth(bool drawWaysWithFixedWidth);

    void SetLabelLineMinCharCount(size_t labelLineMinCharCount);
    void SetLabelLineMaxCharCount(size_t labelLineMaxCharCount);
    void SetLabelLineFitToArea(bool labelLineFitToArea);
    void SetLabelLineFitToWidth(double labelLineFitToWidth);

    void SetLabelPadding(double labelPadding);
    void SetPlateLabelPadding(double plateLabelPadding);
    void SetOverlayLabelPadding(double padding);

    void SetIconMode(const IconMode &mode);
    void SetIconSize(double size);
    void SetIconPixelSize(double size);
    void SetIconPadding(double padding);

    void SetPatternMode(const PatternMode &mode);
    void SetPatternSize(double size);

    void SetContourLabelPadding(double padding);

    void SetRouteLabelSeparator(const std::string &separator);

    void SetLabelLayouterOverlap(double labelLayouterOverlap);

    void SetContourLabelOffset(double contourLabelOffset);
    void SetContourLabelSpace(double contourLabelSpace);

    void SetRenderBackground(bool render);
    void SetRenderSeaLand(bool render);
    void SetRenderUnknowns(bool render);
    void SetRenderContourLines(bool render);
    void SetRenderHillShading(bool render);

    void SetDebugData(bool debug);
    void SetDebugPerformance(bool debug);

    void SetWarningObjectCountLimit(size_t limit);
    void SetWarningCoordCountLimit(size_t limit);

    void SetShowAltLanguage(bool showAltLanguage);

    void SetLocale(const Locale &locale);

    void RegisterFillStyleProcessor(size_t typeIndex,
                                    const FillStyleProcessorRef& processor);

    FillStyleProcessorRef GetFillStyleProcessor(size_t typeIndex) const;

    void SetBreaker(const BreakerRef& breaker);


    std::string GetFontName() const
    {
      return fontName;
    }

    double GetFontSize() const
    {
      return fontSize;
    }

    const std::list<std::string>& GetIconPaths() const
    {
      return iconPaths;
    }

    const std::list<std::string>& GetPatternPaths() const
    {
      return patternPaths;
    }

    double GetLineMinWidthPixel() const
    {
      return lineMinWidthPixel;
    }

    double GetAreaMinDimensionMM() const
    {
      return areaMinDimensionMM;
    }

    double GetSidecarMaxDistanceMM() const
    {
      return sidecarMaxDistanceMM;
    }

    double GetSidecarDistance() const
    {
      return sidecarDistance;
    }

    double GetSidecarMinDistanceMM() const
    {
      return sidecarMinDistanceMM;
    }

    TransPolygon::OptimizeMethod GetOptimizeWayNodes() const
    {
      return optimizeWayNodes;
    }

    TransPolygon::OptimizeMethod GetOptimizeAreaNodes() const
    {
      return optimizeAreaNodes;
    }

    double GetOptimizeErrorToleranceMm() const
    {
      return optimizeErrorToleranceMm;
    }

    bool GetDrawFadings() const
    {
      return drawFadings;
    }

    bool GetDrawWaysWithFixedWidth() const
    {
      return drawWaysWithFixedWidth;
    }

    size_t GetLabelLineMinCharCount() const
    {
      return labelLineMinCharCount;
    }

    size_t GetLabelLineMaxCharCount() const
    {
      return labelLineMaxCharCount;
    }

    bool GetLabelLineFitToArea() const
    {
      return labelLineFitToArea;
    }

    double GetLabelLineFitToWidth() const
    {
      return labelLineFitToWidth;
    }

    double GetLabelPadding() const
    {
      return labelPadding;
    }

    double GetPlateLabelPadding() const
    {
      return plateLabelPadding;
    }

    double GetOverlayLabelPadding() const
    {
      return overlayLabelPadding;
    }

    IconMode GetIconMode() const
    {
      return iconMode;
    }

    double GetIconSize() const
    {
      return iconSize;
    }

    double GetIconPixelSize() const
    {
      return iconPixelSize;
    }

    double GetIconPadding() const
    {
      return iconPadding;
    }

    PatternMode GetPatternMode() const
    {
      return patternMode;
    }

    double GetPatternSize() const
    {
      return patternSize;
    }

    double GetContourLabelPadding() const
    {
      return contourLabelPadding;
    }

    std::string GetRouteLabelSeparator() const
    {
      return routeLabelSeparator;
    }

    double GetLabelLayouterOverlap() const
    {
      return labelLayouterOverlap;
    }

    double GetContourLabelOffset() const
    {
      return contourLabelOffset;
    }

    double GetContourLabelSpace() const
    {
      return contourLabelSpace;
    }

    bool GetRenderBackground() const
    {
      return renderBackground;
    }

    bool GetRenderSeaLand() const
    {
      return renderSeaLand;
    }

    bool GetRenderUnknowns() const
    {
      return renderUnknowns;
    }

    bool GetRenderContourLines() const
    {
      return renderContourLines;
    }

    bool GetRenderHillShading() const
    {
      return renderHillShading;
    }

    bool IsDebugPerformance() const
    {
      return debugPerformance;
    }

    bool IsDebugData() const
    {
      return debugData;
    }

    size_t GetWarningObjectCountLimit() const
    {
      return warnObjectCountLimit;
    }

    size_t GetWarningCoordCountLimit() const
    {
      return warnCoordCountLimit;
    }

    bool GetShowAltLanguage() const
    {
      return showAltLanguage;
    }

    Locale GetLocale() const
    {
      return locale;
    }

    Locale& GetLocaleRef()
    {
      return locale;
    }

    bool IsAborted() const
    {
      if (breaker) {
        return breaker->IsAborted();
      }

      return false;
    }
  };
}

#endif
