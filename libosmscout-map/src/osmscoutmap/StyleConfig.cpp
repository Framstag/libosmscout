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

#include <osmscoutmap/StyleConfig.h>

#include <set>
#include <algorithm>

#include <osmscout/system/Assert.h>

#include <osmscout/log/Logger.h>

#include <osmscout/io/File.h>

#include <osmscoutmap/oss/Parser.h>
#include <osmscoutmap/oss/Scanner.h>

#include <iostream>
namespace osmscout {

  StyleResolveContext::StyleResolveContext(const TypeConfigRef& typeConfig)
  : typeConfig(typeConfig),
    accessReader(*typeConfig)
  {
    // no code
  }

  bool StyleResolveContext::IsOneway(const FeatureValueBuffer& buffer) const
  {
    AccessFeatureValue *accessValue=accessReader.GetValue(buffer);

    if (accessValue!=nullptr) {
      return accessValue->IsOneway();
    }
    else {
      AccessFeatureValue accessValueDefault(buffer.GetType()->GetDefaultAccess());

      return accessValueDefault.IsOneway();
    }
  }

  size_t StyleResolveContext::GetFeatureReaderIndex(const Feature& feature)
  {
    auto entry=featureReaderMap.find(feature.GetName());

    if (entry!=featureReaderMap.end()) {
      return entry->second;
    }

    featureReaders.emplace_back(*typeConfig,
                                feature);

    size_t index=featureReaders.size()-1;

    featureReaderMap[feature.GetName()]=index;

    return index;
  }

  StyleConstantColor::StyleConstantColor(const Color& color)
  : color(color)
  {
    // no code
  }

  StyleConstantMag::StyleConstantMag(const Magnification& magnification)
  : magnification(magnification)
  {
    // no code
  }

  StyleConstantUInt::StyleConstantUInt(size_t value)
  : value(value)
  {
    // no code
  }

  StyleConstantWidth::StyleConstantWidth(double value, Unit unit)
    : value(value),
      unit(unit)
  {
    // no code
  }

  SizeCondition::SizeCondition()
  : minMM(0.0),
    minMMSet(false),
    minPx(0.0),
    minPxSet(false),
    maxMM(0.0),
    maxMMSet(false),
    maxPx(0.0),
    maxPxSet(false)
  {
    // no code
  }

  void SizeCondition::SetMinMM(double minMM)
  {
    this->minMM=minMM;
    this->minMMSet=true;
  }

  void SizeCondition::SetMinPx(double minPx)
  {
    this->minPx=minPx;
    this->minPxSet=true;
  }

  void SizeCondition::SetMaxMM(double maxMM)
  {
    this->maxMM=maxMM;
    this->maxMMSet=true;
  }

  void SizeCondition::SetMaxPx(double maxPx)
  {
    this->maxPx=maxPx;
    this->maxPxSet=true;
  }

  bool SizeCondition::Evaluate(double meterInPixel,
                               double meterInMM) const
  {
    bool matchesMinMM;
    bool matchesMinPx;

    if (minMMSet) {
      matchesMinMM=meterInMM>=minMM;
    }
    else {
      matchesMinMM=true;
    }

    if (minPxSet) {
      matchesMinPx=meterInPixel>=minPx;
    }
    else {
      matchesMinPx=true;
    }

    if (!matchesMinMM || !matchesMinPx) {
      return false;
    }

    bool matchesMaxMM;
    bool matchesMaxPx;

    if (maxMMSet) {
      matchesMaxMM=meterInMM<maxMM;
    }
    else {
      matchesMaxMM=true;
    }

    if (maxPxSet) {
      matchesMaxPx=meterInPixel<maxPx;
    }
    else {
      matchesMaxPx=true;
    }

    return matchesMaxMM || matchesMaxPx;
  }

  StyleFilter::StyleFilter()
  : filtersByType(false),
    minLevel(0),
    maxLevel(std::numeric_limits<size_t>::max()),
    oneway(false)
  {
    // no code
  }

  FeatureFilterData::FeatureFilterData(size_t featureFilterIndex,
                                       size_t flagIndex)
  : featureFilterIndex(featureFilterIndex),
    flagIndex(flagIndex)
  {
  }

  StyleFilter& StyleFilter::SetTypes(const TypeInfoSet& types)
  {
    this->types=types;
    this->filtersByType=true;

    return *this;
  }

  StyleFilter& StyleFilter::SetMinLevel(size_t level)
  {
    minLevel=level;

    return *this;
  }

  StyleFilter& StyleFilter::SetMaxLevel(size_t level)
  {
    maxLevel=level;

    return *this;
  }

  StyleFilter& StyleFilter::SetOneway(bool oneway)
  {
    this->oneway=oneway;

    return *this;
  }

  StyleFilter& StyleFilter::SetSizeCondition(const SizeConditionRef& condition)
  {
    this->sizeCondition=condition;

    return *this;
  }

  StyleFilter& StyleFilter::AddFeature(size_t featureFilterIndex,
                                       size_t flagIndex)
  {
    features.emplace_back(featureFilterIndex,flagIndex);

    return *this;
  }

  StyleCriteria::StyleCriteria(const StyleFilter& other)
  {
    this->features=other.GetFeatures();
    this->oneway=other.GetOneway();
    this->sizeCondition=other.GetSizeCondition();
  }

  bool StyleCriteria::operator==(const StyleCriteria& other) const
  {
    return features==other.features     &&
           oneway==other.oneway         &&
           sizeCondition==other.sizeCondition;
  }

  bool StyleCriteria::operator!=(const StyleCriteria& other) const
  {
    return features!=other.features     ||
           oneway!=other.oneway         ||
           sizeCondition!=other.sizeCondition;
  }

