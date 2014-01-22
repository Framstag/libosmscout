#ifndef OSMSCOUT_MAP_MAPPAINTER_H
#define OSMSCOUT_MAP_MAPPAINTER_H

/*
  This source is part of the libosmscout-map library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/util/Geometry.h>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>
#include <osmscout/GroundTile.h>
#include <osmscout/ObjectRef.h>
#include <osmscout/StyleConfig.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/HashSet.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/Transformation.h>

namespace osmscout {

  /**
   * Collection of Parameter that parametrize and influence drawing of the map.
   */
  class OSMSCOUT_MAP_API MapParameter
  {
  private:
    double                       dpi;                //! DPI of the display, default is 92

    std::string                  fontName;           //! Name of the font to use
    double                       fontSize;           //! Metric size of base font (aka font size 100%) in millimeter

    std::list<std::string>       iconPaths;          //! List of paths to search for images for icons
    std::list<std::string>       patternPaths;       //! List of paths to search for images for patterns

    double                       lineMinWidthPixel;  //! Minimum width of an line to be drawn

    TransPolygon::OptimizeMethod optimizeWayNodes;   //! Try to reduce the number of nodes for
    TransPolygon::OptimizeMethod optimizeAreaNodes;  //! Try to reduce the number of nodes for
    double                       optimizeErrorToleranceMm;//! The maximum error to allow when optimizing lines, in mm
    bool                         drawFadings;        //! Draw label fadings (default: true)
    bool                         drawWaysWithFixedWidth; //! Draw ways using the size of the style sheet, if if the way has a width explicitely given

    double                       labelSpace;         //! Space between point labels in mm (default 3).
    double                       plateLabelSpace;    //! Space between plates in mm (default 5).
    double                       sameLabelSpace;     //! Space between labels with the same value in mm (default 40)
    bool                         dropNotVisiblePointLabels; //! Point labels that are not visbile, are clips during label positioning phase

    bool                         renderSeaLand;      //! Rendering of sea/land tiles

    bool                         debugPerformance;   //! Print out some performance information

    BreakerRef                   breaker;            //! Breaker to abort processing on external request

  public:
    MapParameter();
    virtual ~MapParameter();

    void SetDPI(double dpi);

    void SetFontName(const std::string& fontName);
    void SetFontSize(double fontSize);

    void SetIconPaths(const std::list<std::string>& paths);
    void SetPatternPaths(const std::list<std::string>& paths);

    void SetLineMinWidthPixel(double lineMinWidthPixel);

    void SetOptimizeWayNodes(TransPolygon::OptimizeMethod optimize);
    void SetOptimizeAreaNodes(TransPolygon::OptimizeMethod optimize);
    void SetOptimizeErrorToleranceMm(double errorToleranceMm);

    void SetDrawFadings(bool drawFadings);
    void SetDrawWaysWithFixedWidth(bool drawWaysWithFixedWidth);

    void SetLabelSpace(double labelSpace);
    void SetPlateLabelSpace(double plateLabelSpace);
    void SetSameLabelSpace(double sameLabelSpace);
    void SetDropNotVisiblePointLabels(bool dropNotVisiblePointLabels);

    void SetRenderSeaLand(bool render);

    void SetDebugPerformance(bool debug);

    void SetBreaker(const BreakerRef& breaker);

    inline double GetDPI() const
    {
      return dpi;
    }

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

    inline double GetOptimizeErrorToleranceDots() const
    {
      return dpi*optimizeErrorToleranceMm/25.4;
    }

    inline bool GetDrawFadings() const
    {
      return drawFadings;
    }

    inline bool GetDrawWaysWithFixedWidth() const
    {
      return drawWaysWithFixedWidth;
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

    inline double GetRenderSeaLand() const
    {
      return renderSeaLand;
    }

    inline bool IsDebugPerformance() const
    {
      return debugPerformance;
    }

    bool IsAborted() const
    {
      if (breaker.Valid()) {
        return breaker->IsAborted();
      }
      else {
        return false;
      }
    }
  };

  struct OSMSCOUT_MAP_API MapData
  {
    std::vector<NodeRef>     nodes;
    std::vector<AreaRef>     areas;
    std::vector<WayRef>      ways;
    std::list<NodeRef>       poiNodes;
    std::list<AreaRef>       poiAreas;
    std::list<WayRef>        poiWays;
    std::list<GroundTile>    groundTiles;
  };

  class OSMSCOUT_MAP_API MapPainter
  {
  public:
    struct OSMSCOUT_MAP_API WayData
    {
      ObjectFileRef           ref;
      const WayAttributes     *attributes;     //! Attributes of line segment
      LineStyleRef            lineStyle;       //! Line style
      size_t                  wayPriority;     //! Priority of way (from style sheet)
      size_t                  transStart;      //! Start of coordinates in transformation buffer
      size_t                  transEnd;        //! End of coordinates in transformation buffer
      double                  lineWidth;       //! Line width
      bool                    startIsClosed;   //! The end of the way is closed, it does not lead to another way or area
      bool                    endIsClosed;     //! The end of the way is closed, it does not lead to another way or area

      /**
       * We then draw lines in order of layer (Smaller layers first)
       *
       * Within a layer, we draw lines in order of line style priority (first overlays, lower priority value first)
       *
       * Within a style priority, we draw transparent lines over solid lines
       *
       * Within a style priority we draw lines in order of style sheet way priority
       * (more important ways on top of less important ways, higher prioruty value first))
       *
       * @param other
       * @return
       */
      inline bool operator<(const WayData& other) const
      {
        if (attributes->GetLayer()!=other.attributes->GetLayer())
        {
          return attributes->GetLayer()<other.attributes->GetLayer();
        }
        else if (lineStyle->GetPriority()!=other.lineStyle->GetPriority()) {
          return lineStyle->GetPriority()<other.lineStyle->GetPriority();
        }
        else {
          return wayPriority>other.wayPriority;
        }
      }
    };

    struct OSMSCOUT_API WayPathData
    {
      ObjectFileRef           ref;
      const WayAttributes     *attributes;     //! Attributes of line segment
      size_t                  transStart;      //! Start of coordinates in transformation buffer
      size_t                  transEnd;        //! End of coordinates in transformation buffer
    };

    struct OSMSCOUT_API PolyData
    {
      size_t                  transStart;      //! Start of coordinates in transformation buffer
      size_t                  transEnd;        //! End of coordinates in transformation buffer
    };

    struct OSMSCOUT_API AreaData
    {
      ObjectFileRef           ref;
      const AreaAttributes    *attributes;     //! Area attributes
      FillStyleRef            fillStyle;       //! Fill style
      double                  minLat;
      double                  maxLat;
      double                  minLon;
      double                  maxLon;
      size_t                  transStart;      //! Start of coordinates in transformation buffer
      size_t                  transEnd;        //! End of coordinates in transformation buffer
      std::list<PolyData>     clippings;       //! Clipping polygons to be used during drawing of this area
    };

    struct OSMSCOUT_API LabelData
    {
      bool              mark;     //! Labels can temporary get marked during label coverage conflict resolution
      double            x;        //! Coordinate of the left, top edge of the text
      double            y;        //! Coordinate of the left, top edge of the text
      double            bx1;      //! Dimensions of bounding box
      double            by1;      //! Dimensions of bounding box
      double            bx2;      //! Dimensions of bounding box
      double            by2;      //! Dimensions of bounding box
      double            alpha;    //! Alpha value of the label
      double            fontSize; //! Font size to be used
      LabelStyleRef     style;    //! Style for drawing
      std::string       text;     //! The label text
    };

  private:
    CoordBuffer               *coordBuffer;
  protected:
    /**
       Scratch variables for path optimization algorithm
     */
    //@{
    TransBuffer               transBuffer; //! Static (avoid reallocation) buffer of transformed coordinates
    //@}

    /**
      Presets and similar
     */
    //@{
    std::vector<double>       emptyDash;         //! Empty dash array
    std::vector<double>       tunnelDash;        //! Dash array for drawing tunnel border
    FillStyle                 areaMarkStyle;     //! Marker fill style for internal debugging
    //@}

    std::list<AreaData>       areaData;
    std::list<WayData>        wayData;
    std::list<WayPathData>    wayPathData;

    /**
      Temporary data structures for intelligent label positioning
      */
    //@{
    std::list<LabelData>      labels;
    std::list<LabelData>      overlayLabels;
    std::vector<ScanCell>     wayScanlines;
    //@}

    std::vector<LineStyleRef> lineStyles;     //! Temporary storage for StyleConfig return value
    /**
      Statistics counter
     */
    //@{
    size_t                    waysSegments;
    size_t                    waysDrawn;
    size_t                    waysLabelDrawn;

    size_t                    areasSegments;
    size_t                    areasDrawn;
    size_t                    areasLabelDrawn;

    size_t                    nodesDrawn;

    size_t                    labelsDrawn;
    //@}

    /**
      Fallback styles in case they are missing for the style sheet
      */
    //@{
    FillStyleRef              landFill;
    FillStyleRef              seaFill;
    TextStyleRef              debugLabel;
    WayAttributes             coastlineSegmentAttributes;
    //@}

    /**
     Precalculations
      */
    //@{
    double                    labelSpace;
    double                    shieldLabelSpace;
    double                    sameLabelSpace;
    //@}

  private:
    void CalculateEffectiveLabelStyle(const Projection& projection,
                                      const MapParameter& parameter,
                                      const LabelStyle& style,
                                      double& fontSize,
                                      double& alpha);

    /**
     Ground tile drawing
     */
    //@{
    void DrawGroundTiles(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const MapData& data);
    //@}

    /**
     Label placement routines
     */
    //@{
    void ClearLabelMarks(std::list<LabelData>& labels);
    void RemoveMarkedLabels(std::list<LabelData>& labels);
    bool MarkAllInBoundingBox(double bx1,
                              double bx2,
                              double by1,
                              double by2,
                              const LabelStyle& style,
                              std::list<LabelData>& labels);
    bool MarkCloseLabelsWithSameText(double bx1,
                                     double bx2,
                                     double by1,
                                     double by2,
                                     const LabelStyle& style,
                                     const std::string& text,
                                     std::list<LabelData>& labels);
    //@}

    /**
      Private draw algorithm implementation routines.
     */
    //@{
    /*
    bool PrepareAreaSegment(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const ObjectFileRef& ref,
                            const AreaAttributes& attributes,
                            const std::vector<GeoCoord>& nodes);*/

    void PrepareAreas(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void PrepareWaySegment(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const ObjectFileRef& ref,
                           const WayAttributes& attributes,
                           const std::vector<GeoCoord>& nodes,
                           const std::vector<Id>& ids);

    void PrepareWays(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const MapData& data);

    void RegisterPointWayLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const PathShieldStyleRef& style,
                               const std::string& text,
                               size_t transStart, size_t transEnd);

    bool RegisterPointLabel(const Projection& projection,
                            const MapParameter& parameter,
                            const LabelStyleRef& style,
                            const std::string& text,
                            double x,
                            double y);

    void DrawWayDecorations(const StyleConfig& styleConfig,
                            const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data);

    void DrawWayLabel(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const WayPathData& data);

    void DrawWayLabels(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const MapParameter& parameter,
                       const MapData& data);

    void DrawAreaLabel(const StyleConfig& styleConfig,
                       const Projection& projection,
                       const MapParameter& parameter,
                       const TypeId& type,
                       const AreaAttributes& attributes,
                       double x,
                       double y);

    void DrawAreaLabels(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data);

    void DrawPOINodes(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void DrawLabels(const StyleConfig& styleConfig,
                    const Projection& projection,
                    const MapParameter& parameter);
    //@}

  protected:
    /**
       Useful global helper functions.
     */
    //@{
    bool IsVisible(const Projection& projection,
                   const std::vector<GeoCoord>& nodes,
                   double pixelOffset) const;

    void Transform(const Projection& projection,
                   const MapParameter& parameter,
                   double lon,
                   double lat,
                   double& x,
                   double& y);

    bool GetBoundingBox(const std::vector<GeoCoord>& nodes,
                        double& xmin, double& ymin,
                        double& xmax, double& ymax) const;
    bool GetCenterPixel(const Projection& projection,
                        const std::vector<GeoCoord>& nodes,
                        double& cx,
                        double& cy) const;

    double GetProjectedWidth(const Projection& projection,
                             double minPixel,
                             double width) const;

    inline double GetProjectedWidth(const Projection& projection,
                                    double width) const
    {
      return width/projection.GetPixelSize();
    }

    /**
     * Convert a width in mm into the equivalent pixel size based on the given DPI
     *
     * @param parameter
     *    MapParameter also holding the DPI inforamtion
     * @param width
     *    Width in mm
     * @return
     *    Width in screen pixel
     */
    inline double ConvertWidthToPixel(const MapParameter& parameter,
                                      double width) const
    {
      return width*parameter.GetDPI()/25.4;
    }

    inline double ConvertPixelToWidth(const MapParameter& parameter,
                                      double pixel) const
    {
      return pixel*25.4/parameter.GetDPI();
    }

    //@}

    /**
      Low level drawing routines that have to be implemented by
      the concrete drawing engine.
     */
    //@{

    /**
      Some optional callbacks between individual processing steps.
     */
    //@{
    virtual void AfterPreprocessing(const StyleConfig& styleConfig,
                                    const Projection& projection,
                                    const MapParameter& parameter,
                                    const MapData& data);
    virtual void BeforeDrawing(const StyleConfig& styleConfig,
                               const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data);
    virtual void AfterDrawing(const StyleConfig& styleConfig,
                              const Projection& projection,
                              const MapParameter& parameter,
                              const MapData& data);
    //@}

    /**
      Return true, if the icon in the IconStyle is available and can be drawn.
      If this method returns false, possibly a fallback (using a Symbol)
      will be chosen.
     */
    virtual bool HasIcon(const StyleConfig& styleConfig,
                         const MapParameter& parameter,
                         IconStyle& style)= 0;

    /**
      Return the bounding box of the given text.

      The backend may decide to relayout the given text, however it must assure
      that later calls to corresponding DrawXXX methods will honour the initial
      bounding box.
      */
    virtual void GetTextDimension(const MapParameter& parameter,
                                  double fontSize,
                                  const std::string& text,
                                  double& xOff,
                                  double& yOff,
                                  double& width,
                                  double& height) = 0;

    /**
      Return the size of the frame around the label text.
     */
    virtual void GetLabelFrame(const LabelStyle& style,
                               double& horizontal,
                               double& vertical);

    /**
      Return the size of the frame around a labels.
     */
    virtual void GetLabelSpace(const LabelStyle& styleA,
                               const LabelStyle& styleB,
                               double& horizontal,
                               double& vertical);

    /**
      (Optionally) fills the area with the given default color
      for ground. In 2D backends this just fills the given area,
      3D backends might draw a sphere or an infinite plane.
     */
    virtual void DrawGround(const Projection& projection,
                            const MapParameter& parameter,
                            const FillStyle& style) = 0;

    /**
      Draw the given text at the given pixel coordinate in a style defined
      by the given LabelStyle.
     */
    virtual void DrawLabel(const Projection& projection,
                           const MapParameter& parameter,
                           const LabelData& label) = 0;

    /**
      Draw the Icon as defined by the IconStyle at the given pixel coordinate.
     */
    virtual void DrawIcon(const IconStyle* style,
                          double x, double y) = 0;

    /**
      Draw the Symbol as defined by the SymbolStyle at the given pixel coordinate.
     */
    virtual void DrawSymbol(const Projection& projection,
                            const MapParameter& parameter,
                            const Symbol& symbol,
                            double x, double y) = 0;

    /**
      Draw simple line with the given style,the given color, the given width
      and the given untransformed nodes.
     */
    virtual void DrawPath(const Projection& projection,
                          const MapParameter& parameter,
                          const Color& color,
                          double width,
                          const std::vector<double>& dash,
                          LineStyle::CapStyle startCap,
                          LineStyle::CapStyle endCap,
                          size_t transStart, size_t transEnd) = 0;

    /**
      Draw the given text as a contour of the given path in a style defined
      by the given LabelStyle.
     */
    virtual void DrawContourLabel(const Projection& projection,
                                  const MapParameter& parameter,
                                  const PathTextStyle& style,
                                  const std::string& text,
                                  size_t transStart, size_t transEnd) = 0;

    /**
      Draw the given text as a contour of the given path in a style defined
      by the given LabelStyle.
     */
    virtual void DrawContourSymbol(const Projection& projection,
                                   const MapParameter& parameter,
                                   const Symbol& symbol,
                                   double space,
                                   size_t transStart, size_t transEnd) = 0;

    /**
      Draw the given area using the given FillStyle
      for the area outline.
     */
    virtual void DrawArea(const Projection& projection,
                          const MapParameter& parameter,
                          const AreaData& area) = 0;
    //@}

    /**
      Med level drawing routines that are already implemented by the base class, but which can be overwritten
      by the driver if necessary.
     */
    //@{
    virtual void DrawNodes(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);

    /**
      Med level drawing routines that are already implemented by the base class, but which can be overwritten
      by the driver if necessary.
     */
    //@{
    virtual void DrawNode(const StyleConfig& styleConfig,
                          const Projection& projection,
                          const MapParameter& parameter,
                          const NodeRef& node);

    virtual void DrawWay(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const WayData& data);

    virtual void DrawWays(const StyleConfig& styleConfig,
                          const Projection& projection,
                          const MapParameter& parameter,
                          const MapData& data);

    virtual void DrawAreas(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);

    bool Draw(const StyleConfig& styleConfig,
              const Projection& projection,
              const MapParameter& parameter,
              const MapData& data);
    //@}

  public:
    MapPainter(CoordBuffer *buffer);
    virtual ~MapPainter();
  };
}

#endif
