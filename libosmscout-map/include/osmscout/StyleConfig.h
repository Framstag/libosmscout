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
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include <osmscout/MapImportExport.h>

#include <osmscout/Pixel.h>

#include <osmscout/OSMScoutTypes.h>
#include <osmscout/TypeConfig.h>
#include <osmscout/TypeFeatures.h>
#include <osmscout/TypeInfoSet.h>
#include <osmscout/FeatureReader.h>

#include <osmscout/Node.h>
#include <osmscout/Area.h>
#include <osmscout/Way.h>

#include <osmscout/util/Color.h>
#include <osmscout/util/Transformation.h>

#include <osmscout/LabelProvider.h>
#include <osmscout/StyleDescription.h>
#include <osmscout/Styles.h>

namespace osmscout {

  using ColorPostprocessor = osmscout::Color (*)(const osmscout::Color &);

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleResolveContext
  {
  private:
    TypeConfigRef                     typeConfig;
    std::map<std::string,size_t>      featureReaderMap; //< Map that maps feature names to index in the feature reader vector
    std::vector<DynamicFeatureReader> featureReaders;   //< List of feature readers
    AccessFeatureValueReader          accessReader;

  public:
    explicit StyleResolveContext(const TypeConfigRef& typeConfig);

    size_t GetFeatureReaderIndex(const Feature& feature);

    inline bool HasFeature(size_t featureIndex,
                           const FeatureValueBuffer& buffer) const
    {
      return featureReaders[featureIndex].IsSet(buffer);
    }

    inline std::string GetFeatureName(size_t featureIndex) const
    {
      return featureReaders[featureIndex].GetFeatureName();
    }

    inline FeatureValue* GetFeatureValue(size_t featureIndex,
                                         const FeatureValueBuffer& buffer) const
    {
      return featureReaders[featureIndex].GetValue(buffer);
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
    StyleConstant() = default;
    virtual ~StyleConstant() = default;
  };

  using StyleConstantRef = std::shared_ptr<StyleConstant>;

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API StyleConstantColor : public StyleConstant
  {
  private:
    Color color;

  public:
    explicit StyleConstantColor(const Color& color);

    inline Color GetColor() const
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
    explicit StyleConstantMag(const Magnification& magnification);

    inline Magnification GetMag() const
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
    explicit StyleConstantUInt(size_t value);

    inline size_t GetUInt() const
    {
      return value;
    }
  };

  /**
 * \ingroup Stylesheet
 *
 */
  class OSMSCOUT_MAP_API StyleConstantWidth : public StyleConstant
  {
  public:
    enum class Unit {
      m,
      mm
    };

  private:
    double value;
    Unit   unit;

  public:
    explicit StyleConstantWidth(double value, Unit unit);

    inline double GetWidth() const
    {
      return value;
    }

    inline Unit GetUnit() const
    {
      return unit;
    }
  };

  /**
   * \ingroup Stylesheet
   *
   */
  class OSMSCOUT_MAP_API SizeCondition CLASS_FINAL
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

    void SetMinMM(double minMM);
    void SetMinPx(double minPx);

    void SetMaxMM(double maxMM);
    void SetMaxPx(double maxPx);

    bool Evaluate(double meterInPixel, double meterInMM) const;
  };

  using SizeConditionRef = std::shared_ptr<SizeCondition>;

  struct OSMSCOUT_MAP_API FeatureFilterData
  {
    size_t featureFilterIndex;
    size_t flagIndex;

    FeatureFilterData(size_t featureFilterIndex,
                      size_t flagIndex);

    inline bool operator==(const FeatureFilterData& other) const
    {
      return featureFilterIndex==other.featureFilterIndex &&
             flagIndex==other.flagIndex;
    }
  };

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
    bool                         filtersByType;
    TypeInfoSet                  types;
    size_t                       minLevel;
    size_t                       maxLevel;
    std::list<FeatureFilterData> features;
    bool                         oneway;
    SizeConditionRef             sizeCondition;

