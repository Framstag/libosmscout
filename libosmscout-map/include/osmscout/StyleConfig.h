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

#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeSet.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/Reference.h>

namespace osmscout {

  /**
   * Ways can have a line style
   */
  class OSMSCOUT_MAP_API LineStyle : public Referencable
  {
  private:
    Color               lineColor;
    Color               alternateColor;
    Color               outlineColor;
    Color               gapColor;
    double              minWidth;
    double              width;
    double              fixedWidth;
    double              outline;
    std::vector<double> dash;

  public:
    LineStyle();

    LineStyle& SetLineColor(const Color& color);
    LineStyle& SetAlternateColor(const Color& color);
    LineStyle& SetOutlineColor(const Color& color);
    LineStyle& SetGapColor(const Color& color);
    LineStyle& SetMinWidth(double value);
    LineStyle& SetWidth(double value);
    LineStyle& SetFixedWidth(bool fixedWidth);
    LineStyle& SetOutline(double value);
    LineStyle& AddDashValue(double dashValue);

    inline bool IsVisible() const
    {
      return width>0.0;
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

    inline double GetMinWidth() const
    {
      return minWidth;
    }

    inline double GetWidth() const
    {
      return width;
    }

    inline bool GetFixedWidth() const
    {
      return fixedWidth;
    }

    inline double GetOutline() const
    {
      return outline;
    }

    inline bool HasDashValues() const
    {
      return dash.size()>0;
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
  public:
    enum Style {
      none,
      plain
    };

  private:
    Style               style;
    Color               fillColor;
    std::string         pattern;
    mutable size_t      patternId;
    Mag                 patternMinMag;
    Color               borderColor;
    double              borderWidth;
    std::vector<double> borderDash;

  public:
    FillStyle();

    FillStyle& SetStyle(Style style);
    FillStyle& SetFillColor(const Color& color);
    void SetPatternId(size_t id) const;
    FillStyle& SetPattern(const std::string& pattern);
    FillStyle& SetPatternMinMag(Mag mag);
    FillStyle& SetBorderColor(const Color& color);
    FillStyle& SetBorderWidth(double value);
    FillStyle& AddBorderDashValue(double dashValue);

    inline bool IsVisible() const
    {
      return style!=none;
    }

    inline const Style& GetStyle() const
    {
      return style;
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

    inline bool HasBorderDashValues() const
    {
      return borderDash.size()>0;
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
    Mag     minMag;
    Mag     scaleAndFadeMag;
    Mag     maxMag;
    double  size;
    Color   textColor;
    Color   bgColor;
    Color   borderColor;

  public:
    LabelStyle();

    LabelStyle& SetStyle(Style style);
    LabelStyle& SetPriority(uint8_t priority);
    LabelStyle& SetMinMag(Mag mag);
    LabelStyle& SetScaleAndFadeMag(Mag mag);
    LabelStyle& SetMaxMag(Mag mag);
    LabelStyle& SetSize(double size);
    LabelStyle& SetTextColor(const Color& color);
    LabelStyle& SetBgColor(const Color& color);
    LabelStyle& SetBorderColor(const Color& color);

    inline bool IsVisible() const
    {
      return style!=none;
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

    inline Mag GetMinMag() const
    {
      return minMag;
    }

    inline Mag GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    inline Mag GetMaxMag() const
    {
      return maxMag;
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

  /**
    Nodes and areas can have a symbol style.A symbol is a internal predefined simple
    iconic image, most of the time simple geometric forms lice circles, crosses and
    similar.
   */
  class OSMSCOUT_MAP_API SymbolStyle : public Referencable
  {
  public:
    enum Style {
      none,
      box,
      circle,
      triangle
    };

  private:
    Style  style;
    Mag    minMag;
    double size;
    Color  fillColor;

  public:
    SymbolStyle();

    SymbolStyle& SetStyle(Style style);
    SymbolStyle& SetMinMag(Mag mag);
    SymbolStyle& SetSize(double size);
    SymbolStyle& SetFillColor(const Color& color);

    inline bool IsVisible() const
    {
      return style!=none;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline const Mag& GetMinMag() const
    {
      return minMag;
    }

    inline double GetSize() const
    {
      return size;
    }

    inline const Color& GetFillColor() const
    {
      return fillColor;
    }
  };

  typedef Ref<SymbolStyle> SymbolStyleRef;

  /**
    IconStyle is for define drawing of external images as icons for nodes and areas
    */
  class OSMSCOUT_MAP_API IconStyle : public Referencable
  {
  private:
    size_t      id;       //! Internal id for fast lookup. 0 == no id defined (yet), max(size_t) == error
    std::string iconName; //! name of the icon as given in style
    Mag         minMag;   //! minimum magnification to show icon

  public:
    IconStyle();

    IconStyle& SetId(size_t id);
    IconStyle& SetIconName(const std::string& iconName);
    IconStyle& SetMinMag(Mag mag);

    inline bool IsVisible() const
    {
      return !iconName.empty();
    }

    inline size_t GetId() const
    {
      return id;
    }

    inline std::string GetIconName() const
    {
      return iconName;
    }

    inline const Mag& GetMinMag() const
    {
      return minMag;
    }
  };

  typedef Ref<IconStyle> IconStyleRef;

  /**
   * A complete style definition
   */
  class OSMSCOUT_MAP_API StyleConfig
  {
  private:
    TypeConfig                   *typeConfig;

    // Node

    std::vector<SymbolStyleRef>  nodeSymbolStyles;
    std::vector<LabelStyleRef>   nodeRefLabelStyles;
    std::vector<LabelStyleRef>   nodeLabelStyles;
    std::vector<IconStyleRef>    nodeIconStyles;

    std::vector<TypeSet>         nodeTypeSets;

    // Way

    std::vector<LineStyleRef>    wayLineStyles;
    std::vector<LabelStyleRef>   wayRefLabelStyles;
    std::vector<LabelStyleRef>   wayNameLabelStyles;

    std::vector<size_t>          wayPrio;
    std::vector<Mag>             wayMag;

    std::vector<std::vector<TypeSet> > wayTypeSets;

    // Area

    std::vector<FillStyleRef>    areaFillStyles;
    std::vector<SymbolStyleRef>  areaSymbolStyles;
    std::vector<LabelStyleRef>   areaLabelStyles;
    std::vector<IconStyleRef>    areaIconStyles;

    std::vector<Mag>             areaMag;
    std::vector<TypeSet>         areaTypeSets;


  private:
    void ReserveSpaceForNodeType(TypeId type);
    void ReserveSpaceForWayType(TypeId type);
    void ReserveSpaceForAreaType(TypeId type);

    void PostprocessNodes();
    void PostprocessWays();
    void PostprocessAreas();

  public:
    StyleConfig(TypeConfig* typeConfig);
    virtual ~StyleConfig();

    void Postprocess();

    TypeConfig* GetTypeConfig() const;


    StyleConfig& SetNodeSymbolStyle(TypeId type, const SymbolStyle& style);
    StyleConfig& SetNodeRefLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetNodeLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetNodeIconStyle(TypeId type, const IconStyle& style);

    StyleConfig& SetWayPrio(TypeId type, size_t prio);
    StyleConfig& SetWayMag(TypeId type, Mag mag);
    StyleConfig& SetWayLineStyle(TypeId type, const LineStyle& style);
    StyleConfig& SetWayRefLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetWayNameLabelStyle(TypeId type, const LabelStyle& style);

    StyleConfig& SetAreaMag(TypeId type, Mag mag);
    StyleConfig& SetAreaFillStyle(TypeId type, const FillStyle& style);
    StyleConfig& SetAreaLabelStyle(TypeId type, const LabelStyle& style);
    StyleConfig& SetAreaSymbolStyle(TypeId type, const SymbolStyle& style);
    StyleConfig& SetAreaIconStyle(TypeId type, const IconStyle& style);

    void GetNodeTypesWithMaxMag(double maxMag,
                                TypeSet& types) const;
    void GetWayTypesByPrioWithMaxMag(double mag,
                                     std::vector<TypeSet>& types) const;
    void GetAreaTypesWithMaxMag(double maxMag,
                                TypeSet& types) const;


    inline size_t GetWayPrio(TypeId type) const
    {
      return wayPrio[type];
    }

    inline const SymbolStyle* GetNodeSymbolStyle(TypeId type) const
    {
      if (type<nodeSymbolStyles.size()) {
        return nodeSymbolStyles[type];
      }
      else {
        return NULL;
      }
    }

    inline IconStyle* GetNodeIconStyle(TypeId type) const
    {
      if (type<nodeIconStyles.size()) {
        return nodeIconStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetNodeRefLabelStyle(TypeId type) const
    {
      if (type<nodeRefLabelStyles.size()) {
        return nodeRefLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetNodeLabelStyle(TypeId type) const
    {
      if (type<nodeLabelStyles.size()) {
        return nodeLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LineStyle* GetWayLineStyle(TypeId type) const
    {
      if (type<wayLineStyles.size()) {
        return wayLineStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetWayRefLabelStyle(TypeId type) const
    {
      if (type<wayRefLabelStyles.size()) {
        return wayRefLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetWayNameLabelStyle(TypeId type) const
    {
      if (type<wayNameLabelStyles.size()) {
        return wayNameLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const FillStyle* GetAreaFillStyle(TypeId type) const
    {
      if (type<areaFillStyles.size()) {
        return areaFillStyles[type];
      }
      else {
        return NULL;
      }
    }

    const LabelStyle* GetAreaLabelStyle(TypeId type) const
    {
      if (type<areaLabelStyles.size()) {
        return areaLabelStyles[type];
      }
      else {
        return NULL;
      }
    }

    const SymbolStyle* GetAreaSymbolStyle(TypeId type) const
    {
      if (type<areaSymbolStyles.size()) {
        return areaSymbolStyles[type];
      }
      else {
        return NULL;
      }
    }

    inline IconStyle* GetAreaIconStyle(TypeId type) const
    {
      if (type<areaIconStyles.size()) {
        return areaIconStyles[type];
      }
      else {
        return NULL;
      }
    }

  };
}

#endif
