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

#include <osmscout/Node.h>
#include <osmscout/Relation.h>
#include <osmscout/Way.h>

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

    enum Attribute {
      attrLineColor,
      attrAlternateColor,
      attrOutlineColor,
      attrGapColor,
      attrDisplayWidth,
      attrWidth,
      attrFixedWidth,
      attrCapStyle,
      attrOutline,
      attrDashes
    };

  private:
    Color               lineColor;
    Color               alternateColor;
    Color               outlineColor;
    Color               gapColor;
    double              displayWidth;
    double              width;
    bool                fixedWidth;
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
              width>0.0) &&
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

    void CopyAttributes(const LineStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef Ref<LineStyle> LineStyleRef;

  /**
   * Areas can have a fill style, filling the area with one color
   */
  class OSMSCOUT_MAP_API FillStyle : public Referencable
  {
  public:
    enum Attribute {
      attrFillColor,
      attrPattern,
      attrPatternMinMag,
      attrBorderColor,
      attrBorderWidth,
      attrBorderDashes
    };

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

    void CopyAttributes(const FillStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef Ref<FillStyle> FillStyleRef;

  class OSMSCOUT_MAP_API LabelStyle : public Referencable
  {
  private:
    uint8_t priority;
    double  size;

  public:
    LabelStyle();
    LabelStyle(const LabelStyle& style);
    virtual ~LabelStyle();

    virtual bool IsVisible() const = 0;
    virtual double GetAlpha() const = 0;

    LabelStyle& SetPriority(uint8_t priority);
    LabelStyle& SetSize(double size);

    inline uint8_t GetPriority() const
    {
      return priority;
    }

    inline double GetSize() const
    {
      return size;
    }

  };

  typedef Ref<LabelStyle> LabelStyleRef;

  /**
    Nodes, ways and areas can have a label style for drawing text. Text can be formatted
    in different ways.
   */
  class OSMSCOUT_MAP_API TextStyle : public LabelStyle
  {
  public:
    enum Style {
      normal,
      emphasize
    };

    enum Label {
      none,
      name,
      ref
    };

    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrTextColor,
      attrStyle,
      attrScaleAndFadeMag
    };

  private:
    Style   style;
    Mag     scaleAndFadeMag;
    Label   label;
    Color   textColor;

  public:
    TextStyle();
    TextStyle(const TextStyle& style);

    TextStyle& SetPriority(uint8_t priority);
    TextStyle& SetSize(double size);
    TextStyle& SetLabel(Label label);
    TextStyle& SetTextColor(const Color& color);
    TextStyle& SetStyle(Style style);
    TextStyle& SetScaleAndFadeMag(Mag mag);

    inline bool IsVisible() const
    {
      return label!=none &&
             GetTextColor().IsVisible();
    }

    inline double GetAlpha() const
    {
      return textColor.GetA();
    }

    inline Label GetLabel() const
    {
      return label;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline Mag GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    void CopyAttributes(const TextStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef Ref<TextStyle> TextStyleRef;

  /**
    Nodes, ways and areas can have a label style for drawing text. Text can be formatted
    in different ways.
   */
  class OSMSCOUT_MAP_API ShieldStyle : public LabelStyle
  {
  public:
    enum Label {
      none,
      name,
      ref
    };

    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrTextColor,
      attrBgColor,
      attrBorderColor
    };

  private:
    Label   label;
    Color   textColor;
    Color   bgColor;
    Color   borderColor;

  public:
    ShieldStyle();
    ShieldStyle(const ShieldStyle& style);

    ShieldStyle& SetLabel(Label label);
    ShieldStyle& SetPriority(uint8_t priority);
    ShieldStyle& SetSize(double size);
    ShieldStyle& SetTextColor(const Color& color);
    ShieldStyle& SetBgColor(const Color& color);
    ShieldStyle& SetBorderColor(const Color& color);

    inline bool IsVisible() const
    {
      return label!=none &&
             GetTextColor().IsVisible();
    }

    inline double GetAlpha() const
    {
      return textColor.GetA();
    }

    inline Label GetLabel() const
    {
      return label;
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

    void CopyAttributes(const ShieldStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef Ref<ShieldStyle> ShieldStyleRef;

  /**
    Nodes, ways and areas can have a label style for drawing text. Text can be formatted
    in different ways.
   */
  class OSMSCOUT_MAP_API PathTextStyle : public Referencable
  {
  public:
    enum Label {
      none,
      name,
      ref
    };

    enum Attribute {
      attrLabel,
      attrSize,
      attrTextColor
    };

  private:
    Label   label;
    double  size;
    Color   textColor;

  public:
    PathTextStyle();
    PathTextStyle(const PathTextStyle& style);

    PathTextStyle& SetLabel(Label label);
    PathTextStyle& SetSize(double size);
    PathTextStyle& SetTextColor(const Color& color);

    inline bool IsVisible() const
    {
      return label!=none &&
             textColor.IsVisible();
    }

    inline Label GetLabel() const
    {
      return label;
    }

    inline double GetSize() const
    {
      return size;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    void CopyAttributes(const PathTextStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef Ref<PathTextStyle> PathTextStyleRef;

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
  public:
    enum Attribute {
      attrSymbol,
      attrIconName,
    };

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

    void CopyAttributes(const IconStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef Ref<IconStyle> IconStyleRef;

  class OSMSCOUT_MAP_API StyleFilter
  {
  public:

  private:
    TypeSet types;
    size_t  minLevel;
    size_t  maxLevel;
    bool    oneway;

  public:
    StyleFilter();
    StyleFilter(const StyleFilter& other);

    StyleFilter& SetTypes(const TypeSet& types);

    StyleFilter& SetMinLevel(size_t level);
    StyleFilter& SetMaxLevel(size_t level);
    StyleFilter& SetOneway(bool oneway);

    inline bool HasTypes() const
    {
      return types.HasTypes();
    }

    inline bool HasType(TypeId typeId) const
    {
      return types.IsTypeSet(typeId);
    }

    inline size_t GetMinLevel() const
    {
      return minLevel;
    }

    inline size_t GetMaxLevel() const
    {
      return maxLevel;
    }

    inline bool GetOneway() const
    {
      return oneway;
    }

    inline bool HasMaxLevel() const
    {
      return maxLevel!=std::numeric_limits<size_t>::max();
    }
  };

  class OSMSCOUT_MAP_API StyleCriteria
  {
  public:

  private:
    size_t minLevel;
    size_t maxLevel;
    bool   oneway;

  public:
    StyleCriteria();
    StyleCriteria(const StyleFilter& other);
    StyleCriteria(const StyleCriteria& other);

    inline size_t GetMinLevel() const
    {
      return minLevel;
    }

    inline size_t GetMaxLevel() const
    {
      return maxLevel;
    }

    inline bool GetOneway() const
    {
      return oneway;
    }

    inline bool HasMaxLevel() const
    {
      return maxLevel!=std::numeric_limits<size_t>::max();
    }

   bool Matches(size_t level) const;
   bool Matches(const SegmentAttributes& attributes,
                size_t level) const;
  };

  template<class S, class A>
  struct StyleSelector
  {
    StyleCriteria  criteria;
    std::set<A>    attributes;
    Ref<S>         style;

    StyleSelector(StyleFilter& filter)
    : criteria(filter),
    style(new S())
    {
      // no code
    }
  };

  typedef StyleSelector<TextStyle,TextStyle::Attribute>         TextStyleSelector;
  typedef std::list<TextStyleSelector>                          TextStyleSelectorList;

  typedef StyleSelector<ShieldStyle,ShieldStyle::Attribute>     ShieldStyleSelector;
  typedef std::list<ShieldStyleSelector>                        ShieldStyleSelectorList;

  typedef StyleSelector<PathTextStyle,PathTextStyle::Attribute> PathTextStyleSelector;
  typedef std::list<PathTextStyleSelector>                      PathTextStyleSelectorList;

  typedef StyleSelector<IconStyle,IconStyle::Attribute>         IconStyleSelector;
  typedef std::list<IconStyleSelector>                          IconStyleSelectorList;

  typedef StyleSelector<LineStyle,LineStyle::Attribute>         LineStyleSelector;
  typedef std::list<LineStyleSelector>                          LineStyleSelectorList;

  typedef StyleSelector<FillStyle,FillStyle::Attribute>         FillStyleSelector;
  typedef std::list<FillStyleSelector>                          FillStyleSelectorList;

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

    std::vector<TextStyleSelectorList>         nodeTextStyleSelectors;
    std::vector<IconStyleSelectorList>         nodeIconStyleSelectors;

    std::vector<TypeSet>                       nodeTypeSets;

    // Way

    std::vector<size_t>                        wayPrio;

    std::vector<LineStyleSelectorList>         wayLineStyleSelectors;
    std::vector<PathTextStyleSelectorList>     wayPathTextStyleSelectors;
    std::vector<ShieldStyleSelectorList>       wayShieldStyleSelectors;

    std::vector<std::vector<TypeSet> >         wayTypeSets;

    // Area

    std::vector<FillStyleSelectorList>         areaFillStyleSelectors;
    std::vector<TextStyleSelectorList>         areaTextStyleSelectors;
    std::vector<IconStyleSelectorList>         areaIconStyleSelectors;

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

    void SetNodeTextSelector(const StyleFilter& filter,
                             TextStyleSelector& selector);
    void SetNodeIconSelector(const StyleFilter& filter,
                             IconStyleSelector& selector);

    void SetWayLineSelector(const StyleFilter& filter,
                            LineStyleSelector& selector);
    void SetWayPathTextSelector(const StyleFilter& filter,
                                PathTextStyleSelector& selector);
    void SetWayShieldSelector(const StyleFilter& filter,
                              ShieldStyleSelector& selector);

    void SetAreaFillSelector(const StyleFilter& filter,
                             FillStyleSelector& selector);
    void SetAreaTextSelector(const StyleFilter& filter,
                             TextStyleSelector& selector);
    void SetAreaIconSelector(const StyleFilter& filter,
                             IconStyleSelector& selector);

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

    void GetNodeTextStyle(const Node& node,
                          size_t level,
                          TextStyleRef& textStyle) const;

    void GetNodeIconStyle(const Node& node,
                          size_t level,
                          IconStyleRef& iconStyle) const;

    void GetWayLineStyle(const SegmentAttributes& way,
                         size_t level,
                         LineStyleRef& lineStyle) const;
    void GetWayPathTextStyle(const SegmentAttributes& way,
                             size_t level,
                             PathTextStyleRef& pathTextStyle) const;
    void GetWayShieldStyle(const SegmentAttributes& way,
                           size_t level,
                           ShieldStyleRef& shieldStyle) const;

    void GetAreaFillStyle(const SegmentAttributes& area,
                          size_t level,
                          FillStyleRef& fillStyle) const;
    void GetAreaTextStyle(const SegmentAttributes& area,
                          size_t level,
                          TextStyleRef& textStyle) const;
    void GetAreaIconStyle(const SegmentAttributes& area,
                          size_t level,
                          IconStyleRef& iconStyle) const;

    void GetLandFillStyle(size_t level,
                          FillStyleRef& fillStyle) const;
    void GetSeaFillStyle(size_t level,
                         FillStyleRef& fillStyle) const;
    void GetCoastFillStyle(size_t level,
                           FillStyleRef& fillStyle) const;
    void GetUnknownFillStyle(size_t level,
                             FillStyleRef& fillStyle) const;
    void GetCoastlineLineStyle(size_t level,
                               LineStyleRef& lineStyle) const;
  };
}

#endif
