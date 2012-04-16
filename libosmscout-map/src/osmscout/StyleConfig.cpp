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

#include <osmscout/StyleConfig.h>

#include <limits>
#include <set>

namespace osmscout {

  LineStyle::LineStyle()
   : lineR(1),
     lineG(1),
     lineB(1),
     lineA(1),
     alternateR(0.5),
     alternateG(0.5),
     alternateB(0.5),
     alternateA(1),
     outlineR(0.75),
     outlineG(0.75),
     outlineB(0.75),
     outlineA(1),
     gapR(1),
     gapG(0),
     gapB(0),
     gapA(0),
     minWidth(1),
     width(0),
     fixedWidth(false),
     outline(0)
  {
    // no code
  }

  LineStyle& LineStyle::SetLineColor(double r, double g, double b, double a)
  {
    lineR=r;
    lineG=g;
    lineB=b;
    lineA=a;

    alternateR=r;
    alternateG=g;
    alternateB=b;
    alternateA=a;

    return *this;
  }

  LineStyle& LineStyle::SetAlternateColor(double r, double g, double b, double a)
  {
    alternateR=r;
    alternateG=g;
    alternateB=b;
    alternateA=a;

    return *this;
  }

  LineStyle& LineStyle::SetOutlineColor(double r, double g, double b, double a)
  {
    outlineR=r;
    outlineG=g;
    outlineB=b;
    outlineA=a;

    return *this;
  }

  LineStyle& LineStyle::SetGapColor(double r, double g, double b, double a)
  {
    gapR=r;
    gapG=g;
    gapB=b;
    gapA=a;

    return *this;
  }

  LineStyle& LineStyle::SetMinWidth(double value)
  {
    minWidth=value;

    return *this;
  }

  LineStyle& LineStyle::SetWidth(double value)
  {
    width=value;

    return *this;
  }

  LineStyle& LineStyle::SetFixedWidth(bool fixedWidth)
  {
    this->fixedWidth=fixedWidth;

    return *this;
  }

  LineStyle& LineStyle::SetOutline(double value)
  {
    outline=value;

    return *this;
  }

  LineStyle& LineStyle::AddDashValue(double dashValue)
  {
    dash.push_back(dashValue);

    return *this;
  }


  FillStyle::FillStyle()
   : style(none),
     fillR(1),
     fillG(0),
     fillB(0),
     fillA(1),
     patternId(0),
     patternMinMag(magWorld),
     borderR(1),
     borderG(0),
     borderB(0),
     borderWidth(0.0)
  {
    // no code
  }

  FillStyle& FillStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  FillStyle& FillStyle::SetFillColor(double r, double g, double b, double a)
  {
    fillR=r;
    fillG=g;
    fillB=b;
    fillA=a;

    return *this;
  }

  void FillStyle::SetPatternId(size_t id) const
  {
    patternId=id;
  }

  FillStyle& FillStyle::SetPattern(const std::string& pattern)
  {
    this->pattern=pattern;

    return *this;
  }

  FillStyle& FillStyle::SetPatternMinMag(Mag mag)
  {
    patternMinMag=mag;

    return *this;
  }

  FillStyle& FillStyle::SetBorderColor(double r, double g, double b, double a)
  {
    borderR=r;
    borderG=g;
    borderB=b;
    borderA=a;

    return *this;
  }

  FillStyle& FillStyle::SetBorderWidth(double value)
  {
    borderWidth=value;

    return *this;
  }

  FillStyle& FillStyle::AddBorderDashValue(double dashValue)
  {
    borderDash.push_back(dashValue);

    return *this;
  }

  LabelStyle::LabelStyle()
   : style(none),
     priority(0),
     minMag(magWorld),
     scaleAndFadeMag((Mag)1000000),
     maxMag((Mag)1000000),
     size(1),
     textR(0),
     textG(0),
     textB(0),
     textA(1),
     bgR(1),
     bgG(1),
     bgB(1),
     bgA(1),
     borderR(0),
     borderG(0),
     borderB(0),
     borderA(1)
  {
    // no code
  }

