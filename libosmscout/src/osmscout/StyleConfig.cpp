/*
  Import/TravelJinni - Openstreetmap offline viewer
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

LineStyle::LineStyle()
 : style(none),
   lineR(1),
   lineG(1),
   lineB(1),
   lineA(1),
   outlineR(0.5),
   outlineG(0.5),
   outlineB(0.5),
   outlineA(0.5),
   minPixel(2),
   maxPixel(2),
   width(5),
   outline(0)
{
  // no code
}

LineStyle& LineStyle::SetStyle(Style style)
{
  this->style=style;

  return *this;
}

LineStyle& LineStyle::SetLineColor(double r, double g, double b, double a)
{
  lineR=r;
  lineG=g;
  lineB=b;
  lineA=a;

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

LineStyle& LineStyle::SetMinPixel(double value)
{
  minPixel=value;

  return *this;
}

LineStyle& LineStyle::SetMaxPixel(double value)
{
  maxPixel=value;

  return *this;
}

LineStyle& LineStyle::SetWidth(double value)
{
  width=value;

  return *this;
}

LineStyle& LineStyle::SetOutline(double value)
{
  outline=value;

  return *this;
}



FillStyle::FillStyle()
 : style(none),
   layer(0),
   fillR(1),
   fillG(0),
   fillB(0),
   fillA(1)
{
  // no code
}

FillStyle& FillStyle::SetStyle(Style style)
{
  this->style=style;

  return *this;
}

FillStyle& FillStyle::SetLayer(int layer)
{
  this->layer=layer;

  return *this;
}

FillStyle& FillStyle::SetColor(double r, double g, double b, double a)
{
  fillR=r;
  fillG=g;
  fillB=b;
  fillA=a;

  return *this;
}

LabelStyle::LabelStyle()
 : style(none),
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

StyleConfig::StyleConfig(TypeConfig* typeConfig)
 : typeConfig(typeConfig)
{
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

  for (size_t i=0; i<areaBuildingFillStyles.size(); i++) {
    delete areaBuildingFillStyles[i];
  }

  for (size_t i=0; i<areaSymbolStyles.size(); i++) {
    delete areaSymbolStyles[i];
  }

  for (size_t i=0; i<areaLabelStyles.size(); i++) {
    delete areaLabelStyles[i];
  }
}

void StyleConfig::Postprocess()
{
  std::set<size_t > prios;
  priorities.clear();

  for (size_t i=0; i<areaFillStyles.size() && i<areaPrio.size(); i++) {
    if (areaFillStyles[i]!=NULL || areaBuildingFillStyles[i]!=NULL) {
      prios.insert(areaPrio[i]);
    }
  }

  for (size_t i=0; i<wayLineStyles.size() && i<wayPrio.size(); i++) {
    if (wayLineStyles[i]!=NULL) {
      prios.insert(wayPrio[i]);
    }
  }

  priorities.reserve(prios.size());
  for (std::set<size_t>::const_iterator prio=prios.begin();
       prio!=prios.end();
       ++prio) {
    priorities.push_back(*prio);
  }

  std::set<Mag> magnifications;
  for (size_t i=0; i<nodeSymbolStyles.size(); i++) {
    if (nodeLabelStyles[i]!=NULL) {
      magnifications.insert(nodeLabelStyles[i]->GetMinMag());
    }

    if (nodeSymbolStyles[i]!=NULL) {
      magnifications.insert(nodeSymbolStyles[i]->GetMinMag());
    }
  }

  std::map<Mag,std::set<TypeId> > nodeTypesByMag;

  for (std::set<Mag>::const_iterator mag=magnifications.begin();
       mag!=magnifications.end();
       ++mag) {
    for (size_t i=0; i<nodeSymbolStyles.size() || i<nodeLabelStyles.size(); i++) {
      if (nodeLabelStyles[i]!=NULL && *mag>=nodeLabelStyles[i]->GetMinMag()) {
        nodeTypesByMag[*mag].insert(i);
      }
      if (nodeSymbolStyles[i]!=NULL && *mag>=nodeSymbolStyles[i]->GetMinMag()) {
        nodeTypesByMag[*mag].insert(i);
      }
    }
  }
}

TypeConfig* StyleConfig::GetTypeConfig() const
{
  return typeConfig;
}

StyleConfig& StyleConfig::SetWayPrio(TypeId type, size_t prio)
{
  if (type>=wayPrio.size()) {
    wayPrio.resize(type+1);
    wayLineStyles.resize(type+1);
    wayRefLabelStyles.resize(type+1);
    wayNameLabelStyles.resize(type+1);
  }

  wayPrio[type]=prio;

  return *this;
}

StyleConfig& StyleConfig::SetAreaPrio(TypeId type, size_t prio)
{
  if (type>=areaPrio.size()) {
    areaPrio.resize(type+1);
    areaFillStyles.resize(type+1);
    areaBuildingFillStyles.resize(type+1);
    areaSymbolStyles.resize(type+1);
    areaLabelStyles.resize(type+1);
    areaBorderStyles.resize(type+1);
  }

  areaPrio[type]=prio;

  return *this;
}

StyleConfig& StyleConfig::SetNodeSymbolStyle(TypeId type, const SymbolStyle& style)
{
  if (type>=nodeSymbolStyles.size()) {
    nodeSymbolStyles.resize(type+1,NULL);
    nodeRefLabelStyles.resize(type+1,NULL);
    nodeLabelStyles.resize(type+1,NULL);
  }

  SymbolStyle *s=new SymbolStyle();
  *s=style;

  nodeSymbolStyles[type]=s;

  return *this;
}

StyleConfig& StyleConfig::SetNodeLabelStyle(TypeId type, const LabelStyle& style)
{
  if (type>=nodeSymbolStyles.size()) {
    nodeSymbolStyles.resize(type+1,NULL);
    nodeRefLabelStyles.resize(type+1,NULL);
    nodeLabelStyles.resize(type+1,NULL);
  }

  LabelStyle *l=new LabelStyle();
  *l=style;

  nodeLabelStyles[type]=l;

  return *this;
}

StyleConfig& StyleConfig::SetNodeRefLabelStyle(TypeId type, const LabelStyle& style)
{
  if (type>=nodeSymbolStyles.size()) {
    nodeSymbolStyles.resize(type+1,NULL);
    nodeRefLabelStyles.resize(type+1,NULL);
    nodeLabelStyles.resize(type+1,NULL);
  }

  LabelStyle *l=new LabelStyle();
  *l=style;

  nodeRefLabelStyles[type]=l;

  return *this;
}

StyleConfig& StyleConfig::SetWayLineStyle(TypeId type, const LineStyle& style)
{
  if (type>=wayPrio.size()) {
    wayPrio.resize(type+1,10000); // TODO: max(size_t)
    wayLineStyles.resize(type+1,NULL);
    wayRefLabelStyles.resize(type+1,NULL);
    wayNameLabelStyles.resize(type+1,NULL);
  }

  LineStyle *l=new LineStyle();
  *l=style;

  wayLineStyles[type]=l;

  return *this;
}

StyleConfig& StyleConfig::SetWayRefLabelStyle(TypeId type, const LabelStyle& style)
{
  if (type>=wayPrio.size()) {
    wayPrio.resize(type+1,10000); // TODO: max(size_t)
    wayLineStyles.resize(type+1,NULL);
    wayRefLabelStyles.resize(type+1,NULL);
    wayNameLabelStyles.resize(type+1,NULL);
  }

  LabelStyle *l=new LabelStyle();
  *l=style;

  wayRefLabelStyles[type]=l;

  return *this;
}

StyleConfig& StyleConfig::SetWayNameLabelStyle(TypeId type, const LabelStyle& style)
{
  if (type>=wayPrio.size()) {
    wayPrio.resize(type+1,10000); // TODO: max(size_t)
    wayLineStyles.resize(type+1,NULL);
    wayRefLabelStyles.resize(type+1,NULL);
    wayNameLabelStyles.resize(type+1,NULL);
  }

  LabelStyle *l=new LabelStyle();
  *l=style;

  wayNameLabelStyles[type]=l;

  return *this;
}

StyleConfig& StyleConfig::SetAreaFillStyle(TypeId type, const FillStyle& style)
{
  if (type>=areaPrio.size()) {
    areaPrio.resize(type+1,10000); // TODO: max(size_t)
    areaFillStyles.resize(type+1,NULL);
    areaBuildingFillStyles.resize(type+1,NULL);
    areaSymbolStyles.resize(type+1,NULL);
    areaLabelStyles.resize(type+1,NULL);
    areaBorderStyles.resize(type+1,NULL);
  }

  FillStyle *f=new FillStyle();
  *f=style;

  areaFillStyles[type]=f;

  return *this;
}

StyleConfig& StyleConfig::SetAreaBuildingFillStyle(TypeId type, const FillStyle& style)
{
  if (type>=areaPrio.size()) {
    areaPrio.resize(type+1,10000); // TODO: max(size_t)
    areaFillStyles.resize(type+1,NULL);
    areaBuildingFillStyles.resize(type+1,NULL);
    areaSymbolStyles.resize(type+1,NULL);
    areaLabelStyles.resize(type+1,NULL);
    areaBorderStyles.resize(type+1,NULL);
  }

  FillStyle *f=new FillStyle();
  *f=style;

  areaBuildingFillStyles[type]=f;

  return *this;
}

StyleConfig& StyleConfig::SetAreaLabelStyle(TypeId type, const LabelStyle& style)
{
  if (type>=areaPrio.size()) {
    areaPrio.resize(type+1,10000); // TODO: max(size_t)
    areaFillStyles.resize(type+1,NULL);
    areaBuildingFillStyles.resize(type+1,NULL);
    areaSymbolStyles.resize(type+1,NULL);
    areaLabelStyles.resize(type+1,NULL);
    areaBorderStyles.resize(type+1,NULL);
  }

  LabelStyle *l=new LabelStyle();
  *l=style;

  areaLabelStyles[type]=l;

  return *this;
}

StyleConfig& StyleConfig::SetAreaSymbolStyle(TypeId type, const SymbolStyle& style)
{
  if (type>=areaPrio.size()) {
    areaPrio.resize(type+1,10000); // TODO: max(size_t)
    areaFillStyles.resize(type+1,NULL);
    areaBuildingFillStyles.resize(type+1,NULL);
    areaSymbolStyles.resize(type+1,NULL);
    areaLabelStyles.resize(type+1,NULL);
    areaBorderStyles.resize(type+1,NULL);
  }

  SymbolStyle *s=new SymbolStyle();
  *s=style;

  areaSymbolStyles[type]=s;

  return *this;
}

StyleConfig& StyleConfig::SetAreaBorderStyle(TypeId type, const LineStyle& style)
{
  if (type>=areaPrio.size()) {
    areaPrio.resize(type+1,10000); // TODO: max(size_t)
    areaFillStyles.resize(type+1,NULL);
    areaBuildingFillStyles.resize(type+1,NULL);
    areaSymbolStyles.resize(type+1,NULL);
    areaLabelStyles.resize(type+1,NULL);
    areaBorderStyles.resize(type+1,NULL);
  }

  LineStyle *s=new LineStyle();
  *s=style;

  areaBorderStyles[type]=s;

  return *this;
}

size_t StyleConfig::GetStyleCount() const
{
  size_t result=0;

  result=std::max(result,nodeSymbolStyles.size());
  result=std::max(result,wayLineStyles.size());
  result=std::max(result,areaFillStyles.size());

  return result;
}

void StyleConfig::GetWayTypesWithPrio(size_t prio, std::set<TypeId>& types) const
{
  for (size_t i=0; i<areaFillStyles.size() && i<areaPrio.size(); i++) {
    if ((areaFillStyles[i]!=NULL && areaPrio[i]==prio) ||
        (areaBuildingFillStyles[i]!=NULL && areaPrio[i]==prio)) {
      types.insert(i);
    }
  }

  for (size_t i=0; i<wayLineStyles.size() && i<wayPrio.size(); i++) {
    if (wayLineStyles[i]!=NULL && wayPrio[i]==prio) {
      types.insert(i);
    }
  }
}

void StyleConfig::GetWayTypesWithMaxPrio(size_t prio, std::set<TypeId>& types) const
{
  for (size_t i=0; i<areaFillStyles.size() && i<areaPrio.size(); i++) {
    if ((areaFillStyles[i]!=NULL && areaPrio[i]<=prio) ||
        (areaBuildingFillStyles[i]!=NULL && areaPrio[i]<=prio)) {
      types.insert(i);
    }
  }

  for (size_t i=0; i<wayLineStyles.size() && i<wayPrio.size(); i++) {
    if (wayLineStyles[i]!=NULL && wayPrio[i]<=prio) {
      types.insert(i);
    }
  }
}

void StyleConfig::GetNodeTypesWithMag(double mag, std::set<TypeId>& types) const
{
  for (size_t i=0; i<nodeSymbolStyles.size(); i++) {
    if (nodeLabelStyles[i]!=NULL && mag>=nodeLabelStyles[i]->GetMinMag()) {
      types.insert(i);
    }
    else if (nodeSymbolStyles[i]!=NULL && mag>=nodeSymbolStyles[i]->GetMinMag()) {
      types.insert(i);
    }
  }
}

/**
  Returns a sorted array (high priority with low numerical value to low priority with
  high numerical value) of used priorities.

  Example: [1,2,3,4,5,6,7,8,9,10,11,20]
  */
void StyleConfig::GetPriorities(std::vector<size_t>& priorities) const
{
  priorities=this->priorities;
}

