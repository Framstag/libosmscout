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

#include <osmscout/MapImportExport.h>

#include <osmscout/ObjectRef.h>

#include <osmscout/TypeFeatures.h>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/StyleConfig.h>

#include <osmscout/GroundTile.h>

#include <osmscout/MapData.h>

#include <osmscout/util/Breaker.h>
#include <osmscout/util/Geometry.h>
#include <osmscout/util/Projection.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/system/Compiler.h>

#include <osmscout/LabelLayouter.h>
#include <osmscout/MapParameter.h>

namespace osmscout {

  enum RenderSteps : size_t
  {
    FirstStep             =  0,
    Initialize            =  0, //!< Setup internal state of renderer for executing next steps with current projection and parameters
    DumpStatistics        =  1, //!< Prints details for debugging, if debug flag (performance, data) is set in renderer parameter
    PreprocessData        =  2, //!< Convert geographical coordinates of object points to screen coordinates,
    Prerender             =  3, //!< Implementation specific preparison
    DrawBaseMapTiles      =  4, //!< Draw unknown/sea/land tiles and tiles with "coastlines" from base map
    DrawGroundTiles       =  5, //!< Same as previous, but from main database
    DrawOSMTileGrids      =  6, //!< If special style exists, renders grid corresponding to OSM tiles
    DrawAreas             =  7,
    DrawWays              =  8,
    DrawWayDecorations    =  9,
    DrawWayContourLabels  = 10,
    PrepareAreaLabels     = 11,
    DrawAreaBorderLabels  = 12,
    DrawAreaBorderSymbols = 13,
    PrepareNodeLabels     = 14,
    DrawLabels            = 15,
    Postrender            = 16, //!< Implementation specific final step
    LastStep              = 16
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
  private:
    typedef void (MapPainter::*StepMethod)(const Projection&,
                                           const MapParameter&,
                                           const MapData&);

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