  LabelStyle& LabelStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  LabelStyle& LabelStyle::SetPriority(uint8_t priority)
  {
    this->priority=priority;

    return *this;
  }

  LabelStyle& LabelStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  LabelStyle& LabelStyle::SetScaleAndFadeMag(Mag mag)
  {
    this->scaleAndFadeMag=mag;

    return *this;
  }

  LabelStyle& LabelStyle::SetMaxMag(Mag mag)
  {
    this->maxMag=mag;

    return *this;
  }

  LabelStyle& LabelStyle::SetSize(double size)
  {
    this->size=size;

    return *this;
  }

  LabelStyle& LabelStyle::SetTextColor(double r, double g, double b, double a)
  {
    this->textR=r;
    this->textG=g;
    this->textB=b;
    this->textA=a;

    return *this;
  }

  LabelStyle& LabelStyle::SetBgColor(double r, double g, double b, double a)
  {
    this->bgR=r;
    this->bgG=g;
    this->bgB=b;
    this->bgA=a;

    return *this;
  }

  LabelStyle& LabelStyle::SetBorderColor(double r, double g, double b, double a)
  {
    this->borderR=r;
    this->borderG=g;
    this->borderB=b;
    this->borderA=a;

    return *this;
  }

  SymbolStyle::SymbolStyle()
   : style(none),
     minMag(magWorld),
     size(8),
     fillR(1),
     fillG(0),
     fillB(0),
     fillA(1)
  {
    // no code
  }

  SymbolStyle& SymbolStyle::SetStyle(Style style)
  {
    this->style=style;

    return *this;
  }

  SymbolStyle& SymbolStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  SymbolStyle& SymbolStyle::SetSize(double size)
  {
    this->size=size;

    return *this;
  }

  SymbolStyle& SymbolStyle::SetFillColor(double r, double g, double b, double a)
  {
    fillR=r;
    fillG=g;
    fillB=b;
    fillA=a;

    return *this;
  }

  IconStyle::IconStyle()
   : id(0),
     minMag(magWorld)
  {
    // no code
  }

  IconStyle& IconStyle::SetId(size_t id)
  {
    this->id=id;

    return *this;
  }

  IconStyle& IconStyle::SetIconName(const std::string& iconName)
  {
    this->iconName=iconName;

    return *this;
  }

  IconStyle& IconStyle::SetMinMag(Mag mag)
  {
    this->minMag=mag;

    return *this;
  }

  StyleConfig::StyleConfig(TypeConfig* typeConfig)
   : typeConfig(typeConfig)
  {
    // no code
  }

  StyleConfig::~StyleConfig()
  {
    for (size_t i=0; i<nodeSymbolStyles.size(); i++) {
      delete nodeSymbolStyles[i];
    }

    for (size_t i=0; i<nodeRefLabelStyles.size(); i++) {
      delete nodeRefLabelStyles[i];
    }

    for (size_t i=0; i<nodeLabelStyles.size(); i++) {
      delete nodeLabelStyles[i];
    }

    for (size_t i=0; i<nodeIconStyles.size(); i++) {
      delete nodeIconStyles[i];
    }

    for (size_t i=0; i<wayLineStyles.size(); i++) {
      delete wayLineStyles[i];
    }

    for (size_t i=0; i<wayRefLabelStyles.size(); i++) {
      delete wayRefLabelStyles[i];
    }

    for (size_t i=0; i<wayNameLabelStyles.size(); i++) {
      delete wayNameLabelStyles[i];
    }

    for (size_t i=0; i<areaFillStyles.size(); i++) {
      delete areaFillStyles[i];
    }

    for (size_t i=0; i<areaSymbolStyles.size(); i++) {
      delete areaSymbolStyles[i];
    }

    for (size_t i=0; i<areaLabelStyles.size(); i++) {
      delete areaLabelStyles[i];
    }

    for (size_t i=0; i<areaIconStyles.size(); i++) {
      delete areaIconStyles[i];
    }
  }

