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

#include <osmscout/Preprocess.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>


#include <string.h>

#include <libxml/parser.h>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

class Preprocessor
{
private:
  const TypeConfig& config;
  std::ofstream     nodeFile;
  std::ofstream     wayFile;
  std::ofstream     relationFile;

public:
  size_t nodeCount;
  size_t wayCount;
  size_t areaCount;
  size_t relationCount;

public:
  Preprocessor(const TypeConfig& config);
  void Process(const Id& id,
               const double& lon, const double& lat,
               const std::vector<Tag>& tags);
  void Process(const Id& id,
               const std::vector<Id>& nodes,
               const std::vector<Tag>& tags);
  void Process(const Id& id,
               const std::vector<RawRelation::Member>& members,
               const std::vector<Tag>& tags);

  void Cleanup();
};

Preprocessor::Preprocessor(const TypeConfig& config)
 : config(config),
   nodeCount(0),
   wayCount(0),
   areaCount(0),
   relationCount(0)
{
  nodeFile.open("rawnodes.dat",std::ios::out|std::ios::trunc|std::ios::binary);
  wayFile.open("rawways.dat",std::ios::out|std::ios::trunc|std::ios::binary);
  relationFile.open("rawrels.dat",std::ios::out|std::ios::trunc|std::ios::binary);
}

void Preprocessor::Process(const Id& id,
                           const double& lon, const double& lat,
                           const std::vector<Tag>& tags)
{
  RawNode                    node;
  std::vector<Tag>::iterator tag;

  node.type=typeIgnore;
  node.id=id;
  node.lon=lon;
  node.lat=lat;
  node.tags=tags;

  if (config.GetNodeTypeId(node.tags,tag,node.type))  {
    node.tags.erase(tag);
  }

  node.Write(nodeFile);
  nodeCount++;
}

void Preprocessor::Process(const Id& id,
                           const std::vector<Id>& nodes,
                           const std::vector<Tag>& tags)
{
  TypeId                     areaType=typeIgnore;
  TypeId                     wayType=typeIgnore;
  std::vector<Tag>           t=tags;
  std::vector<Tag>::iterator wayTag=t.end();
  std::vector<Tag>::iterator areaTag=t.end();
  /*
  int8_t                     layer=0;
  bool                       isBridge=false;
  bool                       isTunnel=false;
  bool                       isBuilding=false;*/
  bool                       isArea=false;
  /*
  bool                       isOneway=false;
  bool                       reverseNodes=false;*/

  config.GetWayAreaTypeId(t,wayTag,wayType,areaTag,areaType);

  if (areaType!=typeIgnore &&
      nodes.size()>1 && nodes[0]==nodes[nodes.size()-1]) {
    isArea=true;
    t.erase(areaTag);
  }
  else if (wayType!=typeIgnore) {
    t.erase(wayTag);
  }


  RawWay way;

  if (isArea) {
    way.type=areaType;
    areaCount++;
  }
  else {
    way.type=wayType;
    wayCount++;
  }

  way.id=id;
  way.isArea=isArea;
  way.nodes=nodes;
  way.tags=t;

  way.Write(wayFile);
}

void Preprocessor::Process(const Id& id,
                           const std::vector<RawRelation::Member>& members,
                           const std::vector<Tag>& tags)
{
  RawRelation                relation;
  std::vector<Tag>::iterator tag;

  relation.type=typeIgnore;
  relation.id=id;
  relation.members=members;
  relation.tags=tags;

  if (config.GetRelationTypeId(relation.tags,tag,relation.type))  {
    relation.tags.erase(tag);
  }

  relation.Write(relationFile);
  relationCount++;
}

void Preprocessor::Cleanup()
{
  nodeFile.close();
  wayFile.close();
  relationFile.close();
}

class Parser
{
  enum Context {
    contextUnknown,
    contextNode,
    contextWay,
    contextRelation
  };

private:
  Context                          context;
  Preprocessor&                    pp;
  const TypeConfig&                typeConfig;
  Id                               id;
  double                           lon,lat;
  std::vector<Tag>                 tags;
  std::vector<Id>                  nodes;
  std::vector<RawRelation::Member> members;

public:
  Parser(Preprocessor& pp,
         const TypeConfig& typeConfig)
  : pp(pp),
    typeConfig(typeConfig)
  {
    tags.reserve(20);

    context=contextUnknown;
  }

  void StartElement(const xmlChar *name, const xmlChar **atts)
  {
    if (strcmp((const char*)name,"node")==0) {
      const xmlChar *idValue=NULL;
      const xmlChar *latValue=NULL;
      const xmlChar *lonValue=NULL;

      context=contextNode;
      tags.clear();

      for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
        if (strcmp((const char*)atts[i],"id")==0) {
          idValue=atts[i+1];
        }
        else if (strcmp((const char*)atts[i],"lat")==0) {
          latValue=atts[i+1];
        }
        else if (strcmp((const char*)atts[i],"lon")==0) {
          lonValue=atts[i+1];
        }
      }

      if (idValue==NULL || lonValue==NULL || latValue==NULL) {
        std::cerr << "Not all required attributes found" << std::endl;
      }

      if (sscanf((const char*)idValue,"%lu",&id)!=1) {
        std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
        return;
      }
      if (sscanf((const char*)latValue,"%lf",&lat)!=1) {
        std::cerr << "Cannot parse latitude: '" << latValue << "'" << std::endl;
        return;
      }
      if (sscanf((const char*)lonValue,"%lf",&lon)!=1) {
        std::cerr << "Cannot parse longitude: '" << lonValue << "'" << std::endl;
        return;
      }
    }
    else if (strcmp((const char*)name,"way")==0) {
      const xmlChar *idValue=NULL;

      context=contextWay;
      nodes.clear();
      members.clear();
      tags.clear();

      for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
        if (strcmp((const char*)atts[i],"id")==0) {
          idValue=atts[i+1];
        }
      }