    /**
     * Data structure for holding temporary data about ways
     */
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
        else if (lineStyle->GetZIndex()!=other.lineStyle->GetZIndex()) {
          return lineStyle->GetZIndex()<other.lineStyle->GetZIndex();
        }
        else if (lineStyle->GetPriority()!=other.lineStyle->GetPriority()) {
          return lineStyle->GetPriority()<other.lineStyle->GetPriority();
        }
        else {
          return wayPriority>other.wayPriority;
        }
      }
    };

    /**
     * Data structure for holding temporary data about way paths (a way may consist of
     * multiple paths/lines rendered)
     */
    struct OSMSCOUT_MAP_API WayPathData
    {
      ObjectFileRef            ref;
      const FeatureValueBuffer *buffer;         //!< Features of the line segment
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
      double                   mainSlotWidth;   //!< Width of main slot, used for relative positioning
    };

    struct OSMSCOUT_MAP_API PolyData
    {
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
    };

    /**
     * Data structure for holding temporary data about areas
     */
    struct OSMSCOUT_MAP_API AreaData
    {
      ObjectFileRef            ref;
      TypeInfoRef              type;
      const FeatureValueBuffer *buffer;         //!< Features of the line segment, can be NULL in case of border only areas
      FillStyleRef             fillStyle;       //!< Fill style
      BorderStyleRef           borderStyle;     //!< Border style
      GeoBox                   boundingBox;     //!< Bounding box of the area
      bool                     isOuter;         //!< flag if this area is outer ring of some relation
      size_t                   transStart;      //!< Start of coordinates in transformation buffer
      size_t                   transEnd;        //!< End of coordinates in transformation buffer
      std::list<PolyData>      clippings;       //!< Clipping polygons to be used during drawing of this area
    };

  protected:
    CoordBuffer                  *coordBuffer;      //!< Reference to the coordinate buffer
    TextStyleRef                 debugLabel;

    /**
      Fallback styles in case they are missing for the style sheet
      */
    //@{
    FillStyleRef                 landFill;
    FillStyleRef                 seaFill;
    FeatureValueBuffer           coastlineSegmentAttributes;
    //@}

  private:
    std::vector<StepMethod>      stepMethods;
    double                       errorTolerancePixel;

    std::list<AreaData>          areaData;
    std::list<WayData>           wayData;
    std::list<WayPathData>       wayPathData;

    std::vector<TextStyleRef>    textStyles;     //!< Temporary storage for StyleConfig return value
    std::vector<LineStyleRef>    lineStyles;     //!< Temporary storage for StyleConfig return value

    /**                           L
     Precalculations
      */
    //@{
    double                       standardFontSize;
    double                       areaMinDimension;
    //@}

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
    NameFeatureValueReader       nameReader;         //!< Value reader for the 'name' feature
    NameAltFeatureValueReader    nameAltReader;      //!< Value reader for the 'alternative name' feature
    RefFeatureValueReader        refReader;          //!< Value reader for the 'ref' feature
    LayerFeatureValueReader      layerReader;        //!< Value reader for the 'layer' feature
    WidthFeatureValueReader      widthReader;        //!< Value reader for the 'width' feature
    AddressFeatureValueReader    addressReader;      //!< Value reader for the 'address' feature
    LanesFeatureValueReader      lanesReader;        //!< Value reader for the 'lanes' feature
    AccessFeatureValueReader     accessReader;       //!< Value reader for the 'lanes' feature
    //@}

    /**
      Presets, precalculations and similar
     */
    //@{
    std::vector<double>          emptyDash;          //!< Empty dash array
    std::vector<double>          tunnelDash;         //!< Dash array for drawing tunnel border
    FillStyle                    areaMarkStyle;      //!< Marker fill style for internal debugging
    double                       contourLabelOffset; //!< Same value as in MapParameter but converted to pixel
    double                       contourLabelSpace;  //!< Same value as in MapParameter but converted to pixel
    double                       shieldGridSizeHoriz;//!< Width of a cell for shield label placement
    double                       shieldGridSizeVert; //!< Height of a cell for shield label placement
    //@}

  private:
    /**
     Debugging
     */
    //@{
    void DumpDataStatistics(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data);
    //@}

    /**
      Private draw algorithm implementation routines.
     */
    //@{
    void PrepareNode(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const NodeRef& node);

    void PrepareNodes(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void CalculatePaths(const StyleConfig& styleConfig,
                        const Projection& projection,
                        const MapParameter& parameter,
                        const ObjectFileRef& ref,
                        const FeatureValueBuffer& buffer,
                        const Way& way);

    void PrepareWays(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const MapData& data);

    void PrepareArea(const StyleConfig& styleConfig,
                     const Projection& projection,
                     const MapParameter& parameter,
                     const AreaRef &area);

    void PrepareAreaLabel(const StyleConfig& styleConfig,
                          const Projection& projection,
                          const MapParameter& parameter,
                          const AreaData& areaData);

    void PrepareAreas(const StyleConfig& styleConfig,
                      const Projection& projection,
                      const MapParameter& parameter,
                      const MapData& data);

    void RegisterPointWayLabel(const Projection& projection,
                               const MapParameter& parameter,
                               const PathShieldStyleRef& style,
                               const std::string& text,
                               const std::vector<Point>& nodes);

    void LayoutPointLabels(const Projection& projection,
                           const MapParameter& parameter,
                           const FeatureValueBuffer& buffer,
                           const IconStyleRef& iconStyle,
                           const std::vector<TextStyleRef>& textStyles,
                           double x, double y,
                           double objectWidth=0,
                           double objectHeight=0);

    bool DrawWayDecoration(const StyleConfig& styleConfig,
                           const Projection& projection,
                           const MapParameter& parameter,
                           const WayPathData& data);

    bool CalculateWayShieldLabels(const StyleConfig& styleConfig,
                                  const Projection& projection,
                                  const MapParameter& parameter,
                                  const Way& data);

    bool DrawWayContourLabel(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const WayPathData& data);

    bool DrawAreaBorderLabel(const StyleConfig& styleConfig,
                             const Projection& projection,
                             const MapParameter& parameter,
                             const AreaData& areaData);

    bool DrawAreaBorderSymbol(const StyleConfig& styleConfig,
                              const Projection& projection,
                              const MapParameter& parameter,
                              const AreaData& areaData);

    void DrawOSMTileGrid(const Projection& projection,
                         const MapParameter& parameter,
                         const Magnification& magnification,
                         const LineStyleRef& osmTileLine);

    void DrawGroundTiles(const Projection& projection,
                         const MapParameter& parameter,
                         const std::list<GroundTile> &groundTiles);

    //@}

    /**
     * This are the official render step methods. One method for each render step.
     */
    //@{
    void InitializeRender(const Projection& projection,
                          const MapParameter& parameter,
                          const MapData& data);

    void DumpStatistics(const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data);

    void PreprocessData(const Projection& projection,
                        const MapParameter& parameter,
                        const MapData& data);

    void Prerender(const Projection& projection,
                   const MapParameter& parameter,
                   const MapData& data);

    void DrawBaseMapTiles(const Projection& projection,
                          const MapParameter& parameter,
                          const MapData& data);

    void DrawGroundTiles(const Projection& projection,
                         const MapParameter& parameter,
                         const MapData& data);

    void DrawOSMTileGrids(const Projection& projection,
                          const MapParameter& parameter,
                          const MapData& data);

    void DrawAreas(const Projection& projection,
                   const MapParameter& parameter,
                   const MapData& data);

    void DrawWays(const Projection& projection,
                  const MapParameter& parameter,
                  const MapData& data);

    void DrawWayDecorations(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data);

    void DrawWayContourLabels(const Projection& projection,
                              const MapParameter& parameter,
                              const MapData& data);

    void PrepareAreaLabels(const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);

    void DrawAreaBorderLabels(const Projection& projection,
                              const MapParameter& parameter,
                              const MapData& data);

    void DrawAreaBorderSymbols(const Projection& projection,
                               const MapParameter& parameter,
                               const MapData& data);

    void PrepareNodeLabels(const Projection& projection,
                           const MapParameter& parameter,
                           const MapData& data);

    void Postrender(const Projection& projection,
                    const MapParameter& parameter,
                    const MapData& data);
    //@]

  protected:
    /**
       Useful global helper functions.
     */
    //@{
    bool IsVisibleArea(const Projection& projection,
                       const GeoBox& boundingBox,
                       double pixelOffset) const;

    bool IsVisibleWay(const Projection& projection,
                      const GeoBox& boundingBox,
                      double pixelOffset) const;

    void Transform(const Projection& projection,
                   const MapParameter& parameter,
                   const GeoCoord& coord,
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

    inline const std::list<WayData>& GetWayData() const
    {
      return wayData;
    }

    inline const std::list<AreaData>& GetAreaData() const
    {
      return areaData;
    }

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

      Icon style dimensions and iconId may be setup for later usage.
     */
    virtual bool HasIcon(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         IconStyle& style)= 0;

    /**
     * Returns the height of the font.
     */
    virtual double GetFontHeight(const Projection& projection,
                                 const MapParameter& parameter,
                                 double fontSize) = 0;

    /**
      (Optionally) fills the area with the given default color
      for ground. In 2D backends this just fills the given area,
      3D backends might draw a sphere or an infinite plane.
     */
    virtual void DrawGround(const Projection& projection,
                            const MapParameter& parameter,
                            const FillStyle& style) = 0;

    /**
      Register regular label with given text at the given pixel coordinate
      in a style defined by the given LabelStyle.
     */
    virtual void RegisterRegularLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const std::vector<LabelData> &labels,
                                      const Vertex2D &position,
                                      double objectWidth) = 0;

    /**
     * Register contour label
     */
    virtual void RegisterContourLabel(const Projection &projection,
                                      const MapParameter &parameter,
                                      const PathLabelData &label,
                                      const LabelPath &labelPath) = 0;

    virtual void DrawLabels(const Projection& projection,
                            const MapParameter& parameter,
                            const MapData& data) = 0;

    /**
      Draw the Icon as defined by the IconStyle at the given pixel coordinate (icon center).
     */
    virtual void DrawIcon(const IconStyle* style,
                          double centerX, double centerY,
                          double width, double height) = 0;

    /**
      Draw the Symbol as defined by the SymbolStyle at the given pixel coordinate (symbol center).
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
      Compute suggested label width for given parameters.
      It may be used by backend for layout labels with wrapping words.
     */
    virtual double GetProposedLabelWidth(const MapParameter& parameter,
                                         double averageCharWidth,
                                         double objectWidth,
                                         size_t stringLength);

    virtual void DrawWay(const StyleConfig& styleConfig,
                         const Projection& projection,
                         const MapParameter& parameter,
                         const WayData& data);

    //@}

    std::vector<OffsetRel> ParseLaneTurns(const LanesFeatureValue&);

  public:
    MapPainter(const StyleConfigRef& styleConfig,
               CoordBuffer *buffer);
    virtual ~MapPainter();

    bool Draw(const Projection& projection,
              const MapParameter& parameter,
              const MapData& data,
              RenderSteps startStep,
              RenderSteps endStep);

    bool Draw(const Projection& projection,
              const MapParameter& parameter,
              const MapData& data);
  };

  /**
   * \ingroup Renderer
   *
   * Batch renderer helps to render map based on multiple databases
   * - map data and corresponding MapPainter
   */
  template <class PainterType>
  class MapPainterBatch {
  protected:
    std::vector<MapDataRef> data;
    std::vector<PainterType> painters;

  protected:

    /**
     * Render bach of multiple databases, step by step (\see RenderSteps).
     * All painters should have initialised its (backend specific) state.
     *
     * @param projection
     * @param parameter
     * @return false on error, true otherwise
     */
    bool batchPaintInternal(const Projection& projection,
                            const MapParameter& parameter)
    {
      bool success=true;
      for (size_t step=osmscout::RenderSteps::FirstStep;
           step<=osmscout::RenderSteps::LastStep;
           step++){

        for (size_t i=0;i<data.size(); i++){
          const MapData &d=*(data[i]);
          success &= painters[i]->Draw(projection,
                                       parameter,
                                       d,
                                       (RenderSteps)step,
                                       (RenderSteps)step);
        }
      }
      return success;
    }

  public:
    explicit MapPainterBatch(size_t expectedCount)
    {
      data.reserve(expectedCount);
      painters.reserve(expectedCount);
    }

    virtual ~MapPainterBatch(){}

    void addData(MapDataRef &d, PainterType &painter)
    {
      data.push_back(d);
      painters.push_back(painter);
    }
  };

  /**
   * \defgroup Renderer Map rendering
   *
   * Classes and methods related to rendering of maps.
   */
}

#endif