  public:
    StyleFilter();
    StyleFilter(const StyleFilter& other);

    StyleFilter& SetTypes(const TypeInfoSet& types);
    StyleFilter& SetMinLevel(size_t level);
    StyleFilter& SetMaxLevel(size_t level);

    StyleFilter& AddFeature(size_t featureFilterIndex,
                            size_t flagIndex);

    StyleFilter& SetOneway(bool oneway);

    StyleFilter& SetSizeCondition(const SizeConditionRef& condition);

    inline bool FiltersByType() const
    {
      return filtersByType;
    }

    inline bool FiltersByFeature() const
    {
      return !features.empty();
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

    inline const std::list<FeatureFilterData>& GetFeatures() const
    {
      return features;
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
   * Holds all filter criteria (minus type and zoom level criteria which are
   * directly handled by the lookup table) for a concrete style which have to
   * evaluated during runtime.
   */
  class OSMSCOUT_MAP_API StyleCriteria
  {
  public:

  private:
    std::list<FeatureFilterData> features;
    bool                         oneway;
    SizeConditionRef             sizeCondition;

  public:
    StyleCriteria();

    explicit StyleCriteria(const StyleFilter& other);
    StyleCriteria(const StyleCriteria& other);

    bool operator==(const StyleCriteria& other) const;
    bool operator!=(const StyleCriteria& other) const;

    inline bool HasCriteria() const
    {
      return !features.empty() ||
             oneway     ||
             sizeCondition;
    }

    inline bool GetOneway() const
    {
      return oneway;
    }

    bool Matches(const StyleResolveContext& context,
                 const FeatureValueBuffer& buffer,
                 double meterInPixel,
                 double meterInMM) const;
  };

  struct PartialStyleBase
  {
    virtual ~PartialStyleBase() = default;

    virtual void SetBoolValue(int attribute, bool value) = 0;
    virtual void SetStringValue(int attribute, const std::string& value) = 0;
    virtual void SetColorValue(int attribute, const Color& value) = 0;
    virtual void SetMagnificationValue(int attribute, const Magnification& value) = 0;
    virtual void SetDoubleValue(int attribute, double value) = 0;
    virtual void SetDoubleArrayValue(int attribute, const std::vector<double>& value) = 0;
    virtual void SetSymbolValue(int attribute, const SymbolRef& value) = 0;
    virtual void SetIntValue(int attribute, int value) = 0;
    virtual void SetUIntValue(int attribute, size_t value) = 0;
    virtual void SetLabelValue(int attribute, const LabelProviderRef& value) = 0;
  };

  /**
   * \ingroup Stylesheet
   * A Style together with a set of the attributes that are explicitly
   * set in the style.
   */
  template<class S, class A>
  struct PartialStyle : public PartialStyleBase
  {
    std::set<A>        attributes;
    std::shared_ptr<S> style;

    PartialStyle()
    : style(new S())
    {
      // no code
    }

    inline void SetBoolValue(int attribute, bool value) override
    {
      style->SetBoolValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetStringValue(int attribute, const std::string& value) override
    {
      style->SetStringValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetColorValue(int attribute, const Color& value) override
    {
      style->SetColorValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetMagnificationValue(int attribute, const Magnification& value) override
    {
      style->SetMagnificationValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetDoubleValue(int attribute, double value) override
    {
      style->SetDoubleValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetDoubleArrayValue(int attribute, const std::vector<double>& value) override
    {
      style->SetDoubleArrayValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetSymbolValue(int attribute, const SymbolRef& value) override
    {
      style->SetSymbolValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetIntValue(int attribute, int value) override
    {
      style->SetIntValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetUIntValue(int attribute, size_t value) override
    {
      style->SetUIntValue(attribute,value);
      attributes.insert((A)attribute);
    }

    inline void SetLabelValue(int attribute, const LabelProviderRef& value) override
    {
      style->SetLabelValue(attribute,value);
      attributes.insert((A)attribute);
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

  using LinePartialStyle = PartialStyle<LineStyle,LineStyle::Attribute>;
  using LineConditionalStyle = ConditionalStyle<LineStyle,LineStyle::Attribute>;
  using LineStyleSelector = StyleSelector<LineStyle,LineStyle::Attribute>;
  using LineStyleSelectorList = std::list<LineStyleSelector>; //! List of selectors
  using LineStyleLookupTable = std::vector<std::vector<LineStyleSelectorList> >;  //!Index selectors by type and level

  using FillPartialStyle = PartialStyle<FillStyle, FillStyle::Attribute>;
  using FillConditionalStyle = ConditionalStyle<FillStyle, FillStyle::Attribute>;
  using FillStyleSelector = StyleSelector<FillStyle, FillStyle::Attribute>;
  using FillStyleSelectorList = std::list<FillStyleSelector>; //! List of selectors
  using FillStyleLookupTable = std::vector<std::vector<FillStyleSelectorList> >;  //!Index selectors by type and level

  using BorderPartialStyle = PartialStyle<BorderStyle, BorderStyle::Attribute>;
  using BorderConditionalStyle = ConditionalStyle<BorderStyle, BorderStyle::Attribute>;
  using BorderStyleSelector = StyleSelector<BorderStyle, BorderStyle::Attribute>;
  using BorderStyleSelectorList = std::list<BorderStyleSelector>; //! List of selectors
  using BorderStyleLookupTable = std::vector<std::vector<BorderStyleSelectorList> >;  //!Index selectors by type and level

  using TextPartialStyle = PartialStyle<TextStyle, TextStyle::Attribute>;
  using TextConditionalStyle = ConditionalStyle<TextStyle, TextStyle::Attribute>;
  using TextStyleSelector = StyleSelector<TextStyle, TextStyle::Attribute>;
  using TextStyleSelectorList = std::list<TextStyleSelector>; //! List of selectors
  using TextStyleLookupTable = std::vector<std::vector<TextStyleSelectorList> >;  //!Index selectors by type and level

  using ShieldPartialStyle = PartialStyle<ShieldStyle, ShieldStyle::Attribute>;
  using ShieldConditionalStyle = ConditionalStyle<ShieldStyle, ShieldStyle::Attribute>;
  using ShieldStyleSelector = StyleSelector<ShieldStyle, ShieldStyle::Attribute>;
  using ShieldStyleSelectorList = std::list<ShieldStyleSelector>; //! List of selectors
  using ShieldStyleLookupTable = std::vector<std::vector<ShieldStyleSelectorList> >;  //!Index selectors by type and level

  using PathShieldPartialStyle = PartialStyle<PathShieldStyle, PathShieldStyle::Attribute>;
  using PathShieldConditionalStyle = ConditionalStyle<PathShieldStyle, PathShieldStyle::Attribute>;
  using PathShieldStyleSelector = StyleSelector<PathShieldStyle, PathShieldStyle::Attribute>;
  using PathShieldStyleSelectorList = std::list<PathShieldStyleSelector>; //! List of selectors
  using PathShieldStyleLookupTable = std::vector<std::vector<PathShieldStyleSelectorList> >;  //!Index selectors by type and level

  using PathTextPartialStyle = PartialStyle<PathTextStyle, PathTextStyle::Attribute>;
  using PathTextConditionalStyle = ConditionalStyle<PathTextStyle, PathTextStyle::Attribute>;
  using PathTextStyleSelector = StyleSelector<PathTextStyle, PathTextStyle::Attribute>;
  using PathTextStyleSelectorList = std::list<PathTextStyleSelector>; //! List of selectors
  using PathTextStyleLookupTable = std::vector<std::vector<PathTextStyleSelectorList> >;  //!Index selectors by type and level

  using IconPartialStyle = PartialStyle<IconStyle, IconStyle::Attribute>;
  using IconConditionalStyle = ConditionalStyle<IconStyle, IconStyle::Attribute>;
  using IconStyleSelector = StyleSelector<IconStyle, IconStyle::Attribute>;
  using IconStyleSelectorList = std::list<IconStyleSelector>; //! List of selectors
  using IconStyleLookupTable = std::vector<std::vector<IconStyleSelectorList> >;  //!Index selectors by type and level

  using PathSymbolPartialStyle = PartialStyle<PathSymbolStyle, PathSymbolStyle::Attribute>;
  using PathSymbolConditionalStyle = ConditionalStyle<PathSymbolStyle, PathSymbolStyle::Attribute>;
  using PathSymbolStyleSelector = StyleSelector<PathSymbolStyle, PathSymbolStyle::Attribute>;
  using PathSymbolStyleSelectorList = std::list<PathSymbolStyleSelector>; //! List of selectors
  using PathSymbolStyleLookupTable = std::vector<std::vector<PathSymbolStyleSelectorList> >;  //!Index selectors by type and level

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
  protected:
    TypeConfigRef                              typeConfig;             //!< Reference to the type configuration
    mutable StyleResolveContext                styleResolveContext;    //!< Instance of helper class that can get passed around to templated helper methods

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
    std::vector<PathSymbolStyleLookupTable>    wayPathSymbolStyleSelectors;
    PathShieldStyleLookupTable                 wayPathShieldStyleSelectors;

    std::vector<TypeInfoSet>                   wayTypeSets;     //!< way types with defined style by magnification level
    std::vector<bool>                          wayTextFlags;    //!< flags by magnification level if there is style with way label
    std::vector<bool>                          wayShieldFlags;  //!< flags by magnification level if there is style with way shield

    // Area

    std::list<FillConditionalStyle>            areaFillStyleConditionals;
    std::list<BorderConditionalStyle>          areaBorderStyleConditionals;
    std::list<TextConditionalStyle>            areaTextStyleConditionals;
    std::list<IconConditionalStyle>            areaIconStyleConditionals;
    std::list<PathTextConditionalStyle>        areaBorderTextStyleConditionals;
    std::list<PathSymbolConditionalStyle>      areaBorderSymbolStyleConditionals;

    // Route
    std::vector<LineStyleLookupTable>          routeLineStyleSelectors;
    std::list<LineConditionalStyle>            routeLineStyleConditionals;
    std::list<PathTextConditionalStyle>        routePathTextStyleConditionals;
    PathTextStyleLookupTable                   routePathTextStyleSelectors;

    FillStyleLookupTable                       areaFillStyleSelectors;
    std::vector<BorderStyleLookupTable>        areaBorderStyleSelectors;
    std::vector<TextStyleLookupTable>          areaTextStyleSelectors;
    IconStyleLookupTable                       areaIconStyleSelectors;
    PathTextStyleLookupTable                   areaBorderTextStyleSelectors;
    PathSymbolStyleLookupTable                 areaBorderSymbolStyleSelectors;

    std::vector<TypeInfoSet>                   areaTypeSets;

    // Route
    std::vector<TypeInfoSet>                   routeTypeSets;

    std::unordered_map<std::string,bool>       flags;
    std::unordered_map<std::string,StyleConstantRef> constants;
    std::list<std::string>                     errors;
    std::list<std::string>                     warnings;

  private:
    void Reset();

    void PostprocessNodes();
    void PostprocessWays();
    void PostprocessAreas();
    void PostprocessRoutes();
    void PostprocessIconId();
    void PostprocessPatternId();

  public:
    explicit StyleConfig(const TypeConfigRef& typeConfig);
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

    inline std::unordered_map<std::string,bool> GetFlags() const
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

    size_t GetFeatureFilterIndex(const Feature& feature) const;

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

    void AddRouteLineStyle(const StyleFilter& filter,
                           LinePartialStyle& style);
    void AddRoutePathTextStyle(const StyleFilter& filter,
                               PathTextPartialStyle& style);

    void GetNodeTypesWithMaxMag(const Magnification& maxMag,
                                TypeInfoSet& types) const;
    void GetWayTypesWithMaxMag(const Magnification& mag,
                               TypeInfoSet& types) const;
    void GetAreaTypesWithMaxMag(const Magnification& maxMag,
                                TypeInfoSet& types) const;
    void GetRouteTypesWithMaxMag(const Magnification& maxMag,
                                 TypeInfoSet& types) const;


    inline size_t GetWayPrio(const TypeInfoRef& type) const
    {
      if (type->GetIndex()<wayPrio.size()) {
        return wayPrio[type->GetIndex()];
      }

      return std::numeric_limits<size_t>::max();
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

    IconStyleRef GetNodeIconStyle(const FeatureValueBuffer& buffer,
                                  const Projection& projection) const;

    void GetWayLineStyles(const FeatureValueBuffer& buffer,
                          const Projection& projection,
                          std::vector<LineStyleRef>& lineStyles) const;
    void GetWayPathSymbolStyle(const FeatureValueBuffer& buffer,
                               const Projection& projection,
                               std::vector<PathSymbolStyleRef> &symbolStyles) const;
    PathTextStyleRef GetWayPathTextStyle(const FeatureValueBuffer& buffer,
                                         const Projection& projection) const;

    /**
     * @param projection
     * @return true when some way text style is defined on provided projection
     */
    bool HasWayPathTextStyle(const Projection& projection) const;

    PathShieldStyleRef GetWayPathShieldStyle(const FeatureValueBuffer& buffer,
                                             const Projection& projection) const;

    /**
     * @param projection
     * @return true when some way shield style is defined on provided projection
     */
    bool HasWayPathShieldStyle(const Projection& projection) const;

    PathTextStyleRef GetRoutePathTextStyle(const FeatureValueBuffer& buffer,
                                           const Projection& projection) const;

    void GetRouteLineStyles(const FeatureValueBuffer& buffer,
                            const Projection& projection,
                            std::vector<LineStyleRef>& lineStyles) const;

    FillStyleRef GetAreaFillStyle(const TypeInfoRef& type,
                                  const FeatureValueBuffer& buffer,
                                  const Projection& projection) const;
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
    IconStyleRef GetAreaIconStyle(const TypeInfoRef& type,
                                  const FeatureValueBuffer& buffer,
                                  const Projection& projection) const;
    PathTextStyleRef GetAreaBorderTextStyle(const TypeInfoRef& type,
                                            const FeatureValueBuffer& buffer,
                                            const Projection& projection) const;
    PathSymbolStyleRef GetAreaBorderSymbolStyle(const TypeInfoRef& type,
                                                const FeatureValueBuffer& buffer,
                                                const Projection& projection) const;

    FillStyleRef GetLandFillStyle(const Projection& projection) const;
    FillStyleRef GetSeaFillStyle(const Projection& projection) const;
    FillStyleRef GetCoastFillStyle(const Projection& projection) const;
    FillStyleRef GetUnknownFillStyle(const Projection& projection) const;
    LineStyleRef GetCoastlineLineStyle(const Projection& projection) const;
    LineStyleRef GetOSMTileBorderLineStyle(const Projection& projection) const;
    LineStyleRef GetOSMSubTileBorderLineStyle(const Projection& projection) const;
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
    bool LoadContent(const std::string& content,
                    ColorPostprocessor colorPostprocessor=nullptr);
    bool Load(const std::string& styleFile,
              ColorPostprocessor colorPostprocessor=nullptr);
    const std::list<std::string>&  GetErrors();
    const std::list<std::string>&  GetWarnings();
    //@}
  };

  using StyleConfigRef = std::shared_ptr<StyleConfig>;

  /**
   * \defgroup Stylesheet Stylesheet definition
   *
   * Classes and methods related to stylesheet definition for the renderer.
   */
}

#endif
