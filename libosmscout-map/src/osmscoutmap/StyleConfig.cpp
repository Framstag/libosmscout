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
  {}

  StyleFilter& StyleFilter::SetTypes(const TypeInfoSet& types)
  {
    this->types=types;
    this->filtersByType=true;

    typeList.clear();

    for (const auto& type : types) {
      typeList.push_back(type);
    }

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
    return features==other.features &&
           oneway==other.oneway &&
           sizeCondition==other.sizeCondition;
  }

  bool StyleCriteria::operator!=(const StyleCriteria& other) const
  {
    return features!=other.features ||
           oneway!=other.oneway ||
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
    visibilityBounds.clear();

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
    maxAreaBorderWidthMM.clear();

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

    auto             entry=constants.find(name);

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

  std::vector<std::string> StyleConfig::GetSymbolNames() const
  {
    std::vector<std::string> names;

    names.reserve(symbols.size());
    for (const auto& entry : symbols) {
      names.push_back(entry.first);
    }

    std::sort(names.begin(),names.end());

    return names;
  }

  std::vector<std::string> StyleConfig::GetPatternNames() const
  {
    std::set<std::string> names;

    for (const auto& typeSelector : areaFillStyleSelectors) {
      for (const auto& levelSelector : typeSelector) {
        for (const auto& selector : levelSelector) {
          if (!selector.style->GetPatternName().empty()) {
            names.insert(selector.style->GetPatternName());
          }
        }
      }
    }

    return std::vector<std::string>(names.begin(),names.end());
  }

  template<class S, class A>
  void GetMaxLevelInConditionals(const std::list<ConditionalStyle<S,A>>& conditionals,
                                 size_t& maxLevel)
  {
    for (const auto& conditional : conditionals) {
      maxLevel=std::max(maxLevel,conditional.filter.GetMinLevel()+1);

      if (conditional.filter.HasMaxLevel()) {
        maxLevel=std::max(maxLevel,conditional.filter.GetMaxLevel()+1);
      }
    }
  }

  /**
   * The bytes the per-level type sets of one family retain, as an estimate: one set per level, each
   * of them holding a pointer per defined type.
   */
  size_t TypeSetBytes(const std::vector<TypeInfoSet>& typeSets,
                      const TypeConfig& typeConfig)
  {
    return typeSets.size()*(sizeof(TypeInfoSet)+typeConfig.GetTypeCount()*sizeof(TypeInfoRef));
  }

  /**
   * The positions of the types the given conditionals name: `positions.positions[typeIndex]` is the
   * dense position of that type among the referenced ones, or `noPosition`, and `positions.count` is
   * the number of referenced types (spec style-configuration, requirement "Style-configuration build
   * cost follows the referenced styles").
   */
  template<class S, class A>
  void CollectReferencedTypes(const TypeConfig& typeConfig,
                              const std::list<ConditionalStyle<S,A>>& conditionals,
                              StyleConfig::LookupPositions& positions)
  {
    positions.positions.assign(typeConfig.GetTypeCount(),
                               StyleConfig::LookupPositions::noPosition);
    positions.count=0;

    for (const auto& conditional : conditionals) {
      for (const auto& type : conditional.filter.GetTypeList()) {
        size_t index=type->GetIndex();

        if (index>=positions.positions.size()) {
          continue;
        }

        if (positions.positions[index]==StyleConfig::LookupPositions::noPosition) {
          positions.positions[index]=static_cast<uint32_t>(positions.count++);
        }
      }
    }
  }

  /**
   * The selectors of a family table for the given type index, or nullptr when the loaded sheet does
   * not reference that type. The table holds one entry per referenced type, so the lookup translates
   * the type index through the family's positions first.
   */
  template<class Table>
  const typename Table::value_type* LookupRow(const Table& table,
                                              const StyleConfig::LookupPositions& positions,
                                              size_t typeIndex)
  {
    if (typeIndex>=positions.positions.size()) {
      return nullptr;
    }

    auto position=positions.positions[typeIndex];

    if (position==StyleConfig::LookupPositions::noPosition ||
        position>=table.size()) {
      return nullptr;
    }

    return &table[position];
  }

  template<class S, class A>
  void CalculateUsedTypes(const std::list<ConditionalStyle<S,A>>& conditionals,
                          size_t maxLevel,
                          std::vector<TypeInfoSet>& typeSets,
                          StyleConfig::BuildDiagnostics& diagnostics)
  {
    for (const auto& conditional : conditionals) {
      size_t minLvl=conditional.filter.GetMinLevel();
      size_t maxLvl=conditional.filter.HasMaxLevel()
                      ? conditional.filter.GetMaxLevel()
                      : (maxLevel>0 ? maxLevel-1 : 0);

      for (const auto& type : conditional.filter.GetTypeList()) {
        diagnostics.typeConditionEvaluations++;

        for (size_t level=minLvl; level<maxLevel && level<=maxLvl; level++) {
          typeSets[level].Set(type);
        }
      }
    }
  }

  template<class S, class A>
  void SortInConditionals(const std::list<ConditionalStyle<S,A>>& conditionals,
                          size_t maxLevel,
                          const StyleConfig::LookupPositions& positions,
                          std::vector<std::vector<std::list<StyleSelector<S,A>>>>& selectors,
                          StyleConfig::BuildDiagnostics& diagnostics)
  {
    selectors.resize(positions.count);

    if (!selectors.empty()) {
      diagnostics.tableBytes+=selectors.size()*sizeof(selectors[0]);
    }

    for (auto& selector : selectors) {
      selector.resize(maxLevel+1);

      if (!selector.empty()) {
        diagnostics.preparedSlots+=selector.size();
        diagnostics.tableBytes+=selector.size()*sizeof(selector[0]);
      }
    }

    for (const auto& conditional : conditionals) {
      StyleSelector<S,A> selector(conditional.filter,conditional.style);

      for (const auto& type : conditional.filter.GetTypeList()) {
        diagnostics.typeConditionEvaluations++;

        if (type->GetIndex()>=positions.positions.size()) {
          continue;
        }

        auto position=positions.positions[type->GetIndex()];

        if (position==StyleConfig::LookupPositions::noPosition) {
          continue;
        }

        size_t minLvl=conditional.filter.GetMinLevel();
        size_t maxLvl=conditional.filter.HasMaxLevel() ? conditional.filter.GetMaxLevel() : maxLevel;

        for (size_t level=minLvl; level<=maxLvl; level++) {
          selectors[position][level].push_back(selector);
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

  template<class S, class A>
  void SortInConditionalsBySlot(const std::list<ConditionalStyle<S,A>>& conditionals,
                                size_t maxLevel,
                                const StyleConfig::LookupPositions& positions,
                                std::vector<std::vector<std::vector<std::list<StyleSelector<S,A>>>>>& selectors,
                                StyleConfig::BuildDiagnostics& diagnostics)
  {
    std::unordered_map<std::string,std::list<ConditionalStyle<S,A>>> styleBySlot;

    for (auto& conditional : conditionals) {
      styleBySlot[conditional.style.style->GetSlot()].push_back(conditional);
    }

    selectors.resize(styleBySlot.size());

    if (!selectors.empty()) {
      diagnostics.tableBytes+=selectors.size()*sizeof(selectors[0]);
    }

    size_t idx=0;

    for (const auto& entry : styleBySlot) {
      SortInConditionals(entry.second,
                         maxLevel,
                         positions,
                         selectors[idx],
                         diagnostics);

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

    CollectReferencedTypes(*typeConfig,
                           nodeTextStyleConditionals,
                           nodeTextStylePositions);
    CollectReferencedTypes(*typeConfig,
                           nodeIconStyleConditionals,
                           nodeIconStylePositions);

    SortInConditionalsBySlot(nodeTextStyleConditionals,
                             maxLevel,
                             nodeTextStylePositions,
                             nodeTextStyleSelectors,
                             buildDiagnostics);

    SortInConditionals(nodeIconStyleConditionals,
                       maxLevel,
                       nodeIconStylePositions,
                       nodeIconStyleSelectors,
                       buildDiagnostics);

    nodeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      nodeTypeSets.emplace_back(*typeConfig);
    }

    CalculateUsedTypes(nodeTextStyleConditionals,
                       maxLevel,
                       nodeTypeSets,
                       buildDiagnostics);
    CalculateUsedTypes(nodeIconStyleConditionals,
                       maxLevel,
                       nodeTypeSets,
                       buildDiagnostics);

    buildDiagnostics.typeSetBytes+=TypeSetBytes(nodeTypeSets,*typeConfig);

    nodeTextStyleConditionals.clear();
    nodeIconStyleConditionals.clear();
  }

  template<class S, class A>
  bool HasStyle(const std::vector<std::vector<std::list<StyleSelector<S,A>>>>& styleSelectors,
                const size_t level)
  {
    for (const auto &selectorsForType: styleSelectors) {
      assert(!selectorsForType.empty());
      if (!selectorsForType[std::min(level, selectorsForType.size() - 1)].empty()) {
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

    CollectReferencedTypes(*typeConfig,
                           wayLineStyleConditionals,
                           wayLineStylePositions);
    CollectReferencedTypes(*typeConfig,
                           wayPathTextStyleConditionals,
                           wayPathTextStylePositions);
    CollectReferencedTypes(*typeConfig,
                           wayPathSymbolStyleConditionals,
                           wayPathSymbolStylePositions);
    CollectReferencedTypes(*typeConfig,
                           wayPathShieldStyleConditionals,
                           wayPathShieldStylePositions);

    SortInConditionalsBySlot(wayLineStyleConditionals,
                             maxLevel,
                             wayLineStylePositions,
                             wayLineStyleSelectors,
                             buildDiagnostics);

    SortInConditionalsBySlot(wayPathSymbolStyleConditionals,
                             maxLevel,
                             wayPathSymbolStylePositions,
                             wayPathSymbolStyleSelectors,
                             buildDiagnostics);

    SortInConditionals(wayPathTextStyleConditionals,
                       maxLevel,
                       wayPathTextStylePositions,
                       wayPathTextStyleSelectors,
                       buildDiagnostics);

    SortInConditionals(wayPathShieldStyleConditionals,
                       maxLevel,
                       wayPathShieldStylePositions,
                       wayPathShieldStyleSelectors,
                       buildDiagnostics);

    wayTypeSets.reserve(maxLevel);
    wayTextFlags.reserve(maxLevel);
    wayShieldFlags.reserve(maxLevel);

    for (size_t level=0; level < maxLevel; level++) {
      wayTypeSets.emplace_back(*typeConfig);
      wayTextFlags.emplace_back(HasStyle(wayPathTextStyleSelectors, level));
      wayShieldFlags.emplace_back(HasStyle(wayPathShieldStyleSelectors, level));
    }

    CalculateUsedTypes(wayLineStyleConditionals,
                       maxLevel,
                       wayTypeSets,
                       buildDiagnostics);

    CalculateUsedTypes(wayPathTextStyleConditionals,
                       maxLevel,
                       wayTypeSets,
                       buildDiagnostics);

    CalculateUsedTypes(wayPathSymbolStyleConditionals,
                       maxLevel,
                       wayTypeSets,
                       buildDiagnostics);

    CalculateUsedTypes(wayPathShieldStyleConditionals,
                       maxLevel,
                       wayTypeSets,
                       buildDiagnostics);

    buildDiagnostics.typeSetBytes+=TypeSetBytes(wayTypeSets,*typeConfig);

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

    CollectReferencedTypes(*typeConfig,
                           areaFillStyleConditionals,
                           areaFillStylePositions);
    CollectReferencedTypes(*typeConfig,
                           areaBorderStyleConditionals,
                           areaBorderStylePositions);
    CollectReferencedTypes(*typeConfig,
                           areaTextStyleConditionals,
                           areaTextStylePositions);
    CollectReferencedTypes(*typeConfig,
                           areaIconStyleConditionals,
                           areaIconStylePositions);
    CollectReferencedTypes(*typeConfig,
                           areaBorderTextStyleConditionals,
                           areaBorderTextStylePositions);
    CollectReferencedTypes(*typeConfig,
                           areaBorderSymbolStyleConditionals,
                           areaBorderSymbolStylePositions);

    SortInConditionals(areaFillStyleConditionals,
                       maxLevel,
                       areaFillStylePositions,
                       areaFillStyleSelectors,
                       buildDiagnostics);

    SortInConditionalsBySlot(areaBorderStyleConditionals,
                             maxLevel,
                             areaBorderStylePositions,
                             areaBorderStyleSelectors,
                             buildDiagnostics);

    // The painter's early visibility decision has to extend an area by at least as much as any per-ring
    // visibility decision can, so collect the widest area border style per level. Iterating the built
    // selectors rather than the conditionals guarantees that the bound covers exactly the styles the
    // per-ring decision can read.
    maxAreaBorderWidthMM.assign(maxLevel,0.0);

    for (const auto& ruleSelectors : areaBorderStyleSelectors) {
      for (const auto& typeSelectors : ruleSelectors) {
        size_t levelCount=std::min(typeSelectors.size(),
                                   maxLevel);

        for (size_t level=0; level<levelCount; level++) {
          double & maxWidth=maxAreaBorderWidthMM.at(level);

          for (const auto& selector : typeSelectors.at(level)) {
            if (selector.style &&
                selector.style->GetWidth()>maxWidth) {
              maxWidth=selector.style->GetWidth();
            }
          }
        }
      }
    }

    SortInConditionalsBySlot(areaTextStyleConditionals,
                             maxLevel,
                             areaTextStylePositions,
                             areaTextStyleSelectors,
                             buildDiagnostics);

    SortInConditionals(areaIconStyleConditionals,
                       maxLevel,
                       areaIconStylePositions,
                       areaIconStyleSelectors,
                       buildDiagnostics);

    SortInConditionals(areaBorderTextStyleConditionals,
                       maxLevel,
                       areaBorderTextStylePositions,
                       areaBorderTextStyleSelectors,
                       buildDiagnostics);

    SortInConditionals(areaBorderSymbolStyleConditionals,
                       maxLevel,
                       areaBorderSymbolStylePositions,
                       areaBorderSymbolStyleSelectors,
                       buildDiagnostics);

    areaTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      areaTypeSets.emplace_back(*typeConfig);
    }

    CalculateUsedTypes(areaFillStyleConditionals,
                       maxLevel,
                       areaTypeSets,
                       buildDiagnostics);
    CalculateUsedTypes(areaBorderStyleConditionals,
                       maxLevel,
                       areaTypeSets,
                       buildDiagnostics);
    CalculateUsedTypes(areaTextStyleConditionals,
                       maxLevel,
                       areaTypeSets,
                       buildDiagnostics);
    CalculateUsedTypes(areaIconStyleConditionals,
                       maxLevel,
                       areaTypeSets,
                       buildDiagnostics);
    CalculateUsedTypes(areaBorderTextStyleConditionals,
                       maxLevel,
                       areaTypeSets,
                       buildDiagnostics);
    CalculateUsedTypes(areaBorderSymbolStyleConditionals,
                       maxLevel,
                       areaTypeSets,
                       buildDiagnostics);

    buildDiagnostics.typeSetBytes+=TypeSetBytes(areaTypeSets,*typeConfig);

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

    CollectReferencedTypes(*typeConfig,
                           routeLineStyleConditionals,
                           routeLineStylePositions);
    CollectReferencedTypes(*typeConfig,
                           routePathTextStyleConditionals,
                           routePathTextStylePositions);

    SortInConditionals(routePathTextStyleConditionals,
                       maxLevel,
                       routePathTextStylePositions,
                       routePathTextStyleSelectors,
                       buildDiagnostics);

    routeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      routeTypeSets.emplace_back(*typeConfig);
    }

    CalculateUsedTypes(routeLineStyleConditionals,
                       maxLevel,
                       routeTypeSets,
                       buildDiagnostics);

    CalculateUsedTypes(routePathTextStyleConditionals,
                       maxLevel,
                       routeTypeSets,
                       buildDiagnostics);

    buildDiagnostics.typeSetBytes+=TypeSetBytes(routeTypeSets,*typeConfig);

    SortInConditionalsBySlot(routeLineStyleConditionals,
                             maxLevel,
                             routeLineStylePositions,
                             routeLineStyleSelectors,
                             buildDiagnostics);

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

  namespace {
    /**
     * Widest visual reach of the line styles of one level. The attributes of all styles of
     * one type of that level are summed, because a resolved line style is the sum of the
     * matching partial styles; the maximum over the types is the bound of the level.
     */
    void UpdateLineReach(const std::vector<std::vector<LineStyleSelectorList>>& selectors,
                         size_t level,
                         VisibilityBounds& bounds)
    {
      for (const auto& selectorsForType : selectors) {
        if (selectorsForType.empty()) {
          continue;
        }

        double lineWidth=0.0;
        double displayWidth=0.0;

        for (const auto& selector : selectorsForType[std::min(level,selectorsForType.size()-1)]) {
          lineWidth+=selector.style->GetWidth();
          displayWidth+=selector.style->GetDisplayWidth();
        }

        bounds.maxWayLineWidth=std::max(bounds.maxWayLineWidth,lineWidth);
        bounds.maxWayDisplayWidth=std::max(bounds.maxWayDisplayWidth,displayWidth);
      }
    }

    /**
     * Widest icon and symbol reach of the icon styles of one level
     */
    void UpdateIconReach(const IconStyleLookupTable& selectors,
                         size_t level,
                         VisibilityBounds& bounds)
    {
      for (const auto& selectorsForType : selectors) {
        if (selectorsForType.empty()) {
          continue;
        }

        for (const auto& selector : selectorsForType[std::min(level,selectorsForType.size()-1)]) {
          const IconStyle& style=*selector.style;

          bounds.maxIconWidth=std::max(bounds.maxIconWidth,static_cast<double>(style.GetWidth()));
          bounds.maxIconHeight=std::max(bounds.maxIconHeight,static_cast<double>(style.GetHeight()));

          const SymbolRef& symbol=style.GetSymbol();

          if (symbol &&
              std::find(bounds.symbols.begin(),bounds.symbols.end(),symbol)==bounds.symbols.end()) {
            bounds.symbols.push_back(symbol);
          }
        }
      }
    }
  }

  void StyleConfig::PostprocessVisibilityBounds()
  {
    size_t levelCount=0;

    for (const auto& selectorsBySlot : wayLineStyleSelectors) {
      for (const auto& selectorsForType : selectorsBySlot) {
        levelCount=std::max(levelCount,selectorsForType.size());
      }
    }

    for (const auto& selectorsForType : nodeIconStyleSelectors) {
      levelCount=std::max(levelCount,selectorsForType.size());
    }

    visibilityBounds.clear();
    visibilityBounds.resize(levelCount);

    for (size_t level=0; level<levelCount; level++) {
      VisibilityBounds& bounds=visibilityBounds[level];

      for (const auto& selectorsBySlot : wayLineStyleSelectors) {
        UpdateLineReach(selectorsBySlot,
                        level,
                        bounds);
      }

      UpdateIconReach(nodeIconStyleSelectors,
                      level,
                      bounds);
    }
  }

  VisibilityBounds StyleConfig::GetVisibilityBounds(const Magnification& magnification) const
  {
    if (visibilityBounds.empty()) {
      return {};
    }

    size_t level=magnification.GetLevel();

    if (level>=visibilityBounds.size()) {
      level=visibilityBounds.size()-1;
    }

    return visibilityBounds[level];
  }

  void StyleConfig::Postprocess()
  {
    buildDiagnostics=BuildDiagnostics();

    PostprocessNodes();
    PostprocessWays();
    PostprocessAreas();
    PostprocessRoutes();

    PostprocessIconId();
    PostprocessPatternId();

    PostprocessVisibilityBounds();
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
  template<class S, class A>
  std::shared_ptr<S> GetFeatureStyle(const StyleResolveContext& context,
                                     const std::vector<std::list<StyleSelector<S,A>>>& styleSelectors,
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
      const auto *row=LookupRow(nodeTextStyleSelector,
                                nodeTextStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      if (level>=row->size()) {
        level=static_cast<uint32_t>(row->size()-1);
      }

      if (!(*row)[level].empty()) {
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
      const auto *row=LookupRow(nodeTextStyleSelector,
                                nodeTextStylePositions,
                                buffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(nodeTextStyleSelector,
                                nodeTextStylePositions,
                                buffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
    const auto *row=LookupRow(nodeIconStyleSelectors,
                              nodeIconStylePositions,
                              buffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
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
      const auto *row=LookupRow(wayLineStyleSelector,
                                wayLineStylePositions,
                                buffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(routeLineStyleSelector,
                                routeLineStylePositions,
                                buffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(wayPathSymbolStyleSelector,
                                wayPathSymbolStylePositions,
                                buffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      PathSymbolStyleRef style=GetFeatureStyle(styleResolveContext,
                                               *row,
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
    const auto *row=LookupRow(wayPathTextStyleSelectors,
                              wayPathTextStylePositions,
                              buffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           buffer,
                           projection);
  }

  bool StyleConfig::HasWayPathTextStyle(const Projection& projection) const
  {
    if (wayTextFlags.empty()) {
      return false;
    }

    size_t level = projection.GetMagnification().GetLevel();

    return wayTextFlags[std::min(level, wayTextFlags.size()-1)];
  }

  PathTextStyleRef StyleConfig::GetRoutePathTextStyle(const FeatureValueBuffer& buffer,
                                                      const Projection& projection) const
  {
    const auto *row=LookupRow(routePathTextStyleSelectors,
                              routePathTextStylePositions,
                              buffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           buffer,
                           projection);
  }

  PathShieldStyleRef StyleConfig::GetWayPathShieldStyle(const FeatureValueBuffer& buffer,
                                                        const Projection& projection) const
  {
    const auto *row=LookupRow(wayPathShieldStyleSelectors,
                              wayPathShieldStylePositions,
                              buffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           buffer,
                           projection);
  }

  bool StyleConfig::HasWayPathShieldStyle(const Projection& projection) const
  {
    if (wayShieldFlags.empty()) {
      return false;
    }

    size_t level = projection.GetMagnification().GetLevel();

    return wayShieldFlags[std::min(level, wayTextFlags.size()-1)];
  }

  double StyleConfig::GetMaxAreaBorderWidthMM(const Magnification& magnification) const
  {
    if (maxAreaBorderWidthMM.empty()) {
      return 0.0;
    }

    size_t level=magnification.GetLevel();

    if (level>=maxAreaBorderWidthMM.size()) {
      level=maxAreaBorderWidthMM.size()-1;
    }

    return maxAreaBorderWidthMM.at(level);
  }

  FillStyleRef StyleConfig::GetAreaFillStyle(const TypeInfoRef& type,
                                             const FeatureValueBuffer& buffer,
                                             const Projection& projection) const
  {
    const auto *row=LookupRow(areaFillStyleSelectors,
                              areaFillStylePositions,
                              type->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
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
      const auto *row=LookupRow(areaBorderStyleSelector,
                                areaBorderStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      BorderStyleRef style=GetFeatureStyle(styleResolveContext,
                                           *row,
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
      const auto *row=LookupRow(areaTextStyleSelector,
                                areaTextStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      if (level>=row->size()) {
        level=static_cast<uint32_t>(row->size()-1);
      }

      if (!(*row)[level].empty()) {
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
      const auto *row=LookupRow(areaTextStyleSelector,
                                areaTextStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(areaTextStyleSelector,
                                areaTextStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      TextStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
    const auto *row=LookupRow(areaIconStyleSelectors,
                              areaIconStylePositions,
                              type->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           buffer,
                           projection);
  }

  PathTextStyleRef StyleConfig::GetAreaBorderTextStyle(const TypeInfoRef& type,
                                                       const FeatureValueBuffer& buffer,
                                                       const Projection& projection) const
  {
    const auto *row=LookupRow(areaBorderTextStyleSelectors,
                              areaBorderTextStylePositions,
                              type->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           buffer,
                           projection);
  }

  PathSymbolStyleRef StyleConfig::GetAreaBorderSymbolStyle(const TypeInfoRef& type,
                                                           const FeatureValueBuffer& buffer,
                                                           const Projection& projection) const
  {
    const auto *row=LookupRow(areaBorderSymbolStyleSelectors,
                              areaBorderSymbolStylePositions,
                              type->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           buffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetLandFillStyle(const Projection& projection) const
  {
    const auto *row=LookupRow(areaFillStyleSelectors,
                              areaFillStylePositions,
                              tileLandBuffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           tileLandBuffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetSeaFillStyle(const Projection& projection) const
  {
    const auto *row=LookupRow(areaFillStyleSelectors,
                              areaFillStylePositions,
                              tileSeaBuffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           tileSeaBuffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetCoastFillStyle(const Projection& projection) const
  {
    const auto *row=LookupRow(areaFillStyleSelectors,
                              areaFillStylePositions,
                              tileCoastBuffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           tileCoastBuffer,
                           projection);
  }

  FillStyleRef StyleConfig::GetUnknownFillStyle(const Projection& projection) const
  {
    const auto *row=LookupRow(areaFillStyleSelectors,
                              areaFillStylePositions,
                              tileUnknownBuffer.GetType()->GetIndex());

    if (row==nullptr) {
      return nullptr;
    }

    return GetFeatureStyle(styleResolveContext,
                           *row,
                           tileUnknownBuffer,
                           projection);
  }

  LineStyleRef StyleConfig::GetCoastlineLineStyle(const Projection& projection) const
  {
    for (const auto& wayLineStyleSelector : wayLineStyleSelectors) {
      const auto *row=LookupRow(wayLineStyleSelector,
                                wayLineStylePositions,
                                coastlineBuffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(wayLineStyleSelector,
                                wayLineStylePositions,
                                osmTileBorderBuffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(wayLineStyleSelector,
                                wayLineStylePositions,
                                osmSubTileBorderBuffer.GetType()->GetIndex());

      if (row==nullptr) {
        continue;
      }

      LineStyleRef style=GetFeatureStyle(styleResolveContext,
                                         *row,
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
      const auto *row=LookupRow(slotEntry,
                                nodeTextStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      size_t l=level;

      if (l>=row->size()) {
        l=row->size()-1;
      }

      for (const auto& selector : (*row)[l]) {
        selectors.push_back(selector);
      }
    }
  }

  void StyleConfig::GetAreaFillStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<FillStyleSelector>& selectors) const
  {
    selectors.clear();

    const auto *row=LookupRow(areaFillStyleSelectors,
                              areaFillStylePositions,
                              type->GetIndex());

    if (row==nullptr) {
      return;
    }

    if (level>=row->size()) {
      level=row->size()-1;
    }

    for (const auto& selector : (*row)[level]) {
      selectors.push_back(selector);
    }
  }

  void StyleConfig::GetAreaTextStyleSelectors(size_t level,
                                              const TypeInfoRef& type,
                                              std::list<TextStyleSelector>& selectors) const
  {
    selectors.clear();

    for (const auto& slotEntry : areaTextStyleSelectors) {
      const auto *row=LookupRow(slotEntry,
                                areaTextStylePositions,
                                type->GetIndex());

      if (row==nullptr) {
        continue;
      }

      size_t l=level;

      if (l>=row->size()) {
        l=row->size()-1;
      }

      for (const auto& selector : (*row)[l]) {
        selectors.push_back(selector);
      }
    }
  }

  bool StyleConfig::LoadContent(const std::string& filename,
                                const std::string& content,
                                ColorPostprocessor colorPostprocessor,
                                bool submodule,
                                Log &log)
  {
    oss::Scanner *scanner=new oss::Scanner((const unsigned char*)content.c_str(),
                                           content.length());
    oss::Parser  *parser=new oss::Parser(scanner,
                                         filename,
                                         *this,
                                         colorPostprocessor,
                                         log);

    parser->Parse();

    bool success=!parser->errors->hasErrors;

    errors.clear();
    warnings.clear();

    for (const auto& err : parser->errors->errors) {
      switch (err.type) {
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
                         bool submodule,
                         Log &log)
  {
    StopClock                                 timer;
    bool                                      success=false;

    log.Debug() << "Opening StyleConfig '" << styleFile << "'...";

    try {
      FILE       * file;
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

      unsigned char * content=new unsigned char[fileSize];

      if (fread(content,1,fileSize,file)!=(size_t)fileSize) {
        log.Error() << "Cannot load file '" << styleFile << "'";
        delete[] content;
        fclose(file);

        return false;
      }

      fclose(file);

      success=LoadContent(styleFile,
                          std::string((const char*)content,fileSize),
                          colorPostprocessor,
                          submodule,
                          log);

      delete[] content;

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
