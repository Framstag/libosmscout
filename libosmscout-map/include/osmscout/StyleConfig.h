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
#include <memory>
#include <unordered_map>
#include <vector>

#include <osmscout/private/MapImportExport.h>

#include <osmscout/Pixel.h>

#include <osmscout/Types.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/MapParameter.h>

namespace osmscout {

  /**
   * \ingroup Stylesheet
   *
   * Interface one must implement to provider a label for the map.
   */
  class OSMSCOUT_MAP_API LabelProvider
  {
  public:
    virtual ~LabelProvider();

    /**
     * Returns the label based on the given feature value buffer
     *
     * @param buffer
     *    The FeatureValueBuffer instance
     * @return
     *    The label, if the given feature has a value and a label or a empty string
     */
    virtual std::string GetLabel(const MapParameter& parameter,
                                 const FeatureValueBuffer& buffer) const = 0;

    /**
     * Returns the name of the label provider as it must get stated in the style sheet
     */
    virtual std::string GetName() const = 0;
  };

  typedef std::shared_ptr<LabelProvider> LabelProviderRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API LabelProviderFactory
  {
  public:
    virtual ~LabelProviderFactory();

    virtual LabelProviderRef Create(const TypeConfig& typeConfig) const = 0;
  };

  typedef std::shared_ptr<LabelProviderFactory> LabelProviderFactoryRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API INameLabelProviderFactory : public LabelProviderFactory
  {
  private:
    class INameLabelProvider : public LabelProvider
    {
    private:
      std::vector<size_t> nameLookupTable;
      std::vector<size_t> nameAltLookupTable;

    public:
      INameLabelProvider(const TypeConfig& typeConfig);

      std::string GetLabel(const MapParameter& parameter,
                           const FeatureValueBuffer& buffer) const;

      inline std::string GetName() const
      {
        return "IName";
      }
    };

    private:
      mutable LabelProviderRef instance;

    public:
      LabelProviderRef Create(const TypeConfig& typeConfig) const;
  };

  /**
   * \ingroup Stylesheet
   *
   * Generates a label based on a given feature name and label name.
   *
   * Example:
   *   Give me the label "inMeter" of the Ele-Feature.
   */
  class OSMSCOUT_MAP_API DynamicFeatureLabelReader : public LabelProvider
  {

  private:
    std::vector<size_t> lookupTable;
    std::string         featureName;
    std::string         labelName;
    size_t              labelIndex;

  public:
    /**
     * Assigns a label to the reader
     *
     * @param typeConfig
     *   Reference to the current type configuration
     * @param featureName
     *   Name of the feature which must be valid and must support labels
     * @param labelIndex
     *   The index of the labels to use (a feature might support multiple labels)
     */
    DynamicFeatureLabelReader(const TypeConfig& typeConfig,
                              const std::string& featureName,
                              const std::string& labelName);

    std::string GetLabel(const MapParameter& parameter,
                         const FeatureValueBuffer& buffer) const;

