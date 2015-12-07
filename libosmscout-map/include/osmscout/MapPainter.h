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

#include <osmscout/ObjectRef.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/StyleConfig.h>

#include <osmscout/GroundTile.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/MapParameter.h>

namespace osmscout {

  /**
   * This is the data structure holding all to be rendered data.
   */
  struct OSMSCOUT_MAP_API MapData
  {
    std::vector<NodeRef>     nodes;       //!< Nodes as retrieved from database
    std::vector<AreaRef>     areas;       //!< Areas as retrieved from database
    std::vector<WayRef>      ways;        //!< Ways as retrieved from database
    std::list<NodeRef>       poiNodes;    //!< List of manually added nodes (not managed or changed by the database)
    std::list<AreaRef>       poiAreas;    //!< List of manually added areas (not managed or changed by the database)
    std::list<WayRef>        poiWays;     //!< List of manually added ways (not managed or changed by the database)
    std::list<GroundTile>    groundTiles; //!< List of ground tiles (optional)
  };

  /**
   * Abstract base class of all renders (though you can always write
   * your own renderer without inheriting from this class) It
   * implements the general rendering algorithm. Concrete renders are
   * implemented by implementing the abstract methods defined by this class
   * and used as callbacks to the concrete renderer.
   */
  class OSMSCOUT_MAP_API MapPainter
  {
  public:
    /**
     * Structure used for internal statistic collection
     */
    struct OSMSCOUT_MAP_API DataStatistic
    {
      TypeInfoRef type;        //!< Type
      size_t      objectCount; //!< Sum of nodeCount, wayCount, areaCont
      size_t      nodeCount;   //!< Number of Node objects
      size_t      wayCount;    //!< Number of Way objects
      size_t      areaCount;   //!< Number of Area objects
      size_t      coordCount;  //!< Number of coordinates
      size_t      labelCount;  //!< Number of labels
      size_t      iconCount;   //!< Number of icons

      DataStatistic()
      : objectCount(0),
        nodeCount(0),
        wayCount(0),
        areaCount(0),
        coordCount(0),
        labelCount(0),
        iconCount(0)
      {
        // no code
      }
    };

    struct OSMSCOUT_MAP_API WayData
    {
      ObjectFileRef            ref;
      const FeatureValueBuffer *buffer;         //!< Features of the line segment
      int8_t                   layer;           //!< Layer this way is in
      LineStyleRef             lineStyle;       //!< Line style
      size_t                   wayPriority;     //!< Priority of way (from style sheet)
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
      double                   lineWidth;       //!< Line width
      bool                     startIsClosed;   //!< The end of the way is closed, it does not lead to another way or area
      bool                     endIsClosed;     //!< The end of the way is closed, it does not lead to another way or area

      /**
       * We then draw lines in order of layer (Smaller layers first)
       *
       * Within a layer, we draw lines in order of line style priority (first overlays, lower priority value first)
       *
       * Within a style priority, we draw transparent lines over solid lines
       *
       * Within a style priority we draw lines in order of style sheet way priority
       * (more important ways on top of less important ways, higher priority value first))
       *
       * @param other
       * @return
       */
      inline bool operator<(const WayData& other) const
      {
        if (layer!=other.layer)
        {
          return layer<other.layer;
        }
        else if (lineStyle->GetPriority()!=other.lineStyle->GetPriority()) {
          return lineStyle->GetPriority()<other.lineStyle->GetPriority();
        }
        else {
          return wayPriority>other.wayPriority;
        }
      }
    };

    struct OSMSCOUT_MAP_API WayPathData
    {
      ObjectFileRef            ref;
      const FeatureValueBuffer *buffer;         //!< Features of the line segment
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
    };

    struct OSMSCOUT_MAP_API PolyData
    {
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
    };

    struct OSMSCOUT_MAP_API AreaData
    {
      ObjectFileRef            ref;
      const FeatureValueBuffer *buffer;         //!< Features of the line segment
      FillStyleRef             fillStyle;       //!< Fill style
      double                   minLat;
      double                   maxLat;
      double                   minLon;
      double                   maxLon;
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
      std::list<PolyData>      clippings;       //!< Clipping polygons to be used during drawing of this area
    };

    struct OSMSCOUT_MAP_API LabelData
    {
      bool                     mark;     //!< Labels can temporary get marked during label coverage conflict resolution
      double                   x;        //!< Coordinate of the left, top edge of the text
      double                   y;        //!< Coordinate of the left, top edge of the text
      double                   bx1;      //!< Dimensions of bounding box
      double                   by1;      //!< Dimensions of bounding box
      double                   bx2;      //!< Dimensions of bounding box
      double                   by2;      //!< Dimensions of bounding box
      double                   alpha;    //!< Alpha value of the label
      double                   fontSize; //!< Font size to be used
      LabelStyleRef            style;    //!< Style for drawing
      std::string              text;     //!< The label text
    };