  bool StyleCriteria::Matches(const StyleResolveContext& context,
                              const FeatureValueBuffer& buffer,
                              double meterInPixel,
                              double meterInMM) const
  {
    for (const auto& feature : features) {
      if (!context.HasFeature(feature.featureFilterIndex,
                              buffer)) {
        return false;
      }

      if (feature.flagIndex!=std::numeric_limits<size_t>::max()) {
        FeatureValue *value=context.GetFeatureValue(feature.featureFilterIndex,
                                                    buffer);

        if (value==nullptr) {
          return false;
        }

        if (!value->IsFlagSet(feature.flagIndex)) {
          return false;
        }
      }
    }

    if (oneway &&
        !context.IsOneway(buffer)) {
      return false;
    }

    if (sizeCondition) {
      if (!sizeCondition->Evaluate(meterInPixel,meterInMM)) {
        return false;
      }
    }

    return true;
  }

  StyleConfig::StyleConfig(const TypeConfigRef& typeConfig)
   : typeConfig(typeConfig),
     styleResolveContext(typeConfig)
  {
    log.Debug() << "StyleConfig::StyleConfig()";

    tileLandBuffer.SetType(typeConfig->typeInfoTileLand);
    tileSeaBuffer.SetType(typeConfig->typeInfoTileSea);
    tileCoastBuffer.SetType(typeConfig->typeInfoTileCoast);
    tileUnknownBuffer.SetType(typeConfig->typeInfoTileUnknown);
    coastlineBuffer.SetType(typeConfig->typeInfoCoastline);
    osmTileBorderBuffer.SetType(typeConfig->typeInfoOSMTileBorder);
    osmSubTileBorderBuffer.SetType(typeConfig->typeInfoOSMSubTileBorder);

    LabelProviderFactoryRef labelProviderFactory=std::make_shared<INameLabelProviderFactory>();

    RegisterLabelProviderFactory("IName",labelProviderFactory);
  }

  StyleConfig::~StyleConfig()
  {
    log.Debug() << "StyleConfig::~StyleConfig()";
  }

  void StyleConfig::Reset()
  {
    symbols.clear();
    emptySymbol=nullptr;

    nodeTextStyleConditionals.clear();
    nodeIconStyleConditionals.clear();
    nodeTextStyleSelectors.clear();
    nodeIconStyleSelectors.clear();
    nodeTypeSets.clear();

    wayPrio.clear();
    wayLineStyleConditionals.clear();
    wayPathTextStyleConditionals.clear();
    wayPathSymbolStyleConditionals.clear();
    wayPathShieldStyleConditionals.clear();
    wayLineStyleSelectors.clear();
    wayPathTextStyleSelectors.clear();
    wayPathSymbolStyleSelectors.clear();
    wayPathShieldStyleSelectors.clear();
    wayTypeSets.clear();
    wayTextFlags.clear();
    wayShieldFlags.clear();

    areaFillStyleConditionals.clear();
    areaBorderStyleConditionals.clear();
    areaTextStyleConditionals.clear();
    areaIconStyleConditionals.clear();
    areaBorderTextStyleConditionals.clear();
    areaBorderSymbolStyleConditionals.clear();

    areaFillStyleSelectors.clear();
    areaBorderStyleSelectors.clear();
    areaTextStyleSelectors.clear();
    areaIconStyleSelectors.clear();
    areaBorderTextStyleSelectors.clear();
    areaBorderSymbolStyleSelectors.clear();
    areaTypeSets.clear();

    routeTypeSets.clear();
    routeLineStyleSelectors.clear();
    routePathTextStyleConditionals.clear();

    constants.clear();
  }

  bool StyleConfig::RegisterLabelProviderFactory(const std::string& name,
                                                 const LabelProviderFactoryRef& factory)
  {
    if (!factory) {
      return false;
    }

    if (labelFactories.find(name)!=labelFactories.end()) {
      return false;
    }

    labelFactories[name]=factory;

    return true;
  }

  LabelProviderRef StyleConfig::GetLabelProvider(const std::string& name) const
  {
    auto entry=labelFactories.find(name);

    if (entry==labelFactories.end()) {
      return nullptr;
    }

    return entry->second->Create(*typeConfig);
  }

  /**
   * Returns 'true', if the given flag exists, else 'false'.
   */
  bool StyleConfig::HasFlag(const std::string& name) const
  {
    return flags.find(name)!=flags.end();
  }

  /**
   * Returns the value of the given flag identified by the name of the flag.
   *
   * Asserts, if the flag name is unknown.
   */
  bool StyleConfig::GetFlagByName(const std::string& name) const
  {
    const auto entry=flags.find(name);

    assert(entry!=flags.end());

    return entry->second;
  }

  /**
   * Add the flag with the given value. If the flag already exists, its value
   * gets overwritten.
   */
  void StyleConfig::AddFlag(const std::string& name,
                            bool value)
  {
    flags[name]=value;
  }

  StyleConstantRef StyleConfig::GetConstantByName(const std::string& name) const
  {
    StyleConstantRef result;

    auto entry=constants.find(name);

    if (entry!=constants.end()) {
      result=entry->second;
    }

    return result;
  }

  void StyleConfig::AddConstant(const std::string& name,
                                const StyleConstantRef& variable)
  {
    constants.insert(std::make_pair(name,variable));
  }

  bool StyleConfig::RegisterSymbol(const SymbolRef& symbol)
  {
    auto result=symbols.insert(std::make_pair(symbol->GetName(),symbol));

    return result.second;
  }

