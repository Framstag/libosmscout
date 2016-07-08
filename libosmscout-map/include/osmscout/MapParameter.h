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

#include <osmscout/private/MapImportExport.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Transformation.h>

namespace osmscout {

  /**
   * \ingroup Renderer
   *
   * Collection of Parameter that parametrize and influence drawing of the map.
   */
  class OSMSCOUT_MAP_API MapParameter
  {
  private:
    std::string                  fontName;                  //!< Name of the font to use
    double                       fontSize;                  //!< Metric size of base font (aka font size 100%) in millimeter

    std::list<std::string>       iconPaths;                 //!< List of paths to search for images for icons
    std::list<std::string>       patternPaths;              //!< List of paths to search for images for patterns

    double                       lineMinWidthPixel;         //!< Minimum width of an line to be drawn
    double                       areaMinDimensionMM;        //!< Minimum dimension (either width or height) of an area in mm

    TransPolygon::OptimizeMethod optimizeWayNodes;          //!< Try to reduce the number of nodes for
    TransPolygon::OptimizeMethod optimizeAreaNodes;         //!< Try to reduce the number of nodes for
    double                       optimizeErrorToleranceMm;  //!< The maximum error to allow when optimizing lines, in mm
    bool                         drawFadings;               //!< Draw label fadings (default: true)
    bool                         drawWaysWithFixedWidth;    //!< Draw ways using the size of the style sheet, if if the way has a width explicitly given

    // Node and area labels
    size_t                       labelLineCharCount;        //!< Labels will be word wrapped if  they are longer then the given characters
    double                       labelSpace;                //!< Space between point labels in mm (default 3).
    double                       plateLabelSpace;           //!< Space between plates in mm (default 5).
    double                       sameLabelSpace;            //!< Space between labels with the same value in mm (default 40)
    bool                         dropNotVisiblePointLabels; //!< Point labels that are not visible, are clipped during label positioning phase

  private:
// Contour labels
    double                       contourLabelOffset;        //!< Offset in mm for beginning and end of an contour label in relation to contour begin and end
    double                       contourLabelSpace;         //!< Space in mm between repetive labels on the same contour

    bool                         renderBackground;          //!< Render any background features, else render like the background should be transparent
    bool                         renderSeaLand;             //!< Rendering of sea/land tiles

    bool                         debugData;                 //!< Print out some performance relvant information about the data
    bool                         debugPerformance;          //!< Print out some performance information

    bool                         showAltLanguage;           //!< if true, display alternative language (needs support by style sheet and import)

    BreakerRef                   breaker;                   //!< Breaker to abort processing on external request

  public:
    MapParameter();
    virtual ~MapParameter();

    void SetFontName(const std::string& fontName);
    void SetFontSize(double fontSize);

    void SetIconPaths(const std::list<std::string>& paths);
    void SetPatternPaths(const std::list<std::string>& paths);

    void SetLineMinWidthPixel(double lineMinWidthPixel);
    void SetAreaMinDimensionMM(double areaMinDimensionMM);

    void SetOptimizeWayNodes(TransPolygon::OptimizeMethod optimize);
    void SetOptimizeAreaNodes(TransPolygon::OptimizeMethod optimize);
    void SetOptimizeErrorToleranceMm(double errorToleranceMm);

    void SetDrawFadings(bool drawFadings);
    void SetDrawWaysWithFixedWidth(bool drawWaysWithFixedWidth);

    void SetLabelLineCharCount(size_t labelLineCharCount);
    void SetLabelSpace(double labelSpace);
    void SetPlateLabelSpace(double plateLabelSpace);
    void SetSameLabelSpace(double sameLabelSpace);
    void SetDropNotVisiblePointLabels(bool dropNotVisiblePointLabels);

    void SetContourLabelOffset(double contourLabelOffset);
    void SetContourLabelSpace(double contourLabelSpace);

    void SetRenderBackground(bool render);
    void SetRenderSeaLand(bool render);

    void SetDebugData(bool debug);
    void SetDebugPerformance(bool debug);

    void SetShowAltLanguage(bool showAltLanguage);

    void SetBreaker(const BreakerRef& breaker);


    inline std::string GetFontName() const
    {
      return fontName;
    }

    inline double GetFontSize() const
    {
      return fontSize;
    }

    inline const std::list<std::string>& GetIconPaths() const
    {
      return iconPaths;
    }

    inline const std::list<std::string>& GetPatternPaths() const
    {
      return patternPaths;
    }

    inline double GetLineMinWidthPixel() const
    {
      return lineMinWidthPixel;
    }

    inline double GetAreaMinDimensionMM() const
    {
      return areaMinDimensionMM;
    }

    inline TransPolygon::OptimizeMethod GetOptimizeWayNodes() const
    {
      return optimizeWayNodes;
    }

    inline TransPolygon::OptimizeMethod GetOptimizeAreaNodes() const
    {
      return optimizeAreaNodes;
    }

    inline double GetOptimizeErrorToleranceMm() const
    {
      return optimizeErrorToleranceMm;
    }

    inline bool GetDrawFadings() const
    {
      return drawFadings;
    }

    inline bool GetDrawWaysWithFixedWidth() const
    {
      return drawWaysWithFixedWidth;
    }

    inline size_t GetLabelLineCharCount() const
    {
      return labelLineCharCount;
    }

    inline double GetLabelSpace() const
    {
      return labelSpace;
    }

    inline double GetPlateLabelSpace() const
    {
      return plateLabelSpace;
    }

    inline double GetSameLabelSpace() const
    {
      return sameLabelSpace;
    }

    inline bool GetDropNotVisiblePointLabels() const
    {
      return dropNotVisiblePointLabels;
    }

    inline double GetContourLabelOffset() const
    {
      return contourLabelOffset;
    }

    inline double GetContourLabelSpace() const
    {
      return contourLabelSpace;
    }

    inline double GetRenderBackground() const
    {
      return renderBackground;
    }

    inline double GetRenderSeaLand() const
    {
      return renderSeaLand;
    }

    inline bool IsDebugPerformance() const
    {
      return debugPerformance;
    }

    inline bool IsDebugData() const
    {
      return debugData;
    }

    inline bool GetShowAltLanguage() const
    {
      return showAltLanguage;
    }

    bool IsAborted() const
    {
      if (breaker) {
        return breaker->IsAborted();
      }
      else {
        return false;
      }
    }
  };
}

#endif