    struct OSMSCOUT_MAP_API LabelLayoutData
    {
      size_t       position;   //!< Relative position of the label
      double       height;     //!< Height of the label
      std::string  label;      //!< The text of the label (only used if TextStyle is set)
      double       fontSize;   //!< The font size (only used if TextStyle is set)
      double       alpha;      //!< The alpha value for rendering the text label (only used if TextStyle is set)
      TextStyleRef textStyle;  //!< The text style for a textual label (optional)
      bool         icon;       //!< Flag signaling that an icon is available, else a symbol will be rendered
      IconStyleRef iconStyle;  //!< The icon style for a icon or symbol
    };

  private:
    CoordBuffer                  *coordBuffer;      //!< Reference to the coordinate buffer

  protected:
    StyleConfigRef               styleConfig;       //!< Reference to the style configuration to be used
    /**
       Scratch variables for path optimization algorithm
     */
    //@{
    TransBuffer                  transBuffer;       //!< Static (avoid reallocation) buffer of transformed coordinates
    //@}

    /**
     * Attribute readers
     */
    //@{
    NameFeatureValueReader       nameReader;
    NameAltFeatureValueReader    nameAltReader;
    RefFeatureValueReader        refReader;
    LayerFeatureValueReader      layerReader;
    WidthFeatureValueReader      widthReader;
    AddressFeatureValueReader    addressReader;
    //@}

    /**
      Presets and similar
     */
    //@{
    std::vector<double>          emptyDash;         //!< Empty dash array
    std::vector<double>          tunnelDash;        //!< Dash array for drawing tunnel border
    FillStyle                    areaMarkStyle;     //!< Marker fill style for internal debugging
    //@}

    std::list<AreaData>          areaData;
    std::list<WayData>           wayData;
    std::list<WayPathData>       wayPathData;

    /**
      Temporary data structures for intelligent label positioning
      */
    //@{
    std::list<LabelData>         labels;
    std::list<LabelData>         overlayLabels;
    std::vector<ScanCell>        wayScanlines;
    std::vector<LabelLayoutData> labelLayoutData;
    //@}

    std::vector<TextStyleRef>    textStyles;     //!< Temporary storage for StyleConfig return value
    std::vector<LineStyleRef>    lineStyles;     //!< Temporary storage for StyleConfig return value
    /**
      Statistics counter
     */
    //@{
    size_t                       waysSegments;
    size_t                       waysDrawn;
    size_t                       waysLabelDrawn;

    size_t                       areasSegments;
    size_t                       areasDrawn;

    size_t                       nodesDrawn;

    size_t                       labelsDrawn;
    //@}

    /**
      Fallback styles in case they are missing for the style sheet
      */
    //@{
    FillStyleRef                 landFill;
    FillStyleRef                 seaFill;
    LabelStyleRef                debugLabel;
    FeatureValueBuffer           coastlineSegmentAttributes;
    //@}

    /**
     Precalculations
      */
    //@{
    double                       labelSpace;
    double                       shieldLabelSpace;
    double                       sameLabelSpace;
    double                       standardFontSize;
    double                       oneMMInPixel;
    //@}

  private:
    /**
     Debuggn
     */
    //@{
    void DumpDataStatistics(const Projection& projection,
                            const MapData& data);
    //@}

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
    void PrepareAreas(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void PrepareWaySegment(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const ObjectFileRef& ref,
                           const FeatureValueBuffer& buffer,
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
                            double fontSize,
                            double height,
                            double alpha,
                            double x,
                            double y);

    void LayoutPointLabels(const Projection& projection,
                           const MapParameter& parameter,
                           const FeatureValueBuffer& buffer,
                           const IconStyleRef iconStyle,
                           const std::vector<TextStyleRef>& textStyles,
                           double x, double y,
                           double objectHeight);

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
                       const TypeInfoRef& type,
                       const FeatureValueBuffer& buffer,
                       const GeoBox& boundingBox);

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
    bool IsVisibleArea(const Projection& projection,
                       const std::vector<GeoCoord>& nodes,
                       double pixelOffset) const;

    bool IsVisibleWay(const Projection& projection,
                      const std::vector<GeoCoord>& nodes,
                      double pixelOffset) const;

    void Transform(const Projection& projection,
                   const MapParameter& parameter,
                   double lon,
                   double lat,
                   double& x,
                   double& y);

    double GetProjectedWidth(const Projection& projection,
                             double minPixel,
                             double width) const;

    inline double GetProjectedWidth(const Projection& projection,
                                    double width) const
    {
      return width/projection.GetPixelSize();
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
     * Returns the height of the font.
     */
    virtual void GetFontHeight(const Projection& projection,
                               const MapParameter& parameter,
                               double fontSize,
                               double& height) = 0;

    /**
      Return the bounding box of the given text. The method is call
      every time a label for a node or an area has to be drawn (which means
      "not for contour labels").

      The backend may decide to relayout the given text, however it must assure
      that later calls to corresponding DrawXXX methods will honour the initial
      bounding box.
      */
    virtual void GetTextDimension(const Projection& projection,
                                  const MapParameter& parameter,
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

    bool Draw(const Projection& projection,
              const MapParameter& parameter,
              const MapData& data);
    //@}

  public:
    MapPainter(const StyleConfigRef& styleConfig,
               CoordBuffer *buffer);
    virtual ~MapPainter();
  };
}

#endif