  const SymbolRef& StyleConfig::GetSymbol(const std::string& name) const
  {
    auto entry=symbols.find(name);

    if (entry!=symbols.end()) {
      return entry->second;
    }
    else {
      return emptySymbol;
    }
  }

  template <class S, class A>
  void GetMaxLevelInConditionals(const std::list<ConditionalStyle<S,A> >& conditionals,
                                 size_t& maxLevel)
  {
    for (const auto& conditional : conditionals) {
      maxLevel=std::max(maxLevel,conditional.filter.GetMinLevel()+1);

      if (conditional.filter.HasMaxLevel()) {
        maxLevel=std::max(maxLevel,conditional.filter.GetMaxLevel()+1);
      }
    }
  }

  template <class S, class A>
  void CalculateUsedTypes(const TypeConfig& typeConfig,
                          const std::list<ConditionalStyle<S,A> >& conditionals,
                          size_t maxLevel,
                          std::vector<TypeInfoSet>& typeSets)
  {
    for (size_t level=0;
        level<maxLevel;
        ++level) {
      for (const auto& conditional : conditionals) {
        for (const auto& type : typeConfig.GetTypes()) {
          if (!conditional.filter.HasType(type)) {
            continue;
          }

          if (level<conditional.filter.GetMinLevel()) {
            continue;
          }

          if (conditional.filter.HasMaxLevel() &&
              level>conditional.filter.GetMaxLevel()) {
            continue;
          }

          typeSets[level].Set(type);
        }
      }
    }
  }

  template <class S, class A>
  void SortInConditionals(const TypeConfig& typeConfig,
                          const std::list<ConditionalStyle<S,A> >& conditionals,
                          size_t maxLevel,
                          std::vector<std::vector<std::list<StyleSelector<S,A> > > >& selectors)
  {
    selectors.resize(typeConfig.GetTypeCount());

    for (auto& selector : selectors) {
      selector.resize(maxLevel+1);
    }

    for (const auto& conditional : conditionals) {
      StyleSelector<S,A> selector(conditional.filter,conditional.style);

      for (const auto& type : typeConfig.GetTypes()) {
        if (!conditional.filter.HasType(type)) {
          continue;
        }

        size_t minLvl=conditional.filter.GetMinLevel();
        size_t maxLvl=conditional.filter.HasMaxLevel() ? conditional.filter.GetMaxLevel() : maxLevel;

        for (size_t level=minLvl; level<=maxLvl; level++) {
          selectors[type->GetIndex()][level].push_back(selector);
        }
      }
    }

    for (auto& selector : selectors) {
      for (size_t level=0; level<selector.size(); level++) {
        if (selector[level].size()>=2) {
          // If two consecutive conditions are equal, one can be removed and the style can get merged
          auto prevSelector=selector[level].begin();
          auto curSelector=prevSelector;

          ++curSelector;

          while (curSelector!=selector[level].end()) {
            if (prevSelector->criteria==curSelector->criteria) {
              prevSelector->attributes.insert(curSelector->attributes.begin(),
                                              curSelector->attributes.end());
              prevSelector->style=std::make_shared<S>(*prevSelector->style);
              prevSelector->style->CopyAttributes(*curSelector->style,
                                                  curSelector->attributes);

              curSelector=selector[level].erase(curSelector);
            }
            else {
              prevSelector=curSelector;
              ++curSelector;
            }
          }
        }

        // If there is only one conditional and it is not visible, we can remove it
        if (selector[level].size()==1 &&
            !selector[level].front().style->IsVisible()) {
          selector[level].clear();
        }
      }
    }
  }

  template <class S, class A>
  void SortInConditionalsBySlot(const TypeConfig& typeConfig,
                                const std::list<ConditionalStyle<S,A> >& conditionals,
                                size_t maxLevel,
                                std::vector<std::vector<std::vector<std::list<StyleSelector<S,A> > > > >& selectors)
  {
    std::unordered_map<std::string,std::list<ConditionalStyle<S,A>> > styleBySlot;

    for (auto& conditional : conditionals) {
      styleBySlot[conditional.style.style->GetSlot()].push_back(conditional);
    }

    selectors.resize(styleBySlot.size());

    size_t idx=0;
    for (const auto& entry : styleBySlot) {
      SortInConditionals(typeConfig,
                         entry.second,
                         maxLevel,
                         selectors[idx]);

      idx++;
    }
  }

  void StyleConfig::PostprocessNodes()
  {
    size_t maxLevel=0;

    GetMaxLevelInConditionals(nodeTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(nodeIconStyleConditionals,
                              maxLevel);

    SortInConditionalsBySlot(*typeConfig,
                             nodeTextStyleConditionals,
                             maxLevel,
                             nodeTextStyleSelectors);

    SortInConditionals(*typeConfig,
                       nodeIconStyleConditionals,
                       maxLevel,
                       nodeIconStyleSelectors);

    nodeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      nodeTypeSets.emplace_back(*typeConfig);
    }

    CalculateUsedTypes(*typeConfig,
                       nodeTextStyleConditionals,
                       maxLevel,
                       nodeTypeSets);
    CalculateUsedTypes(*typeConfig,
                       nodeIconStyleConditionals,
                       maxLevel,
                       nodeTypeSets);

    nodeTextStyleConditionals.clear();
    nodeIconStyleConditionals.clear();
  }

  template <class S, class A>
  bool HasStyle(const std::vector<std::vector<std::list<StyleSelector<S,A>>>>& styleSelectors,
                const size_t level)
  {
    for (const auto &selectorsForType: styleSelectors){
      assert(!selectorsForType.empty());
      if (!selectorsForType[std::min(level, selectorsForType.size() - 1)].empty()){
        return true;
      }
    }
    return false;
  }