      if (sscanf((const char*)idValue,"%lu",&id)!=1) {
        std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
        return;
      }
    }
    if (strcmp((const char*)name,"relation")==0) {
      const xmlChar *idValue=NULL;

      context=contextRelation;
      nodes.clear();
      members.clear();
      tags.clear();

      for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
        if (strcmp((const char*)atts[i],"id")==0) {
          idValue=atts[i+1];
        }
      }

      if (sscanf((const char*)idValue,"%lu",&id)!=1) {
        std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
        return;
      }
    }
    else if (strcmp((const char*)name,"tag")==0) {
      if (context!=contextWay && context!=contextNode && context!=contextRelation) {
        return;
      }

      const xmlChar *keyValue=NULL;
      const xmlChar *valueValue=NULL;

      for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
        if (strcmp((const char*)atts[i],"k")==0) {
          keyValue=atts[i+1];
        }
        else if (strcmp((const char*)atts[i],"v")==0) {
          valueValue=atts[i+1];
        }
      }

      if (keyValue==NULL || valueValue==NULL) {
        std::cerr << "Cannot parse tag, skipping..." << std::endl;
        return;
      }

      TagId tagId=typeConfig.GetTagId((const char*)keyValue);

      if (tagId==tagIgnore) {
        return;
      }

      Tag tag;

      tag.key=tagId;
      tag.value=(const char*)valueValue;

      tags.push_back(tag);
    }
    else if (strcmp((const char*)name,"nd")==0) {
      if (context!=contextWay) {
        return;
      }

      Id             node;
      const xmlChar *idValue=NULL;

      for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
        if (strcmp((const char*)atts[i],"ref")==0) {
          idValue=atts[i+1];
        }
      }

      if (sscanf((const char*)idValue,"%lu",&node)!=1) {
        std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
        return;
      }

      nodes.push_back(node);
    }
    else if (strcmp((const char*)name,"member")==0) {
      if (context!=contextRelation) {
        return;
      }

      RawRelation::Member member;
      const xmlChar       *typeValue=NULL;
      const xmlChar       *refValue=NULL;
      const xmlChar       *roleValue=NULL;

      for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
        if (strcmp((const char*)atts[i],"type")==0) {
          typeValue=atts[i+1];
        }
        else if (strcmp((const char*)atts[i],"ref")==0) {
          refValue=atts[i+1];
        }
        else if (strcmp((const char*)atts[i],"role")==0) {
          roleValue=atts[i+1];
        }
      }

      if (strcmp((const char*)typeValue,"node")==0) {
        member.type=RawRelation::memberNode;
      }
      else if (strcmp((const char*)typeValue,"way")==0) {
        member.type=RawRelation::memberWay;
      }
      else if (strcmp((const char*)typeValue,"relation")==0) {
        member.type=RawRelation::memberRelation;
      }
      else {
        std::cerr << "Cannot parse member type: '" << typeValue << "'" << std::endl;
        return;
      }

      if (sscanf((const char*)refValue,"%lu",&member.id)!=1) {
        std::cerr << "Cannot parse ref: '" << refValue << "'" << std::endl;
        return;
      }

      if (roleValue!=NULL) {
        member.role=(const char*)roleValue;
      }

      members.push_back(member);
    }
  }

  void EndElement(const xmlChar *name)
  {
    if (strcmp((const char*)name,"node")==0) {
      pp.Process(id,lon,lat,tags);
      tags.clear();
      context=contextUnknown;
    }
    else if (strcmp((const char*)name,"way")==0) {
      pp.Process(id,nodes,tags);
      nodes.clear();
      tags.clear();
      context=contextUnknown;
    }
    else if (strcmp((const char*)name,"relation")==0) {
      pp.Process(id,members,tags);
      members.clear();
      tags.clear();
      context=contextUnknown;
    }
  }
};

static xmlEntityPtr GetEntity(void* /*data*/, const xmlChar *name)
{
  return xmlGetPredefinedEntity(name);
}

static void StartElement(void *data, const xmlChar *name, const xmlChar **atts)
{
  Parser* parser=static_cast<Parser*>(data);

  parser->StartElement(name,atts);
}

static void EndElement(void *data, const xmlChar *name)
{
  Parser* parser=static_cast<Parser*>(data);

  parser->EndElement(name);
}

static void StructuredErrorHandler(void *data, xmlErrorPtr error)
{
  std::cerr << "XML error, line " << error->line << ": " << error->message << std::endl;
}

bool Preprocess(const char* mapfile,
                const TypeConfig& typeConfig)
{
  Preprocessor  pp(typeConfig);
  Parser        parser(pp,typeConfig);

  xmlSAXHandler saxParser;

  memset(&saxParser,0,sizeof(xmlSAXHandler));
  saxParser.initialized=XML_SAX2_MAGIC;
  saxParser.getEntity=GetEntity;
  saxParser.startElement=StartElement;
  saxParser.endElement=EndElement;
  saxParser.serror=StructuredErrorHandler;

  xmlSAXUserParseFile(&saxParser,&parser,mapfile);

  std::cout << "Nodes: " << pp.nodeCount << std::endl;
  std::cout << "Ways:  " << pp.wayCount << std::endl;
  std::cout << "Areas: " << pp.areaCount << std::endl;
  std::cout << "Relations: " << pp.relationCount << std::endl;

  return true;
}