  void StyleConfig::ReserveSpaceForNodeType(TypeId type)
  {
    if (type>=nodeLabelStyles.size()) {
      nodeSymbolStyles.resize(type+1,NULL);
      nodeRefLabelStyles.resize(type+1,NULL);
      nodeLabelStyles.resize(type+1,NULL);
      nodeIconStyles.resize(type+1,NULL);
    }
  }

  void StyleConfig::ReserveSpaceForWayType(TypeId type)
  {
    if (type>=wayLineStyles.size()) {
      wayPrio.resize(type+1,std::numeric_limits<size_t>::max());
      wayMag.resize(type+1,magVeryClose);
      wayLineStyles.resize(type+1);
      wayRefLabelStyles.resize(type+1);
      wayNameLabelStyles.resize(type+1);
    }
  }

  void StyleConfig::ReserveSpaceForAreaType(TypeId type)
  {
    if (type>=areaFillStyles.size()) {
      areaMag.resize(type+1,magWorld);
      areaFillStyles.resize(type+1,NULL);
      areaSymbolStyles.resize(type+1,NULL);
      areaLabelStyles.resize(type+1,NULL);
      areaIconStyles.resize(type+1,NULL);
    }
  }

  void StyleConfig::PostprocessNodes()
  {
    size_t maxLevel=0;
    for (size_t type=0; type<nodeLabelStyles.size(); type++) {
      if (nodeLabelStyles[type]!=NULL) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeLabelStyles[type]->GetMinMag()));
      }
      else if (nodeRefLabelStyles[type]!=NULL) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeRefLabelStyles[type]->GetMinMag()));
      }
      else if (nodeSymbolStyles[type]!=NULL) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeSymbolStyles[type]->GetMinMag()));
      }
      else if (nodeIconStyles[type]!=NULL) {
        maxLevel=std::max(maxLevel,MagToLevel(nodeIconStyles[type]->GetMinMag()));
      }
    }

    nodeTypeSets.reserve(maxLevel);

    for (size_t type=0; type<maxLevel; type++) {
      nodeTypeSets.push_back(TypeSet(*typeConfig));
    }

    for (size_t level=0;
        level<nodeTypeSets.size();
        ++level) {
      for (size_t type=0; type<nodeLabelStyles.size(); type++) {
        if (nodeLabelStyles[type]!=NULL &&
            MagToLevel(nodeLabelStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }

        if (nodeRefLabelStyles[type]!=NULL &&
            MagToLevel(nodeRefLabelStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }

        if (nodeSymbolStyles[type]!=NULL &&
            MagToLevel(nodeSymbolStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }

        if (nodeIconStyles[type]!=NULL &&
            MagToLevel(nodeIconStyles[type]->GetMinMag())<=level) {
          nodeTypeSets[level].SetType(type);
        }
      }
    }
  }

  void StyleConfig::PostprocessWays()
  {
    size_t maxLevel=0;
    for (size_t i=0; i<wayLineStyles.size(); i++) {
      maxLevel=std::max(maxLevel,MagToLevel(wayMag[i]));
    }

    wayTypeSets.resize(maxLevel+1);

    std::set<size_t> prios;

    for (size_t i=0; i<wayLineStyles.size(); i++) {
      prios.insert(wayPrio[i]);
    }

    for (size_t level=0;
        level<wayTypeSets.size();
        ++level) {
      for (std::set<size_t>::const_iterator prio=prios.begin();
          prio!=prios.end();
          ++prio) {
        TypeSet typeSet(*typeConfig);

        for (size_t i=0; i<wayLineStyles.size(); i++) {
          if (wayPrio[i]==*prio &&
              MagToLevel(wayMag[i])<=level) {
            typeSet.SetType(i);
          }
        }

        // TODO: Is type set not empty?
        wayTypeSets[level].push_back(typeSet);
      }
    }
  }

  void StyleConfig::PostprocessAreas()
  {
    size_t maxLevel=0;
    for (size_t i=0; i<areaFillStyles.size(); i++) {
      maxLevel=std::max(maxLevel,MagToLevel(areaMag[i]));
    }

    areaTypeSets.reserve(maxLevel);

    for (size_t i=0; i<maxLevel; i++) {
      areaTypeSets.push_back(TypeSet(*typeConfig));
    }

    for (size_t level=0;
        level<areaTypeSets.size();
        ++level) {
      for (size_t i=0; i<areaFillStyles.size(); i++) {
        if (MagToLevel(areaMag[i])<=level) {
          areaTypeSets[level].SetType(i);
        }
      }
    }
  }

  void StyleConfig::Postprocess()
  {
    PostprocessNodes();
    PostprocessWays();
    PostprocessAreas();
  }

  TypeConfig* StyleConfig::GetTypeConfig() const
  {
    return typeConfig;
  }

  StyleConfig& StyleConfig::SetNodeSymbolStyle(TypeId type,
                                               const SymbolStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeSymbolStyles[type];
    nodeSymbolStyles[type]=new SymbolStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeLabelStyle(TypeId type,
                                              const LabelStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeLabelStyles[type];
    nodeLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeRefLabelStyle(TypeId type,
                                                 const LabelStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeRefLabelStyles[type];
    nodeRefLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetNodeIconStyle(TypeId type,
                                             const IconStyle& style)
  {
    ReserveSpaceForNodeType(type);

    delete nodeIconStyles[type];
    nodeIconStyles[type]=new IconStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayPrio(TypeId type, size_t prio)
  {
    ReserveSpaceForWayType(type);

    wayPrio[type]=prio;

    return *this;
  }

  StyleConfig& StyleConfig::SetWayMag(TypeId type, Mag mag)
  {
    ReserveSpaceForWayType(type);

    wayMag[type]=mag;

    return *this;
  }

  StyleConfig& StyleConfig::SetWayLineStyle(TypeId type,
                                            const LineStyle& style)
  {
    ReserveSpaceForWayType(type);

    delete wayLineStyles[type];
    wayLineStyles[type]=new LineStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayRefLabelStyle(TypeId type,
                                                const LabelStyle& style)
  {
    ReserveSpaceForWayType(type);

    delete wayRefLabelStyles[type];
    wayRefLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetWayNameLabelStyle(TypeId type,
                                                 const LabelStyle& style)
  {
    ReserveSpaceForWayType(type);

    delete wayNameLabelStyles[type];
    wayNameLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaMag(TypeId type, Mag mag)
  {
    ReserveSpaceForAreaType(type);

    areaMag[type]=mag;

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaFillStyle(TypeId type,
                                             const FillStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaFillStyles[type];
    areaFillStyles[type]=new FillStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaLabelStyle(TypeId type,
                                              const LabelStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaLabelStyles[type];
    areaLabelStyles[type]=new LabelStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaSymbolStyle(TypeId type,
                                               const SymbolStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaSymbolStyles[type];
    areaSymbolStyles[type]=new SymbolStyle(style);

    return *this;
  }

  StyleConfig& StyleConfig::SetAreaIconStyle(TypeId type,
                                             const IconStyle& style)
  {
    ReserveSpaceForAreaType(type);

    delete areaIconStyles[type];
    areaIconStyles[type]=new IconStyle(style);

    return *this;
  }

  void StyleConfig::GetNodeTypesWithMaxMag(double maxMag,
                                           TypeSet& types) const
  {
    types=nodeTypeSets[std::min(MagToLevel(maxMag),nodeTypeSets.size()-1)];
  }

  void StyleConfig::GetWayTypesByPrioWithMaxMag(double maxMag,
                                             std::vector<TypeSet>& types) const
  {
    types=wayTypeSets[std::min(MagToLevel(maxMag),wayTypeSets.size()-1)];
  }

  void StyleConfig::GetAreaTypesWithMaxMag(double maxMag,
                                           TypeSet& types) const
  {
    types=areaTypeSets[std::min(MagToLevel(maxMag),areaTypeSets.size()-1)];
  }
}