  void StyleConfig::PostprocessWays()
  {
    size_t maxLevel=0;

    GetMaxLevelInConditionals(wayLineStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(wayPathTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(wayPathSymbolStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(wayPathShieldStyleConditionals,
                              maxLevel);

    SortInConditionalsBySlot(*typeConfig,
                             wayLineStyleConditionals,
                             maxLevel,
                             wayLineStyleSelectors);

    SortInConditionalsBySlot(*typeConfig,
                             wayPathSymbolStyleConditionals,
                             maxLevel,
                             wayPathSymbolStyleSelectors);

    SortInConditionals(*typeConfig,
                       wayPathTextStyleConditionals,
                       maxLevel,
                       wayPathTextStyleSelectors);

    SortInConditionals(*typeConfig,
                       wayPathShieldStyleConditionals,
                       maxLevel,
                       wayPathShieldStyleSelectors);

    wayTypeSets.reserve(maxLevel);
    wayTextFlags.reserve(maxLevel);
    wayShieldFlags.reserve(maxLevel);

    for (size_t level=0; level < maxLevel; level++) {
      wayTypeSets.emplace_back(*typeConfig);
      wayTextFlags.emplace_back(HasStyle(wayPathTextStyleSelectors, level));
      wayShieldFlags.emplace_back(HasStyle(wayPathShieldStyleSelectors, level));
    }

    CalculateUsedTypes(*typeConfig,
                       wayLineStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    CalculateUsedTypes(*typeConfig,
                       wayPathTextStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    CalculateUsedTypes(*typeConfig,
                       wayPathSymbolStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    CalculateUsedTypes(*typeConfig,
                       wayPathShieldStyleConditionals,
                       maxLevel,
                       wayTypeSets);

    wayLineStyleConditionals.clear();
    wayPathTextStyleConditionals.clear();
    wayPathSymbolStyleConditionals.clear();
    wayPathShieldStyleConditionals.clear();
  }

  void StyleConfig::PostprocessAreas()
  {
    size_t maxLevel=0;

    GetMaxLevelInConditionals(areaFillStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaBorderStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaIconStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaBorderTextStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(areaBorderSymbolStyleConditionals,
                              maxLevel);

    SortInConditionals(*typeConfig,
                       areaFillStyleConditionals,
                       maxLevel,
                       areaFillStyleSelectors);

    SortInConditionalsBySlot(*typeConfig,
                             areaBorderStyleConditionals,
                             maxLevel,
                             areaBorderStyleSelectors);

    SortInConditionalsBySlot(*typeConfig,
                             areaTextStyleConditionals,
                             maxLevel,
                             areaTextStyleSelectors);

    SortInConditionals(*typeConfig,
                       areaIconStyleConditionals,
                       maxLevel,
                       areaIconStyleSelectors);

    SortInConditionals(*typeConfig,
                       areaBorderTextStyleConditionals,
                       maxLevel,
                       areaBorderTextStyleSelectors);

    SortInConditionals(*typeConfig,
                       areaBorderSymbolStyleConditionals,
                       maxLevel,
                       areaBorderSymbolStyleSelectors);

    areaTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      areaTypeSets.emplace_back(*typeConfig);
    }

    CalculateUsedTypes(*typeConfig,
                       areaFillStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaBorderStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaTextStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaIconStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaBorderTextStyleConditionals,
                       maxLevel,
                       areaTypeSets);
    CalculateUsedTypes(*typeConfig,
                       areaBorderSymbolStyleConditionals,
                       maxLevel,
                       areaTypeSets);

    areaFillStyleConditionals.clear();
    areaBorderStyleConditionals.clear();
    areaTextStyleConditionals.clear();
    areaIconStyleConditionals.clear();
    areaBorderTextStyleConditionals.clear();
    areaBorderSymbolStyleConditionals.clear();
  }

  void StyleConfig::PostprocessRoutes()
  {
    size_t maxLevel=0;
    GetMaxLevelInConditionals(routeLineStyleConditionals,
                              maxLevel);
    GetMaxLevelInConditionals(routePathTextStyleConditionals,
                              maxLevel);

    SortInConditionals(*typeConfig,
                       routePathTextStyleConditionals,
                       maxLevel,
                       routePathTextStyleSelectors);

    routeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      routeTypeSets.emplace_back(*typeConfig);
    }

    CalculateUsedTypes(*typeConfig,
                       routeLineStyleConditionals,
                       maxLevel,
                       routeTypeSets);

    CalculateUsedTypes(*typeConfig,
                       routePathTextStyleConditionals,
                       maxLevel,
                       routeTypeSets);

    SortInConditionalsBySlot(*typeConfig,
                             routeLineStyleConditionals,
                             maxLevel,
                             routeLineStyleSelectors);

    routeLineStyleConditionals.clear();
    routePathTextStyleConditionals.clear();
  }

  void StyleConfig::PostprocessIconId()
  {
    std::unordered_map<std::string,size_t> symbolIdMap;
    size_t                                 nextId=1;

    for (auto& typeSelector : areaIconStyleSelectors) {
      for (auto& levelSelector : typeSelector) {
        for (auto& selector : levelSelector) {
          if (!selector.style->GetIconName().empty()) {
            auto entry=symbolIdMap.find(selector.style->GetIconName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector.style->GetIconName(),nextId));

              selector.style->SetIconId(nextId);

              nextId++;
            }
            else {
              selector.style->SetIconId(entry->second);
            }
          }
        }
      }
    }

    for (auto& typeSelector: nodeIconStyleSelectors) {
      for (auto& levelSelector : typeSelector) {
        for (auto& selector : levelSelector) {
          if (!selector.style->GetIconName().empty()) {
            auto entry=symbolIdMap.find(selector.style->GetIconName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector.style->GetIconName(),nextId));

              selector.style->SetIconId(nextId);

              nextId++;
            }
            else {
              selector.style->SetIconId(entry->second);
            }
          }
        }
      }
    }
  }

  void StyleConfig::PostprocessPatternId()
  {
    std::unordered_map<std::string,size_t> symbolIdMap;
    size_t                                 nextId=1;

    for (auto& typeSelector : areaFillStyleSelectors) {
      for (auto& levelSelector: typeSelector) {
        for (auto& selector : levelSelector) {
          if (!selector.style->GetPatternName().empty()) {
            auto entry=symbolIdMap.find(selector.style->GetPatternName());

            if (entry==symbolIdMap.end()) {
              symbolIdMap.insert(std::make_pair(selector.style->GetPatternName(),nextId));

              selector.style->SetPatternId(nextId);

              nextId++;
            }
            else {
              selector.style->SetPatternId(entry->second);
            }
          }
        }
      }
    }
  }

  void StyleConfig::Postprocess()
  {
    PostprocessNodes();
    PostprocessWays();
    PostprocessAreas();
    PostprocessRoutes();

    PostprocessIconId();
    PostprocessPatternId();
  }

  TypeConfigRef StyleConfig::GetTypeConfig() const
  {
    return typeConfig;
  }

  size_t StyleConfig::GetFeatureFilterIndex(const Feature& feature) const
  {
    return styleResolveContext.GetFeatureReaderIndex(feature);
  }

  StyleConfig& StyleConfig::SetWayPrio(const TypeInfoRef& type,
                                       size_t prio)
  {
    if (wayPrio.size()<=type->GetIndex()) {
      wayPrio.resize(type->GetIndex()+1,
                     std::numeric_limits<size_t>::max());
    }

    wayPrio[type->GetIndex()]=prio;

    return *this;
  }

  void StyleConfig::AddNodeTextStyle(const StyleFilter& filter,
                                     TextPartialStyle& style)
  {
    TextConditionalStyle conditional(filter,style);

    nodeTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddNodeIconStyle(const StyleFilter& filter,
                                        IconPartialStyle& style)
  {
    IconConditionalStyle conditional(filter,style);

    nodeIconStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayLineStyle(const StyleFilter& filter,
                                    LinePartialStyle& style)
  {
    LineConditionalStyle conditional(filter,style);

    wayLineStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayPathTextStyle(const StyleFilter& filter,
                                        PathTextPartialStyle& style)
  {
    PathTextConditionalStyle conditional(filter,style);

    wayPathTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayPathSymbolStyle(const StyleFilter& filter,
                                          PathSymbolPartialStyle& style)
  {
    PathSymbolConditionalStyle conditional(filter,style);

    wayPathSymbolStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddWayPathShieldStyle(const StyleFilter& filter,
                                         PathShieldPartialStyle& style)
  {
    PathShieldConditionalStyle conditional(filter,style);

    wayPathShieldStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaFillStyle(const StyleFilter& filter,
                                     FillPartialStyle& style)
  {
    FillConditionalStyle conditional(filter,style);

    areaFillStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaBorderStyle(const StyleFilter& filter,
                                       BorderPartialStyle& style)
  {
    BorderConditionalStyle conditional(filter,style);

    areaBorderStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaTextStyle(const StyleFilter& filter,
                                     TextPartialStyle& style)
  {
    TextConditionalStyle conditional(filter,style);

    areaTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaIconStyle(const StyleFilter& filter,
                                     IconPartialStyle& style)
  {
    IconConditionalStyle conditional(filter,style);

    areaIconStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaBorderTextStyle(const StyleFilter& filter,
                                           PathTextPartialStyle& style)
  {
    PathTextConditionalStyle conditional(filter,style);

    areaBorderTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddAreaBorderSymbolStyle(const StyleFilter& filter,
                                             PathSymbolPartialStyle& style)
  {
    PathSymbolConditionalStyle conditional(filter,style);

    areaBorderSymbolStyleConditionals.push_back(conditional);
  }


  void StyleConfig::AddRouteLineStyle(const StyleFilter& filter,
                                      LinePartialStyle& style)
  {
    LineConditionalStyle conditional(filter,style);

    routeLineStyleConditionals.push_back(conditional);
  }

  void StyleConfig::AddRoutePathTextStyle(const StyleFilter& filter,
                                          PathTextPartialStyle& style)
  {
    PathTextConditionalStyle conditional(filter,style);

    routePathTextStyleConditionals.push_back(conditional);
  }

  void StyleConfig::GetNodeTypesWithMaxMag(const Magnification& maxMag,
                                           TypeInfoSet& types) const
  {
    if (!nodeTypeSets.empty()) {
      types=nodeTypeSets[std::min((size_t)maxMag.GetLevel(),nodeTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetWayTypesWithMaxMag(const Magnification& maxMag,
                                          TypeInfoSet& types) const
  {
    if (!wayTypeSets.empty()) {
      types=wayTypeSets[std::min((size_t)maxMag.GetLevel(),wayTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetAreaTypesWithMaxMag(const Magnification& maxMag,
                                           TypeInfoSet& types) const
  {
    if (!areaTypeSets.empty()) {
      types=areaTypeSets[std::min((size_t)maxMag.GetLevel(),areaTypeSets.size()-1)];
    }
  }

  void StyleConfig::GetRouteTypesWithMaxMag(const Magnification& maxMag,
                                            TypeInfoSet& types) const
  {
    if (!routeTypeSets.empty()) {
      types=routeTypeSets[std::min((size_t)maxMag.GetLevel(),routeTypeSets.size()-1)];
    }
  }

  /**
   * Get the style data based on the given features of an object,
   * a given style (S) and its style attributes (A).
   */
  template <class S, class A>
  std::shared_ptr<S> GetFeatureStyle(const StyleResolveContext& context,
                                     const std::vector<std::list<StyleSelector<S,A> > >& styleSelectors,
                                     const FeatureValueBuffer& buffer,
                                     const Projection& projection)
  {
    assert(!styleSelectors.empty());

    bool               fastpath=false;
    bool               composed=false;
    size_t             level=projection.GetMagnification().GetLevel();
    double             meterInPixel=projection.GetMeterInPixel();
    double             meterInMM=projection.GetMeterInMM();
    std::shared_ptr<S> style;

    if (level>=styleSelectors.size()) {
      level=styleSelectors.size()-1;
    }

    for (const auto& selector : styleSelectors[level]) {
      if (!selector.criteria.Matches(context,
                                     buffer,
                                     meterInPixel,
                                     meterInMM)) {
        continue;
      }

      if (!style) {
        style=selector.style;
        fastpath=true;

        continue;
      }

      if (fastpath) {
        style=std::make_shared<S>(*style);
        fastpath=false;
      }

      style->CopyAttributes(*selector.style,
                            selector.attributes);
      composed=true;
    }

    if (composed &&
        !style->IsVisible()) {
      style=nullptr;
    }

    return style;
  }

  bool StyleConfig::HasNodeTextStyles(const TypeInfoRef& type,
                                      const Magnification& magnification) const
  {
    auto level=magnification.GetLevel();

    for (const auto& nodeTextStyleSelector : nodeTextStyleSelectors) {
      if (level>=nodeTextStyleSelector[type->GetIndex()].size()) {
        level=static_cast<uint32_t>(nodeTextStyleSelector[type->GetIndex()].size()-1);
      }

      if (!nodeTextStyleSelector[type->GetIndex()][level].empty()) {
        return true;
      }
    }

    return false;
  }

  void StyleConfig::GetNodeTextStyles(const FeatureValueBuffer& buffer,
                                      const Projection& projection,
                                      std::vector<TextStyleRef>& textStyles) const
  {

    textStyles.clear();
    textStyles.reserve(nodeTextStyleSelectors.size());

    for (const auto& nodeTextStyleSelector : nodeTextStyleSelectors) {
      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         nodeTextStyleSelector[buffer.GetType()->GetIndex()],
                                         buffer,
                                         projection);

      if (style) {
        textStyles.push_back(style);
      }
    }
  }

  size_t StyleConfig::GetNodeTextStyleCount(const FeatureValueBuffer& buffer,
                                            const Projection& projection) const
  {
    size_t count=0;

    for (const auto& nodeTextStyleSelector : nodeTextStyleSelectors) {
      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         nodeTextStyleSelector[buffer.GetType()->GetIndex()],
                                         buffer,
                                         projection);

      if (style) {
        count++;
      }
    }

    return count;
  }

  IconStyleRef StyleConfig::GetNodeIconStyle(const FeatureValueBuffer& buffer,
                                             const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           nodeIconStyleSelectors[buffer.GetType()->GetIndex()],
                           buffer,
                           projection);
  }

  void StyleConfig::GetWayLineStyles(const FeatureValueBuffer& buffer,
                                     const Projection& projection,
                                     std::vector<LineStyleRef>& lineStyles) const
  {
    lineStyles.clear();
    lineStyles.reserve(wayLineStyleSelectors.size());

    bool requireSort=false;

    for (const auto& wayLineStyleSelector : wayLineStyleSelectors) {
      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         wayLineStyleSelector[buffer.GetType()->GetIndex()],
                                         buffer,
                                         projection);

      if (style) {
        if (style->GetOffsetRel()!=OffsetRel::base) {
          requireSort=true;
        }

        lineStyles.push_back(style);
      }
    }

    if (requireSort &&
        lineStyles.size()>1) {
      std::sort(lineStyles.begin(),
                lineStyles.end(),
                [](const LineStyleRef& a, const LineStyleRef& b) -> bool {
                  return a->GetSlot()<b->GetSlot();
      });
    }
  }

  void StyleConfig::GetRouteLineStyles(const FeatureValueBuffer& buffer,
                                       const Projection& projection,
                                       std::vector<LineStyleRef>& lineStyles) const
  {
    lineStyles.clear();
    lineStyles.reserve(routeLineStyleSelectors.size());

    bool requireSort=false;

    for (const auto& routeLineStyleSelector : routeLineStyleSelectors) {
      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         routeLineStyleSelector[buffer.GetType()->GetIndex()],
                                         buffer,
                                         projection);

      if (style) {
        if (style->GetOffsetRel()!=OffsetRel::base) {
          requireSort=true;
        }

        lineStyles.push_back(style);
      }
    }

    if (requireSort &&
        lineStyles.size()>1) {
      std::sort(lineStyles.begin(),
                lineStyles.end(),
                [](const LineStyleRef& a, const LineStyleRef& b) -> bool {
                  return a->GetSlot()<b->GetSlot();
                });
    }
  }

  void StyleConfig::GetWayPathSymbolStyle(const FeatureValueBuffer& buffer,
                                          const Projection& projection,
                                          std::vector<PathSymbolStyleRef> &symbolStyles) const
  {
    symbolStyles.clear();
    symbolStyles.reserve(wayLineStyleSelectors.size());
    for (const auto& wayPathSymbolStyleSelector : wayPathSymbolStyleSelectors) {
      PathSymbolStyleRef style=GetFeatureStyle(styleResolveContext,
                                               wayPathSymbolStyleSelector[buffer.GetType()->GetIndex()],
                                               buffer,
                                               projection);
      if (style) {
        symbolStyles.push_back(style);
      }
    }
  }


  PathTextStyleRef StyleConfig::GetWayPathTextStyle(const FeatureValueBuffer& buffer,
                                                    const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           wayPathTextStyleSelectors[buffer.GetType()->GetIndex()],
                           buffer,
                           projection);
  }

  bool StyleConfig::HasWayPathTextStyle(const Projection& projection) const
  {
    if (wayTextFlags.empty()){
      return false;
    }
    size_t level = projection.GetMagnification().GetLevel();
    return wayTextFlags[std::min(level, wayTextFlags.size()-1)];
  }

  PathTextStyleRef StyleConfig::GetRoutePathTextStyle(const FeatureValueBuffer& buffer,
                                                      const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           routePathTextStyleSelectors[buffer.GetType()->GetIndex()],
                           buffer,
                           projection);
  }

  PathShieldStyleRef StyleConfig::GetWayPathShieldStyle(const FeatureValueBuffer& buffer,
                                                        const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           wayPathShieldStyleSelectors[buffer.GetType()->GetIndex()],
                           buffer,
                           projection);
  }

  bool StyleConfig::HasWayPathShieldStyle(const Projection& projection) const
  {
    if (wayShieldFlags.empty()){
      return false;
    }
    size_t level = projection.GetMagnification().GetLevel();
    return wayShieldFlags[std::min(level, wayTextFlags.size()-1)];
  }

  FillStyleRef StyleConfig::GetAreaFillStyle(const TypeInfoRef& type,
                                             const FeatureValueBuffer& buffer,
                                             const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaFillStyleSelectors[type->GetIndex()],
                           buffer,
                           projection);
  }

  void StyleConfig::GetAreaBorderStyles(const TypeInfoRef& type,
                                        const FeatureValueBuffer& buffer,
                                        const Projection& projection,
                                        std::vector<BorderStyleRef>& borderStyles) const
  {
    borderStyles.clear();
    borderStyles.reserve(areaBorderStyleSelectors.size());

    for (const auto& areaBorderStyleSelector : areaBorderStyleSelectors) {
      BorderStyleRef style=GetFeatureStyle(styleResolveContext,
                                           areaBorderStyleSelector[type->GetIndex()],
                                           buffer,
                                           projection);

      if (style) {
        borderStyles.push_back(style);
      }
    }
  }

  bool StyleConfig::HasAreaTextStyles(const TypeInfoRef& type,
                                      const Magnification& magnification) const
  {
    auto level=magnification.GetLevel();

    for (const auto& areaTextStyleSelector : areaTextStyleSelectors) {
      if (level>=areaTextStyleSelector[type->GetIndex()].size()) {
        level=static_cast<uint32_t>(areaTextStyleSelector[type->GetIndex()].size()-1);
      }

      if (!areaTextStyleSelector[type->GetIndex()][level].empty()) {
        return true;
      }
    }

    return false;
  }

  void StyleConfig::GetAreaTextStyles(const TypeInfoRef& type,
                                      const FeatureValueBuffer& buffer,
                                      const Projection& projection,
                                      std::vector<TextStyleRef>& textStyles) const
  {
    textStyles.clear();
    textStyles.reserve(areaTextStyleSelectors.size());

    for (const auto& areaTextStyleSelector : areaTextStyleSelectors) {
      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         areaTextStyleSelector[type->GetIndex()],
                                         buffer,
                                         projection);

      if (style) {
        textStyles.push_back(style);
      }
    }
  }

  size_t StyleConfig::GetAreaTextStyleCount(const TypeInfoRef& type,
                                            const FeatureValueBuffer& buffer,
                                            const Projection& projection) const
  {
    size_t count=0;

    for (const auto& areaTextStyleSelector : areaTextStyleSelectors) {
      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         areaTextStyleSelector[type->GetIndex()],
                                         buffer,
                                         projection);

      if (style) {
        count++;
      }
    }

    return count;
  }

  IconStyleRef StyleConfig::GetAreaIconStyle(const TypeInfoRef& type,
                                             const FeatureValueBuffer& buffer,
                                             const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaIconStyleSelectors[type->GetIndex()],
                           buffer,
                           projection);
  }

  PathTextStyleRef StyleConfig::GetAreaBorderTextStyle(const TypeInfoRef& type,
                                                       const FeatureValueBuffer& buffer,
                                                       const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaBorderTextStyleSelectors[type->GetIndex()],
                           buffer,
                           projection);
  }

  PathSymbolStyleRef StyleConfig::GetAreaBorderSymbolStyle(const TypeInfoRef& type,
                                                           const FeatureValueBuffer& buffer,
                                                           const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaBorderSymbolStyleSelectors[type->GetIndex()],
                           buffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetLandFillStyle(const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaFillStyleSelectors[tileLandBuffer.GetType()->GetIndex()],
                           tileLandBuffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetSeaFillStyle(const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaFillStyleSelectors[tileSeaBuffer.GetType()->GetIndex()],
                           tileSeaBuffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetCoastFillStyle(const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaFillStyleSelectors[tileCoastBuffer.GetType()->GetIndex()],
                           tileCoastBuffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetUnknownFillStyle(const Projection& projection) const
  {
    return GetFeatureStyle(styleResolveContext,
                           areaFillStyleSelectors[tileUnknownBuffer.GetType()->GetIndex()],
                           tileUnknownBuffer,
                           projection);
  }

  LineStyleRef StyleConfig::GetCoastlineLineStyle(const Projection& projection) const
  {
    for (const auto& wayLineStyleSelector : wayLineStyleSelectors) {
      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         wayLineStyleSelector[coastlineBuffer.GetType()->GetIndex()],
                                         coastlineBuffer,
                                         projection);

      if (style) {
        return style;
      }
    }

    return nullptr;
  }

  LineStyleRef StyleConfig::GetOSMTileBorderLineStyle(const Projection& projection) const
  {
    for (const auto& wayLineStyleSelector : wayLineStyleSelectors) {
      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         wayLineStyleSelector[osmTileBorderBuffer.GetType()->GetIndex()],
                                         osmTileBorderBuffer,
                                         projection);

      if (style) {
        return style;
      }
    }

    return nullptr;
  }

  LineStyleRef StyleConfig::GetOSMSubTileBorderLineStyle(const Projection& projection) const
  {
    for (const auto& wayLineStyleSelector : wayLineStyleSelectors) {
      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         wayLineStyleSelector[osmSubTileBorderBuffer.GetType()->GetIndex()],
                                         osmSubTileBorderBuffer,
                                         projection);

      if (style) {
        return style;
      }
    }

    return nullptr;
  }

  void StyleConfig::GetNodeTextStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<TextStyleSelector>& selectors) const
  {
    selectors.clear();

    for (const auto& slotEntry : nodeTextStyleSelectors) {
      size_t l=level;

      if (l>=slotEntry[type->GetIndex()].size()) {
        l=slotEntry[type->GetIndex()].size()-1;
      }

      for (const auto& selector : slotEntry[type->GetIndex()][l]) {
        selectors.push_back(selector);
      }
    }
  }

  void StyleConfig::GetAreaFillStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<FillStyleSelector>& selectors) const
  {
    selectors.clear();

    if (level>=areaFillStyleSelectors[type->GetIndex()].size()) {
      level=areaFillStyleSelectors[type->GetIndex()].size()-1;
    }

    for (const auto& selector : areaFillStyleSelectors[type->GetIndex()][level]) {
      selectors.push_back(selector);
    }
  }

  void StyleConfig::GetAreaTextStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<TextStyleSelector>& selectors) const
  {
    selectors.clear();

    for (const auto& slotEntry : areaTextStyleSelectors) {
      size_t l=level;

      if (l>=slotEntry[type->GetIndex()].size()) {
        l=slotEntry[type->GetIndex()].size()-1;
      }

      for (const auto& selector : slotEntry[type->GetIndex()][l]) {
        selectors.push_back(selector);
      }
    }
  }

  bool StyleConfig::LoadContent(const std::string& filename,
                                const std::string& content,
                                ColorPostprocessor colorPostprocessor,
                                bool submodule)
  {
    oss::Scanner *scanner=new oss::Scanner((const unsigned char *)content.c_str(),
                                           content.length());
    oss::Parser  *parser=new oss::Parser(scanner,
                                         filename,
                                         *this,
                                         colorPostprocessor);
    parser->Parse();

    bool success=!parser->errors->hasErrors;

    errors.clear();
    warnings.clear();

    for (const auto& err : parser->errors->errors) {
      switch(err.type) {
        case oss::Errors::Err::Symbol:
          errors.push_back(StyleError(StyleError::Symbol, err.line, err.column, err.text));
          break;
        case oss::Errors::Err::Error:
          errors.push_back(StyleError(StyleError::Error, err.line, err.column, err.text));
          break;
        case oss::Errors::Err::Warning:
          warnings.push_back(StyleError(StyleError::Warning, err.line, err.column, err.text));
          break;
        case oss::Errors::Err::Exception:
          errors.push_back(StyleError(StyleError::Exception, err.line, err.column, err.text));
          break;
        default:
          break;
      }
    }

    delete parser;
    delete scanner;
    if (!submodule) {
      Postprocess();
    }
    return success;
  }

  /**
   * Load the given *.oss file into the current style config object.
   *
   * @param styleFile
   *    The file to load
   * @param colorPostprocessor
   *    Optional function to post process color values
   * @param submodule
   *    Before loading submodule, style config is not reset and post-process after.
   * @return
   *     true, if loading was successful, else false
   */
  bool StyleConfig::Load(const std::string& styleFile,
                         ColorPostprocessor colorPostprocessor,
                         bool submodule)
  {
    StopClock  timer;
    bool       success=false;

    log.Debug() << "Opening StyleConfig '" << styleFile << "'...";

    try {
      FILE*      file;
      FileOffset fileSize;

      if (!submodule) {
        Reset();
      }

      fileSize=GetFileSize(styleFile);

      file=fopen(styleFile.c_str(),"rb");
      if (file==nullptr) {
        log.Error() << "Cannot open file '" << styleFile << "'";

        return false;
      }

      unsigned char* content=new unsigned char[fileSize];

      if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
        log.Error() << "Cannot load file '" << styleFile << "'";
        delete [] content;
        fclose(file);

        return false;
      }

      fclose(file);

      success=LoadContent(styleFile,
                          std::string((const char *)content,fileSize),
                          colorPostprocessor,
                          submodule);

      delete [] content;

      timer.Stop();

      log.Debug() << "Opening StyleConfig '" << styleFile << "' " << timer.ResultString();
    }
    catch (const IOException& e) {
      log.Error() << e.GetDescription();
    }

    return success;
  }

  const std::list<StyleError>& StyleConfig::GetErrors() const
  {
    return errors;
  }

  const std::list<StyleError>& StyleConfig::GetWarnings() const
  {
    return warnings;
  }
}
