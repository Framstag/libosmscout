#ifndef OSMSCOUT_STYLECONFIG_H
#define OSMSCOUT_STYLECONFIG_H

/*
  This source is part of the libosmscout library
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

#include <limits>
#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeSet.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/Reference.h>
#include <osmscout/util/Transformation.h>

namespace osmscout {

  /**
   * Ways can have a line style
   */
  class OSMSCOUT_MAP_API LineStyle : public Referencable
  {
  public:
    enum CapStyle {
      capButt,
      capRound,
      capSquare
    };

  private:
    Color               lineColor;
    Color               alternateColor;
    Color               outlineColor;
    Color               gapColor;
    double              displayWidth;
    double              width;
    double              fixedWidth;
    CapStyle            capStyle;
    double              outline;
    std::vector<double> dash;

  public:
    LineStyle();
    LineStyle(const LineStyle& style);

    LineStyle& SetLineColor(const Color& color);
    LineStyle& SetAlternateColor(const Color& color);
    LineStyle& SetOutlineColor(const Color& color);
    LineStyle& SetGapColor(const Color& color);
    LineStyle& SetDisplayWidth(double value);
    LineStyle& SetWidth(double value);
    LineStyle& SetFixedWidth(bool fixedWidth);
    LineStyle& SetCapStyle(CapStyle capStyle);
    LineStyle& SetOutline(double value);
    LineStyle& SetDashes(const std::vector<double> dashes);

    inline bool IsVisible() const
    {
      return (displayWidth>0.0 ||
              width>0.0 ||
              fixedWidth>0.0) &&
             (lineColor.IsVisible() ||
              outlineColor.IsVisible());
    }

    inline const Color& GetLineColor() const
    {
      return lineColor;
    }

    inline const Color& GetAlternateColor() const
    {
      return alternateColor;
    }

    inline const Color& GetOutlineColor() const
    {
      return outlineColor;
    }

    inline const Color& GetGapColor() const
    {
      return gapColor;
    }

    inline double GetDisplayWidth() const
    {
      return displayWidth;
    }

    inline double GetWidth() const
    {
      return width;
    }

    inline bool GetFixedWidth() const
    {
      return fixedWidth;
    }

    inline CapStyle GetCapStyle() const
    {
      return capStyle;
    }

    inline double GetOutline() const
    {
      return outline;
    }

    inline bool HasDashes() const
    {
      return !dash.empty();
    }

    inline const std::vector<double>& GetDash() const
    {
      return dash;
    }
  };

  typedef Ref<LineStyle> LineStyleRef;

  /**
   * Areas can have a fill style, filling the area with one color
   */
  class OSMSCOUT_MAP_API FillStyle : public Referencable
  {
  private:
    Color               fillColor;
    std::string         pattern;
    mutable size_t      patternId;
    Mag                 patternMinMag;
    Color               borderColor;
    double              borderWidth;
    std::vector<double> borderDash;

  public:
    FillStyle();
    FillStyle(const FillStyle& style);

    FillStyle& SetFillColor(const Color& color);
    void SetPatternId(size_t id) const;
    FillStyle& SetPattern(const std::string& pattern);
    FillStyle& SetPatternMinMag(Mag mag);
    FillStyle& SetBorderColor(const Color& color);
    FillStyle& SetBorderWidth(double value);
    FillStyle& SetBorderDashes(const std::vector<double> dashes);

    inline bool IsVisible() const
    {
      return (fillColor.IsVisible() ||
              (borderWidth>0 && borderColor.IsVisible()) ||
              !pattern.empty());
    }

    inline const Color& GetFillColor() const
    {
      return fillColor;
    }

    inline bool HasPattern() const
    {
      return !pattern.empty();
    }

    inline size_t GetPatternId() const
    {
      return patternId;
    }

    inline std::string GetPatternName() const
    {
      return pattern;
    }

    inline const Mag& GetPatternMinMag() const
    {
      return patternMinMag;
    }

    inline const Color& GetBorderColor() const
    {
      return borderColor;
    }

    inline double GetBorderWidth() const
    {
      return borderWidth;
    }

    inline bool HasBorderDashes() const
    {
      return !borderDash.empty();
    }

    inline const std::vector<double>& GetBorderDash() const
    {
      return borderDash;
    }
  };

  typedef Ref<FillStyle> FillStyleRef;

  /**
    Nodes, ways and areas can have a label style for drawing text. Text can be formatted
    in different ways.
   */
  class OSMSCOUT_MAP_API LabelStyle : public Referencable
  {
  public:
    enum Style {
      none,
      normal,
      contour,
      plate,
      emphasize
    };

  private:
    Style   style;
    uint8_t priority;
    Mag     scaleAndFadeMag;
    double  size;
    Color   textColor;
    Color   bgColor;
    Color   borderColor;

  public:
    LabelStyle();
    LabelStyle(const LabelStyle& style);

    LabelStyle& SetStyle(Style style);
    LabelStyle& SetPriority(uint8_t priority);
    LabelStyle& SetScaleAndFadeMag(Mag mag);
    LabelStyle& SetSize(double size);
    LabelStyle& SetTextColor(const Color& color);
    LabelStyle& SetBgColor(const Color& color);
    LabelStyle& SetBorderColor(const Color& color);

    inline bool IsVisible() const
    {
      return style!=none &&
          textColor.IsVisible();
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline bool IsPointStyle() const
    {
      return style==normal || style==plate || style==emphasize;
    }

    inline bool IsContourStyle() const
    {
      return style==contour;
    }

    inline uint8_t GetPriority() const
    {
      return priority;
    }

    inline Mag GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    inline double GetSize() const
    {
      return size;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    inline const Color& GetBgColor() const
    {
      return bgColor;
    }

    inline const Color& GetBorderColor() const
    {
      return borderColor;
    }
  };

  typedef Ref<LabelStyle> LabelStyleRef;

  class OSMSCOUT_MAP_API DrawPrimitive : public Referencable
  {
  private:
    FillStyleRef fillStyle;

  public:
    DrawPrimitive(const FillStyleRef& fillStyle);
    virtual ~DrawPrimitive();

    inline const FillStyleRef& GetFillStyle() const
    {
      return fillStyle;
    }

    virtual void GetBoundingBox(double& minX,
                                double& minY,
                                double& maxX,
                                double& maxY) const = 0;
  };

  typedef Ref<DrawPrimitive> DrawPrimitiveRef;

  class OSMSCOUT_MAP_API PolygonPrimitive : public DrawPrimitive
  {
  private:
    std::list<Pixel>  pixels;

  public:
    PolygonPrimitive(const FillStyleRef& fillStyle);

    void AddPixel(const Pixel& pixel);

    inline const std::list<Pixel>& GetPixels() const
    {
      return pixels;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const;
  };

  typedef Ref<PolygonPrimitive> PolygonPrimitiveRef;

  class OSMSCOUT_MAP_API RectanglePrimitive : public DrawPrimitive
  {
  private:
    Pixel        topLeft;
    double       width;
    double       height;

  public:
    RectanglePrimitive(const Pixel& topLeft,
                       double width,
                       double height,
                       const FillStyleRef& fillStyle);

    inline const Pixel& GetTopLeft() const
    {
      return topLeft;
    }

    inline const double& GetWidth() const
    {
      return width;
    }

    inline const double& GetHeight() const
    {
      return height;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const;
  };

  typedef Ref<RectanglePrimitive> RectanglePrimitiveRef;

  class OSMSCOUT_MAP_API CirclePrimitive : public DrawPrimitive
  {
  private:
    Pixel        center;
    double       radius;

  public:
    CirclePrimitive(const Pixel& center,
                    double radius,
                    const FillStyleRef& fillStyle);

    inline const Pixel& GetCenter() const
    {
      return center;
    }

    inline const double& GetRadius() const
    {
      return radius;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const;
  };

  typedef Ref<CirclePrimitive> CirclePrimitiveRef;

  class OSMSCOUT_MAP_API Symbol : public Referencable
  {
  private:
    std::string                 name;
    std::list<DrawPrimitiveRef> primitives;
    double                      minX;
    double                      minY;
    double                      maxX;
    double                      maxY;

  public:
    Symbol(const std::string& name);

    void AddPrimitive(const DrawPrimitiveRef& primitive);

    inline std::string GetName() const
    {
      return name;
    }

    inline const std::list<DrawPrimitiveRef>& GetPrimitives() const
    {
      return primitives;
    }

    inline void GetBoundingBox(double& minX,
                               double& minY,
                               double& maxX,
                               double& maxY) const
    {
      minX=this->minX;
      minY=this->minY;

      maxX=this->maxX;
      maxY=this->maxY;
    }

    inline double GetWidth() const
    {
      return maxX-minX;
    }

    inline double GetHeight() const
    {
      return maxY-minY;
    }
  };

  typedef Ref<Symbol> SymbolRef;

  /**
    IconStyle is for define drawing of external images as icons for nodes and areas
    */
  class OSMSCOUT_MAP_API IconStyle : public Referencable
  {
  private:
    size_t      id;       //! Internal id for fast lookup. 0 == no id defined (yet), max(size_t) == error
    SymbolRef   symbol;
    std::string iconName; //! name of the icon as given in style

  public:
    IconStyle();
    IconStyle(const IconStyle& style);

    IconStyle& SetSymbol(const SymbolRef& symbol);
    IconStyle& SetId(size_t id);
    IconStyle& SetIconName(const std::string& iconName);

    inline bool IsVisible() const
    {
      return !iconName.empty() ||
              symbol.Valid();
    }

    inline const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    inline size_t GetId() const
    {
      return id;
    }

    inline std::string GetIconName() const
    {
      return iconName;
    }
  };

  typedef Ref<IconStyle> IconStyleRef;

  class OSMSCOUT_MAP_API StyleFilter
  {
  private:
    TypeSet types;
    size_t  minLevel;
    size_t  maxLevel;

  public:
    StyleFilter();

    StyleFilter& SetTypes(const TypeSet& types);

    StyleFilter& SetMinLevel(size_t level);
    StyleFilter& SetMaxLevel(size_t level);

    bool HasTypes() const
    {
      return types.HasTypes();
    }

    bool HasType(TypeId typeId) const
    {
      return types.IsTypeSet(typeId);
    }

    size_t GetMinLevel() const
    {
      return minLevel;
    }

    size_t GetMaxLevel() const
    {
      return maxLevel;
    }

    bool HasMaxLevel() const;
  };

  /**
   * A complete style definition
   */
  class OSMSCOUT_MAP_API StyleConfig
  {
  private:
    TypeConfig                                 *typeConfig;

    // Symbol
    OSMSCOUT_HASHMAP<std::string,SymbolRef>    symbols;
    SymbolRef                                  emptySymbol;

    // Node

    std::vector<std::vector<LabelStyleRef> >   nodeRefLabelStyles;
    std::vector<std::vector<LabelStyleRef> >   nodeLabelStyles;
    std::vector<std::vector<IconStyleRef> >    nodeIconStyles;

    std::vector<TypeSet>                       nodeTypeSets;

    // Way

    std::vector<size_t>                        wayPrio;

    std::vector<std::vector<LineStyleRef> >    wayLineStyles;
    std::vector<std::vector<LabelStyleRef> >   wayRefLabelStyles;
    std::vector<std::vector<LabelStyleRef> >   wayNameLabelStyles;

    std::vector<std::vector<TypeSet> >         wayTypeSets;

    // Area

    std::vector<std::vector<FillStyleRef> >    areaFillStyles;
    std::vector<std::vector<LabelStyleRef> >   areaLabelStyles;
    std::vector<std::vector<IconStyleRef> >    areaIconStyles;

    std::vector<TypeSet>                       areaTypeSets;


  private:
    void GetAllNodeTypes(std::list<TypeId>& types);
    void GetAllWayTypes(std::list<TypeId>& types);
    void GetAllAreaTypes(std::list<TypeId>& types);

    void PostprocessNodes();
    void PostprocessWays();
    void PostprocessAreas();

  public:
    StyleConfig(TypeConfig* typeConfig);
    virtual ~StyleConfig();

    bool RegisterSymbol(const SymbolRef& symbol);
    const SymbolRef& GetSymbol(const std::string& name) const;

    void Postprocess();

    TypeConfig* GetTypeConfig() const;

    StyleConfig& SetWayPrio(TypeId type, size_t prio);

    void GetNodeRefLabelStyles(const StyleFilter& filter, std::list<LabelStyleRef>& styles);
    void GetNodeNameLabelStyles(const StyleFilter& filter, std::list<LabelStyleRef>& styles);
    void GetNodeIconStyles(const StyleFilter& filter, std::list<IconStyleRef>& styles);

    void GetWayLineStyles(const StyleFilter& filter, std::list<LineStyleRef>& styles);
    void GetWayRefLabelStyles(const StyleFilter& filter, std::list<LabelStyleRef>& styles);
    void GetWayNameLabelStyles(const StyleFilter& filter, std::list<LabelStyleRef>& styles);

    void GetAreaFillStyles(const StyleFilter& filter, std::list<FillStyleRef>& styles);
    void GetAreaLabelStyles(const StyleFilter& filter, std::list<LabelStyleRef>& styles);
    void GetAreaIconStyles(const StyleFilter& filter, std::list<IconStyleRef>& styles);

    void GetNodeTypesWithMaxMag(double maxMag,
                                TypeSet& types) const;
    void GetWayTypesByPrioWithMaxMag(double mag,
                                     std::vector<TypeSet>& types) const;
    void GetAreaTypesWithMaxMag(double maxMag,
                                TypeSet& types) const;


    inline size_t GetWayPrio(TypeId type) const
    {
      if (type<wayPrio.size()) {
        return wayPrio[type];
      }
      else {
        return std::numeric_limits<size_t>::max();
      }
    }

    IconStyle* GetNodeIconStyle(TypeId type, size_t level) const;
    LabelStyle* GetNodeRefLabelStyle(TypeId type, size_t level) const;
    LabelStyle* GetNodeLabelStyle(TypeId type, size_t level) const;

    LineStyle* GetWayLineStyle(TypeId type, size_t level) const;
    LabelStyle* GetWayRefLabelStyle(TypeId type, size_t level) const;
    LabelStyle* GetWayNameLabelStyle(TypeId type, size_t level) const;

    FillStyle* GetAreaFillStyle(TypeId type, size_t level) const;
    LabelStyle* GetAreaLabelStyle(TypeId type, size_t level) const;
    IconStyle* GetAreaIconStyle(TypeId type, size_t level) const;
  };
}

#endif