    inline std::string GetName() const
    {
      return featureName + "." + labelName;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleResolveContext
  {
  private:
    BridgeFeatureReader      bridgeReader;
    TunnelFeatureReader      tunnelReader;
    AccessFeatureValueReader accessReader;

  public:
    StyleResolveContext(const TypeConfigRef& typeConfig);

    inline bool IsBridge(const FeatureValueBuffer& buffer) const
    {
      return bridgeReader.IsSet(buffer);
    }

    inline bool IsTunnel(const FeatureValueBuffer& buffer) const
    {
      return tunnelReader.IsSet(buffer);
    }

    bool IsOneway(const FeatureValueBuffer& buffer) const;
  };

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleConstant
  {
  public:
    StyleConstant();
    virtual ~StyleConstant();
  };

  typedef std::shared_ptr<StyleConstant> StyleConstantRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleConstantColor : public StyleConstant
  {
  private:
    Color color;

  public:
    StyleConstantColor(const Color& color);

    inline const Color& GetColor()
    {
      return color;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleConstantMag : public StyleConstant
  {
  private:
    Magnification magnification;

  public:
    StyleConstantMag(Magnification& magnification);

    inline const Magnification& GetMag()
    {
      return magnification;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleConstantUInt : public StyleConstant
  {
  private:
    size_t value;

  public:
    StyleConstantUInt(size_t& value);

    inline const size_t& GetUInt()
    {
      return value;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API SizeCondition
  {
  private:
    double minMM;
    bool minMMSet;

    double minPx;
    bool minPxSet;

    double maxMM;
    bool maxMMSet;

    double maxPx;
    bool maxPxSet;
  public:
    SizeCondition();
    virtual ~SizeCondition();

    void SetMinMM(double minMM);
    void SetMinPx(double minPx);

    void SetMaxMM(double maxMM);
    void SetMaxPx(double maxPx);

    bool Evaluate(double meterInPixel, double meterInMM) const;
  };

  typedef std::shared_ptr<SizeCondition> SizeConditionRef;

  /**
   * \ingroup Stylesheet
   *
   * Holds the all accumulated filter conditions as defined in the style sheet
   * for a style.
   */
  class OSMSCOUT_MAP_API StyleFilter
  {
  public:

  private:
    TypeInfoSet           types;
    size_t                minLevel;
    size_t                maxLevel;
    bool                  bridge;
    bool                  tunnel;
    bool                  oneway;
    SizeConditionRef      sizeCondition;

  public:
    StyleFilter();
    StyleFilter(const StyleFilter& other);

    StyleFilter& SetTypes(const TypeInfoSet& types);

    StyleFilter& SetMinLevel(size_t level);
    StyleFilter& SetMaxLevel(size_t level);
    StyleFilter& SetBridge(bool bridge);
    StyleFilter& SetTunnel(bool tunnel);
    StyleFilter& SetOneway(bool oneway);

    StyleFilter& SetSizeCondition(const SizeConditionRef& condition);

    inline bool HasTypes() const
    {
      return !types.Empty();
    }

    inline bool HasType(const TypeInfoRef& type) const
    {
      return types.IsSet(type);
    }

    inline size_t GetMinLevel() const
    {
      return minLevel;
    }

    inline size_t GetMaxLevel() const
    {
      return maxLevel;
    }

    inline bool GetBridge() const
    {
      return bridge;
    }

    inline bool GetTunnel() const
    {
      return tunnel;
    }

    inline bool GetOneway() const
    {
      return oneway;
    }

    inline bool HasMaxLevel() const
    {
      return maxLevel!=std::numeric_limits<size_t>::max();
    }

    inline const SizeConditionRef& GetSizeCondition() const
    {
      return sizeCondition;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Holds all filter criteria (minus type and zoomlevel criteria which are
   * directly handled by the lookup table) for a concrete style which have to
   * evaluated during runtime.
   */
  class OSMSCOUT_MAP_API StyleCriteria
  {
  public:

  private:
    bool             bridge;
    bool             tunnel;
    bool             oneway;
    SizeConditionRef sizeCondition;

  public:
    StyleCriteria();
    StyleCriteria(const StyleFilter& other);
    StyleCriteria(const StyleCriteria& other);

    bool operator==(const StyleCriteria& other) const;
    bool operator!=(const StyleCriteria& other) const;

    inline bool HasCriteria() const
    {
      return bridge ||
             tunnel ||
             oneway ||
             sizeCondition;
    }

    inline bool GetBridge() const
    {
      return bridge;
    }

    inline bool GetTunnel() const
    {
      return tunnel;
    }

    inline bool GetOneway() const
    {
      return oneway;
    }

    bool Matches(double meterInPixel,
                 double meterInMM) const;
    bool Matches(const StyleResolveContext& context,
                 const FeatureValueBuffer& buffer,
                 double meterInPixel,
                 double meterInMM) const;
  };

  /**
   * \ingroup Stylesheet
   * A Style together with a set of the attributes that are explicitly
   * set in the stye.
   */
  template<class S, class A>
  struct PartialStyle
  {
    std::set<A>        attributes;
    std::shared_ptr<S> style;

    PartialStyle()
    : style(new S())
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * A line in a style sheet. Connecting a set of filter criteria together with
   * a partial style definition.
   */
  template<class S, class A>
  struct ConditionalStyle
  {
    StyleFilter       filter;
    PartialStyle<S,A> style;

    ConditionalStyle(const StyleFilter& filter,
                     const PartialStyle<S,A>& style)
    : filter(filter),
      style(style)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Correlation of a StyleFilter and a PartialStyle. For an object
   * (node, way, area) all ConditionalStyle styles matching the criteria
   * are summed up to build the final style attribute set.
   */
  template<class S, class A>
  struct StyleSelector
  {
    StyleCriteria      criteria;
    std::set<A>        attributes;
    std::shared_ptr<S> style;

    StyleSelector(const StyleFilter& filter,
                  const PartialStyle<S,A>& style)
    : criteria(filter),
      attributes(style.attributes),
      style(style.style)
    {
      // no code
    }
  };

  /**
   * \ingroup Stylesheet
   *
   * Style options for a line.
   */
  class OSMSCOUT_MAP_API LineStyle
  {
  public:
    enum CapStyle {
      capButt,
      capRound,
      capSquare
    };

    enum Attribute {
      attrLineColor,
      attrGapColor,
      attrDisplayWidth,
      attrWidth,
      attrDisplayOffset,
      attrOffset,
      attrJoinCap,
      attrEndCap,
      attrDashes,
      attrPriority,
      attrZIndex
    };

  private:
    std::string         slot;
    Color               lineColor;
    Color               gapColor;
    double              displayWidth;
    double              width;
    double              displayOffset;
    double              offset;
    CapStyle            joinCap;
    CapStyle            endCap;
    std::vector<double> dash;
    int                 priority;
    int                 zIndex;

  public:
    LineStyle();
    LineStyle(const LineStyle& style);

    LineStyle& SetSlot(const std::string& slot);

    LineStyle& SetLineColor(const Color& color);
    LineStyle& SetGapColor(const Color& color);
    LineStyle& SetDisplayWidth(double value);
    LineStyle& SetWidth(double value);
    LineStyle& SetDisplayOffset(double value);
    LineStyle& SetOffset(double value);
    LineStyle& SetJoinCap(CapStyle joinCap);
    LineStyle& SetEndCap(CapStyle endCap);
    LineStyle& SetDashes(const std::vector<double> dashes);
    LineStyle& SetPriority(int priority);
    LineStyle& SetZIndex(int zIndex);

    inline bool IsVisible() const
    {
      return (displayWidth>0.0 ||
              width>0.0) &&
              lineColor.IsVisible();
    }

    inline const std::string& GetSlot() const
    {
      return slot;
    }

    inline const Color& GetLineColor() const
    {
      return lineColor;
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

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    inline CapStyle GetJoinCap() const
    {
      return joinCap;
    }

    inline CapStyle GetEndCap() const
    {
      return endCap;
    }

    inline bool HasDashes() const
    {
      return !dash.empty();
    }

    inline const std::vector<double>& GetDash() const
    {
      return dash;
    }

    inline int GetPriority() const
    {
      return priority;
    }

    inline int GetZIndex() const
    {
      return zIndex;
    }

    void CopyAttributes(const LineStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const LineStyle& other) const;
    bool operator!=(const LineStyle& other) const;
    bool operator<(const LineStyle& other) const;
  };

  typedef std::shared_ptr<LineStyle>                       LineStyleRef;
  typedef PartialStyle<LineStyle,LineStyle::Attribute>     LinePartialStyle;
  typedef ConditionalStyle<LineStyle,LineStyle::Attribute> LineConditionalStyle;
  typedef StyleSelector<LineStyle,LineStyle::Attribute>    LineStyleSelector;
  typedef std::list<LineStyleSelector>                     LineStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<LineStyleSelectorList> > LineStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * Style options for filling an area.
   */
  class OSMSCOUT_MAP_API FillStyle
  {
  public:
    enum Attribute {
      attrFillColor,
      attrPattern,
      attrPatternMinMag
    };

  private:
    Color               fillColor;
    std::string         pattern;
    mutable size_t      patternId;
    Magnification       patternMinMag;

  public:
    FillStyle();
    FillStyle(const FillStyle& style);

    FillStyle& SetFillColor(const Color& color);
    FillStyle& SetPattern(const std::string& pattern);
    void SetPatternId(size_t id) const;
    FillStyle& SetPatternMinMag(const Magnification& mag);

    inline bool IsVisible() const
    {
      return (fillColor.IsVisible() ||
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

    inline std::string GetPatternName() const
    {
      return pattern;
    }

    inline size_t GetPatternId() const
    {
      return patternId;
    }

    inline const Magnification& GetPatternMinMag() const
    {
      return patternMinMag;
    }

    void CopyAttributes(const FillStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const FillStyle& other) const;
    bool operator!=(const FillStyle& other) const;
    bool operator<(const FillStyle& other) const;
  };

  typedef std::shared_ptr<FillStyle>                       FillStyleRef;
  typedef PartialStyle<FillStyle,FillStyle::Attribute>     FillPartialStyle;
  typedef ConditionalStyle<FillStyle,FillStyle::Attribute> FillConditionalStyle;
  typedef StyleSelector<FillStyle,FillStyle::Attribute>    FillStyleSelector;
  typedef std::list<FillStyleSelector>                     FillStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<FillStyleSelectorList> > FillStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * Style options for borders around an area.
   */
  class OSMSCOUT_MAP_API BorderStyle
  {
  public:
    enum Attribute {
      attrColor,
      attrWidth,
      attrDashes,
      attrDisplayOffset,
      attrOffset,
      attrPriority
    };

  private:
    std::string         slot;
    Color               color;
    double              width;
    std::vector<double> dash;
    double              displayOffset;
    double              offset;
    int                 priority;

  public:
    BorderStyle();
    BorderStyle(const BorderStyle& style);

    BorderStyle& SetSlot(const std::string& slot);

    BorderStyle& SetColor(const Color& color);
    BorderStyle& SetWidth(double value);
    BorderStyle& SetDashes(const std::vector<double> dashes);
    BorderStyle& SetDisplayOffset(double value);
    BorderStyle& SetOffset(double value);
    BorderStyle& SetPriority(int priority);

    inline bool IsVisible() const
    {
      return width>0 && color.IsVisible();
    }

    inline const std::string& GetSlot() const
    {
      return slot;
    }

    inline const Color& GetColor() const
    {
      return color;
    }

    inline double GetWidth() const
    {
      return width;
    }

    inline bool HasDashes() const
    {
      return !dash.empty();
    }

    inline const std::vector<double>& GetDash() const
    {
      return dash;
    }

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    inline int GetPriority() const
    {
      return priority;
    }

    void CopyAttributes(const BorderStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const BorderStyle& other) const;
    bool operator!=(const BorderStyle& other) const;
    bool operator<(const BorderStyle& other) const;
  };

  typedef std::shared_ptr<BorderStyle>                         BorderStyleRef;
  typedef PartialStyle<BorderStyle,BorderStyle::Attribute>     BorderPartialStyle;
  typedef ConditionalStyle<BorderStyle,BorderStyle::Attribute> BorderConditionalStyle;
  typedef StyleSelector<BorderStyle,BorderStyle::Attribute>    BorderStyleSelector;
  typedef std::list<BorderStyleSelector>                       BorderStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<BorderStyleSelectorList> >   BorderStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * Abstract base class for all (point) labels. All point labels have priority
   * and a alpha value.
   */
  class OSMSCOUT_MAP_API LabelStyle
  {
  private:
    size_t priority;
    double size;

  public:
    LabelStyle();
    LabelStyle(const LabelStyle& style);
    virtual ~LabelStyle();

    virtual bool IsVisible() const = 0;
    virtual double GetAlpha() const = 0;

    LabelStyle& SetPriority(size_t priority);
    LabelStyle& SetSize(double size);

    inline size_t GetPriority() const
    {
      return priority;
    }

    inline double GetSize() const
    {
      return size;
    }
  };

  typedef std::shared_ptr<LabelStyle> LabelStyleRef;

  /**
   * \ingroup Stylesheet
   *
   * A textual label.
   */
  class OSMSCOUT_MAP_API TextStyle : public LabelStyle
  {
  public:
    enum Style {
      normal,
      emphasize
    };

    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrPosition,
      attrTextColor,
      attrStyle,
      attrScaleAndFadeMag,
      attrAutoSize
    };

  private:
    std::string      slot;
    LabelProviderRef label;           //!< The label - a reference to a feature and its label index
    size_t           position;        //!< Relative vertical position of the label
    Color            textColor;       //!< Color of text
    Style            style;           //!< Style of the text
    Magnification    scaleAndFadeMag; //!< Automatic pseudo-autoSize scaling for nodes
    bool             autoSize;        //!< Calculate the size of the label base don the height of the area

  public:
    TextStyle();
    TextStyle(const TextStyle& style);

    TextStyle& SetSlot(const std::string& slot);

    TextStyle& SetPriority(uint8_t priority);
    TextStyle& SetSize(double size);
    TextStyle& SetLabel(const LabelProviderRef& label);
    TextStyle& SetPosition(size_t position);
    TextStyle& SetTextColor(const Color& color);
    TextStyle& SetStyle(Style style);
    TextStyle& SetScaleAndFadeMag(const Magnification& mag);
    TextStyle& SetAutoSize(bool autoSize);

    inline bool IsVisible() const
    {
      return label &&
             GetTextColor().IsVisible();
    }

    inline double GetAlpha() const
    {
      return textColor.GetA();
    }

    inline const std::string& GetSlot() const
    {
      return slot;
    }

    inline const LabelProviderRef& GetLabel() const
    {
      return label;
    }

    inline size_t GetPosition() const
    {
      return position;
    }

    inline const Color& GetTextColor() const
    {
      return textColor;
    }

    inline const Style& GetStyle() const
    {
      return style;
    }

    inline Magnification GetScaleAndFadeMag() const
    {
      return scaleAndFadeMag;
    }

    inline bool GetAutoSize() const
    {
      return autoSize;
    }

    void CopyAttributes(const TextStyle& other,
                        const std::set<Attribute>& attributes);

    bool operator==(const TextStyle& other) const;
    bool operator!=(const TextStyle& other) const;
    bool operator<(const TextStyle& other) const;
  };

  typedef std::shared_ptr<TextStyle>                       TextStyleRef;
  typedef PartialStyle<TextStyle,TextStyle::Attribute>     TextPartialStyle;
  typedef ConditionalStyle<TextStyle,TextStyle::Attribute> TextConditionalStyle;
  typedef StyleSelector<TextStyle,TextStyle::Attribute>    TextStyleSelector;
  typedef std::list<TextStyleSelector>                     TextStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<TextStyleSelectorList> > TextStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * A shield or plate label (text placed on a plate).
   */
  class OSMSCOUT_MAP_API ShieldStyle : public LabelStyle
  {
  public:
    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrTextColor,
      attrBgColor,
      attrBorderColor
    };

  private:
    LabelProviderRef label;          //!< The label - a reference to a feature and its label index
    Color            textColor;      //!< Color of the text
    Color            bgColor;        //!< Background of the text
    Color            borderColor;    //!< Color of the border

  public:
    ShieldStyle();
    ShieldStyle(const ShieldStyle& style);

    ShieldStyle& SetLabel(const LabelProviderRef& label);
    ShieldStyle& SetPriority(uint8_t priority);
    ShieldStyle& SetSize(double size);
    ShieldStyle& SetTextColor(const Color& color);
    ShieldStyle& SetBgColor(const Color& color);
    ShieldStyle& SetBorderColor(const Color& color);

    inline bool IsVisible() const
    {
      return label &&
             GetTextColor().IsVisible();
    }

    inline double GetAlpha() const
    {
      return textColor.GetA();
    }

    inline const LabelProviderRef& GetLabel() const
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

  typedef std::shared_ptr<ShieldStyle>                         ShieldStyleRef;
  typedef PartialStyle<ShieldStyle,ShieldStyle::Attribute>     ShieldPartialStyle;
  typedef ConditionalStyle<ShieldStyle,ShieldStyle::Attribute> ShieldConditionalStyle;
  typedef StyleSelector<ShieldStyle,ShieldStyle::Attribute>    ShieldStyleSelector;
  typedef std::list<ShieldStyleSelector>                       ShieldStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<ShieldStyleSelectorList> >   ShieldStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * A style defining repretive drawing of a shield label along a path. It consists
   * mainly of the attributes of the shield itself (it internally holds a shield
   * label for this) and some more attributes defining the way of repetition.
   */
  class OSMSCOUT_MAP_API PathShieldStyle
  {
  public:
    enum Attribute {
      attrPriority,
      attrSize,
      attrLabel,
      attrTextColor,
      attrBgColor,
      attrBorderColor,
      attrShieldSpace
    };

  private:
    ShieldStyleRef shieldStyle;
    double         shieldSpace;

  public:
    PathShieldStyle();
    PathShieldStyle(const PathShieldStyle& style);

    PathShieldStyle& SetLabel(const LabelProviderRef& label);
    PathShieldStyle& SetPriority(uint8_t priority);
    PathShieldStyle& SetSize(double size);
    PathShieldStyle& SetTextColor(const Color& color);
    PathShieldStyle& SetBgColor(const Color& color);
    PathShieldStyle& SetBorderColor(const Color& color);
    PathShieldStyle& SetShieldSpace(double shieldSpace);

    inline bool IsVisible() const
    {
      return shieldStyle->IsVisible();
    }

    inline double GetAlpha() const
    {
      return shieldStyle->GetAlpha();
    }

    inline uint8_t GetPriority() const
    {
      return shieldStyle->GetPriority();
    }

    inline double GetSize() const
    {
      return shieldStyle->GetSize();
    }

    inline const LabelProviderRef& GetLabel() const
    {
      return shieldStyle->GetLabel();
    }

    inline const Color& GetTextColor() const
    {
      return shieldStyle->GetTextColor();
    }

    inline const Color& GetBgColor() const
    {
      return shieldStyle->GetBgColor();
    }

    inline const Color& GetBorderColor() const
    {
      return shieldStyle->GetBorderColor();
    }

    inline double GetShieldSpace() const
    {
      return shieldSpace;
    }

    inline const ShieldStyleRef& GetShieldStyle() const
    {
      return shieldStyle;
    }

    void CopyAttributes(const PathShieldStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<PathShieldStyle>                             PathShieldStyleRef;
  typedef PartialStyle<PathShieldStyle,PathShieldStyle::Attribute>     PathShieldPartialStyle;
  typedef ConditionalStyle<PathShieldStyle,PathShieldStyle::Attribute> PathShieldConditionalStyle;
  typedef StyleSelector<PathShieldStyle,PathShieldStyle::Attribute>    PathShieldStyleSelector;
  typedef std::list<PathShieldStyleSelector>                           PathShieldStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<PathShieldStyleSelectorList> >       PathShieldStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * A style for drawing text onto a path, the text following the
   * contour of the path.
   */
  class OSMSCOUT_MAP_API PathTextStyle
  {
  public:
    enum Attribute {
      attrLabel,
      attrSize,
      attrTextColor,
      attrDisplayOffset,
      attrOffset
    };

  private:
    LabelProviderRef label;
    double           size;
    Color            textColor;
    double           displayOffset;
    double           offset;

  public:
    PathTextStyle();
    PathTextStyle(const PathTextStyle& style);

    PathTextStyle& SetLabel(const LabelProviderRef& label);
    PathTextStyle& SetSize(double size);
    PathTextStyle& SetTextColor(const Color& color);
    PathTextStyle& SetDisplayOffset(double value);
    PathTextStyle& SetOffset(double value);

    inline bool IsVisible() const
    {
      return label &&
             textColor.IsVisible();
    }

    inline const LabelProviderRef& GetLabel() const
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

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    void CopyAttributes(const PathTextStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<PathTextStyle>                           PathTextStyleRef;
  typedef PartialStyle<PathTextStyle,PathTextStyle::Attribute>     PathTextPartialStyle;
  typedef ConditionalStyle<PathTextStyle,PathTextStyle::Attribute> PathTextConditionalStyle;
  typedef StyleSelector<PathTextStyle,PathTextStyle::Attribute>    PathTextStyleSelector;
  typedef std::list<PathTextStyleSelector>                         PathTextStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<PathTextStyleSelectorList> >     PathTextStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API DrawPrimitive
  {
  private:
    FillStyleRef   fillStyle;
    BorderStyleRef borderStyle;

  public:
    DrawPrimitive(const FillStyleRef& fillStyle,
                  const BorderStyleRef& borderStyle);
    virtual ~DrawPrimitive();

    inline const FillStyleRef& GetFillStyle() const
    {
      return fillStyle;
    }

    inline const BorderStyleRef& GetBorderStyle() const
    {
      return borderStyle;
    }

    virtual void GetBoundingBox(double& minX,
                                double& minY,
                                double& maxX,
                                double& maxY) const = 0;
  };

  typedef std::shared_ptr<DrawPrimitive> DrawPrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API PolygonPrimitive : public DrawPrimitive
  {
  private:
    std::list<Vertex2D> coords;

  public:
    PolygonPrimitive(const FillStyleRef& fillStyle,
                     const BorderStyleRef& borderStyle);

    void AddCoord(const Vertex2D& coord);

    inline const std::list<Vertex2D>& GetCoords() const
    {
      return coords;
    }

    void GetBoundingBox(double& minX,
                        double& minY,
                        double& maxX,
                        double& maxY) const;
  };

  typedef std::shared_ptr<PolygonPrimitive> PolygonPrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API RectanglePrimitive : public DrawPrimitive
  {
  private:
    Vertex2D topLeft;
    double   width;
    double   height;

  public:
    RectanglePrimitive(const Vertex2D& topLeft,
                       double width,
                       double height,
                       const FillStyleRef& fillStyle,
                       const BorderStyleRef& borderStyle);

    inline const Vertex2D& GetTopLeft() const
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

  typedef std::shared_ptr<RectanglePrimitive> RectanglePrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API CirclePrimitive : public DrawPrimitive
  {
  private:
    Vertex2D center;
    double   radius;

  public:
    CirclePrimitive(const Vertex2D& center,
                    double radius,
                    const FillStyleRef& fillStyle,
                    const BorderStyleRef& borderStyle);

    inline const Vertex2D& GetCenter() const
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

  typedef std::shared_ptr<CirclePrimitive> CirclePrimitiveRef;

  /**
   * \ingroup Stylesheet
   *
   * Definition of a symbol. A symbol consists of a list of DrawPrimitives
   * with with assigned rendeirng styes.
   */
  class OSMSCOUT_MAP_API Symbol
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

  typedef std::shared_ptr<Symbol> SymbolRef;

  /**
   * \ingroup Stylesheet
   *
   * The icon style allow the rendering of external images or internal symbols.
   */
  class OSMSCOUT_MAP_API IconStyle
  {
  public:
    enum Attribute {
      attrSymbol,
      attrIconName,
      attrPosition
    };

  private:
    SymbolRef   symbol;
    std::string iconName; //!< name of the icon as given in style
    size_t      iconId;   //!< Id for external resource binding
    size_t      position; //!< Relative vertical position of the label

  public:
    IconStyle();
    IconStyle(const IconStyle& style);

    IconStyle& SetSymbol(const SymbolRef& symbol);
    IconStyle& SetIconName(const std::string& iconName);
    IconStyle& SetIconId(size_t id);
    IconStyle& SetPosition(size_t position);

    inline bool IsVisible() const
    {
      return !iconName.empty() ||
              symbol;
    }

    inline const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    inline std::string GetIconName() const
    {
      return iconName;
    }

    inline size_t GetIconId() const
    {
      return iconId;
    }

    inline size_t GetPosition() const
    {
      return position;
    }

    void CopyAttributes(const IconStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<IconStyle>                       IconStyleRef;
  typedef PartialStyle<IconStyle,IconStyle::Attribute>     IconPartialStyle;
  typedef ConditionalStyle<IconStyle,IconStyle::Attribute> IconConditionalStyle;
  typedef StyleSelector<IconStyle,IconStyle::Attribute>    IconStyleSelector;
  typedef std::list<IconStyleSelector>                     IconStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<IconStyleSelectorList> > IconStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * Style for repetive drawing of symbols on top of a path.
   */
  class OSMSCOUT_MAP_API PathSymbolStyle
  {
  public:

    enum Attribute {
      attrSymbol,
      attrSymbolSpace,
      attrDisplayOffset,
      attrOffset
    };

  private:
    SymbolRef symbol;
    double    symbolSpace;
    double    displayOffset;
    double    offset;

  public:
    PathSymbolStyle();
    PathSymbolStyle(const PathSymbolStyle& style);

    PathSymbolStyle& SetSymbol(const SymbolRef& symbol);
    PathSymbolStyle& SetSymbolSpace(double space);
    PathSymbolStyle& SetDisplayOffset(double value);
    PathSymbolStyle& SetOffset(double value);

    inline bool IsVisible() const
    {
      return (bool)symbol;
    }

    inline const SymbolRef& GetSymbol() const
    {
      return symbol;
    }

    inline double GetSymbolSpace() const
    {
      return symbolSpace;
    }

    inline double GetDisplayOffset() const
    {
      return displayOffset;
    }

    inline double GetOffset() const
    {
      return offset;
    }

    void CopyAttributes(const PathSymbolStyle& other,
                        const std::set<Attribute>& attributes);
  };

  typedef std::shared_ptr<PathSymbolStyle>                             PathSymbolStyleRef;
  typedef PartialStyle<PathSymbolStyle,PathSymbolStyle::Attribute>     PathSymbolPartialStyle;
  typedef ConditionalStyle<PathSymbolStyle,PathSymbolStyle::Attribute> PathSymbolConditionalStyle;
  typedef StyleSelector<PathSymbolStyle,PathSymbolStyle::Attribute>    PathSymbolStyleSelector;
  typedef std::list<PathSymbolStyleSelector>                           PathSymbolStyleSelectorList; //! List of selectors
  typedef std::vector<std::vector<PathSymbolStyleSelectorList> >       PathSymbolStyleLookupTable;  //!Index selectors by type and level

  /**
   * \ingroup Stylesheet
   *
   * A complete style definition
   *
   * Internals:
   * * Fastpath: Fastpath means, that we can directly return the style definition from the style sheet. This is normally
   * the case, if there is excactly one match in the style sheet. If there are multiple matches a new style has to be
   * allocated and composed from all matches.
   */
  class OSMSCOUT_MAP_API StyleConfig
  {
  private:
    TypeConfigRef                              typeConfig;             //!< Reference to the type configuration
    StyleResolveContext                        styleResolveContext;    //!< Instance of helper class that can get passed around to templated helper methods

    FeatureValueBuffer                         tileLandBuffer;         //!< Fake FeatureValueBuffer for land tiles
    FeatureValueBuffer                         tileSeaBuffer;          //!< Fake FeatureValueBuffer for sea tiles
    FeatureValueBuffer                         tileCoastBuffer;        //!< Fake FeatureValueBuffer for coast tiles
    FeatureValueBuffer                         tileUnknownBuffer;      //!< Fake FeatureValueBuffer for unknown tiles
    FeatureValueBuffer                         coastlineBuffer;        //!< Fake FeatureValueBuffer for coastlines
    FeatureValueBuffer                         osmTileBorderBuffer;    //!< Fake FeatureValueBuffer for OSM tile borders
    FeatureValueBuffer                         osmSubTileBorderBuffer; //!< Fake FeatureValueBuffer for OSM tile borders

    std::unordered_map<std::string,LabelProviderFactoryRef> labelFactories; //!< Map of Label Factories

    // Symbol
    std::unordered_map<std::string,SymbolRef>  symbols;                //!< Map of symbols by name
    SymbolRef                                  emptySymbol;            //!< A default empty symbol

    // Node

    std::list<TextConditionalStyle>            nodeTextStyleConditionals;
    std::list<IconConditionalStyle>            nodeIconStyleConditionals;

    std::vector<TextStyleLookupTable>          nodeTextStyleSelectors;
    IconStyleLookupTable                       nodeIconStyleSelectors;

    std::vector<TypeInfoSet>                   nodeTypeSets;

    // Way

    std::vector<size_t>                        wayPrio;

    std::list<LineConditionalStyle>            wayLineStyleConditionals;
    std::list<PathTextConditionalStyle>        wayPathTextStyleConditionals;
    std::list<PathSymbolConditionalStyle>      wayPathSymbolStyleConditionals;
    std::list<PathShieldConditionalStyle>      wayPathShieldStyleConditionals;

    std::vector<LineStyleLookupTable>          wayLineStyleSelectors;
    PathTextStyleLookupTable                   wayPathTextStyleSelectors;
    PathSymbolStyleLookupTable                 wayPathSymbolStyleSelectors;
    PathShieldStyleLookupTable                 wayPathShieldStyleSelectors;

    std::vector<TypeInfoSet>                   wayTypeSets;

    // Area

    std::list<FillConditionalStyle>            areaFillStyleConditionals;
    std::list<BorderConditionalStyle>          areaBorderStyleConditionals;
    std::list<TextConditionalStyle>            areaTextStyleConditionals;
    std::list<IconConditionalStyle>            areaIconStyleConditionals;
    std::list<PathTextConditionalStyle>        areaBorderTextStyleConditionals;
    std::list<PathSymbolConditionalStyle>      areaBorderSymbolStyleConditionals;

    FillStyleLookupTable                       areaFillStyleSelectors;
    std::vector<BorderStyleLookupTable>        areaBorderStyleSelectors;
    std::vector<TextStyleLookupTable>          areaTextStyleSelectors;
    IconStyleLookupTable                       areaIconStyleSelectors;
    PathTextStyleLookupTable                   areaBorderTextStyleSelectors;
    PathSymbolStyleLookupTable                 areaBorderSymbolStyleSelectors;

    std::vector<TypeInfoSet>                   areaTypeSets;

    std::unordered_map<std::string,bool>       flags;
    std::unordered_map<std::string,StyleConstantRef> constants;
    std::list<std::string>                     errors;

  private:
    void Reset();

    void GetAllNodeTypes(std::list<TypeId>& types);
    void GetAllWayTypes(std::list<TypeId>& types);
    void GetAllAreaTypes(std::list<TypeId>& types);

    void PostprocessNodes();
    void PostprocessWays();
    void PostprocessAreas();
    void PostprocessIconId();
    void PostprocessPatternId();

  public:
    StyleConfig(const TypeConfigRef& typeConfig);
    virtual ~StyleConfig();

    /**
     * Methods for registering LabelProvider-factories and and retrieving label providers
     */
    //@{
    bool RegisterLabelProviderFactory(const std::string& name,
                                      const LabelProviderFactoryRef& factory);

    LabelProviderRef GetLabelProvider(const std::string& name) const;
    //@}

    bool HasFlag(const std::string& name) const;
    bool GetFlagByName(const std::string& name) const;
    void AddFlag(const std::string& name,
                 bool value);

    inline const std::unordered_map<std::string,bool> GetFlags() const
    {
      return flags;
    }

    StyleConstantRef GetConstantByName(const std::string& name) const;
    void AddConstant(const std::string& name,
                     const StyleConstantRef& variable);

    bool RegisterSymbol(const SymbolRef& symbol);
    const SymbolRef& GetSymbol(const std::string& name) const;

    void Postprocess();

    TypeConfigRef GetTypeConfig() const;

    StyleConfig& SetWayPrio(const TypeInfoRef& type,
                            size_t prio);

    void AddNodeTextStyle(const StyleFilter& filter,
                          TextPartialStyle& stype);
    void AddNodeIconStyle(const StyleFilter& filter,
                          IconPartialStyle& style);

    void AddWayLineStyle(const StyleFilter& filter,
                         LinePartialStyle& style);
    void AddWayPathTextStyle(const StyleFilter& filter,
                             PathTextPartialStyle& style);
    void AddWayPathSymbolStyle(const StyleFilter& filter,
                               PathSymbolPartialStyle& style);
    void AddWayPathShieldStyle(const StyleFilter& filter,
                               PathShieldPartialStyle& style);

    void AddAreaFillStyle(const StyleFilter& filter,
                          FillPartialStyle& style);
    void AddAreaBorderStyle(const StyleFilter& filter,
                            BorderPartialStyle& style);
    void AddAreaTextStyle(const StyleFilter& filter,
                          TextPartialStyle& style);
    void AddAreaIconStyle(const StyleFilter& filter,
                          IconPartialStyle& style);
    void AddAreaBorderTextStyle(const StyleFilter& filter,
                                PathTextPartialStyle& style);
    void AddAreaBorderSymbolStyle(const StyleFilter& filter,
                                  PathSymbolPartialStyle& style);

    void GetNodeTypesWithMaxMag(const Magnification& maxMag,
                                TypeInfoSet& types) const;
    void GetWayTypesWithMaxMag(const Magnification& mag,
                               TypeInfoSet& types) const;
    void GetAreaTypesWithMaxMag(const Magnification& maxMag,
                                TypeInfoSet& types) const;


    inline size_t GetWayPrio(const TypeInfoRef& type) const
    {
      if (type->GetIndex()<wayPrio.size()) {
        return wayPrio[type->GetIndex()];
      }
      else {
        return std::numeric_limits<size_t>::max();
      }
    }


    /**
     * Methods for retrieval of styles for a given object.
     */
    //@{
    bool HasNodeTextStyles(const TypeInfoRef& type,
                           const Magnification& magnification) const;
    void GetNodeTextStyles(const FeatureValueBuffer& buffer,
                           const Projection& projection,
                           std::vector<TextStyleRef>& textStyles) const;

    void GetNodeIconStyle(const FeatureValueBuffer& buffer,
                          const Projection& projection,
                          IconStyleRef& iconStyle) const;

    void GetWayLineStyles(const FeatureValueBuffer& buffer,
                          const Projection& projection,
                          std::vector<LineStyleRef>& lineStyles) const;
    void GetWayPathTextStyle(const FeatureValueBuffer& buffer,
                             const Projection& projection,
                             PathTextStyleRef& pathTextStyle) const;
    void GetWayPathSymbolStyle(const FeatureValueBuffer& buffer,
                               const Projection& projection,
                               PathSymbolStyleRef& pathSymbolStyle) const;
    void GetWayPathShieldStyle(const FeatureValueBuffer& buffer,
                               const Projection& projection,
                               PathShieldStyleRef& pathShieldStyle) const;

    void GetAreaFillStyle(const TypeInfoRef& type,
                          const FeatureValueBuffer& buffer,
                          const Projection& projection,
                          FillStyleRef& fillStyle) const;
    void GetAreaBorderStyles(const TypeInfoRef& type,
                             const FeatureValueBuffer& buffer,
                             const Projection& projection,
                             std::vector<BorderStyleRef>& borderStyles) const;
    bool HasAreaTextStyles(const TypeInfoRef& type,
                           const Magnification& magnification) const;
    void GetAreaTextStyles(const TypeInfoRef& type,
                           const FeatureValueBuffer& buffer,
                           const Projection& projection,
                           std::vector<TextStyleRef>& textStyles) const;
    void GetAreaIconStyle(const TypeInfoRef& type,
                          const FeatureValueBuffer& buffer,
                          const Projection& projection,
                          IconStyleRef& iconStyle) const;
    void GetAreaBorderTextStyle(const TypeInfoRef& type,
                                const FeatureValueBuffer& buffer,
                                const Projection& projection,
                                PathTextStyleRef& pathTextStyle) const;
    void GetAreaBorderSymbolStyle(const TypeInfoRef& type,
                                  const FeatureValueBuffer& buffer,
                                  const Projection& projection,
                                  PathSymbolStyleRef& pathSymbolStyle) const;

    void GetLandFillStyle(const Projection& projection,
                          FillStyleRef& fillStyle) const;
    void GetSeaFillStyle(const Projection& projection,
                         FillStyleRef& fillStyle) const;
    void GetCoastFillStyle(const Projection& projection,
                           FillStyleRef& fillStyle) const;
    void GetUnknownFillStyle(const Projection& projection,
                             FillStyleRef& fillStyle) const;
    void GetCoastlineLineStyle(const Projection& projection,
                               LineStyleRef& lineStyle) const;
    void GetOSMTileBorderLineStyle(const Projection& projection,
                                   LineStyleRef& lineStyle) const;
    void GetOSMSubTileBorderLineStyle(const Projection& projection,
                                      LineStyleRef& lineStyle) const;
    //@}

    /**
     * Methods for low level debugging access to the style sheet internals
     */
    //@{
    void GetNodeTextStyleSelectors(size_t level,
                                   const TypeInfoRef& type,
                                   std::list<TextStyleSelector>& selectors) const;
    void GetAreaFillStyleSelectors(size_t level,
                                   const TypeInfoRef& type,
                                   std::list<FillStyleSelector>& selectors) const;
    void GetAreaTextStyleSelectors(size_t level,
                                   const TypeInfoRef& type,
                                   std::list<TextStyleSelector>& selectors) const;
    //@}

    /**
     * Methods for loading a concrete OSS style sheet
     */
    //@{
    bool Load(const std::string& styleFile);
    const std::list<std::string>&  GetErrors();
    //@}
  };

  typedef std::shared_ptr<StyleConfig> StyleConfigRef;

  /**
   * \defgroup Stylesheet Stylesheet definition
   *
   * Classes and methods related to stylesheet definition for the renderer.
   */
}

#endif
