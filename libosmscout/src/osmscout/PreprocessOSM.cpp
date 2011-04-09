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

#include <osmscout/PreprocessOSM.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <string.h>

#include <libxml/parser.h>

#include <osmscout/RawNode.h>
#include <osmscout/RawRelation.h>
#include <osmscout/RawWay.h>

#include <osmscout/Util.h>

#include <osmscout/util/String.h>

namespace osmscout {

  class Preprocessor
  {
  private:
    const TypeConfig& config;
    FileWriter        nodeWriter;
    FileWriter        wayWriter;
    FileWriter        relationWriter;

    std::vector<Tag>  tags;

  public:
    uint32_t nodeCount;
    uint32_t wayCount;
    uint32_t areaCount;
    uint32_t relationCount;

  public:
    Preprocessor(const ImportParameter& parameter,
                 const TypeConfig& config);
    virtual ~Preprocessor();

    void Process(const Id& id,
                 const double& lon, const double& lat,
                 const std::map<TagId,std::string>& tags);
    void Process(const Id& id,
                 std::vector<Id>& nodes,
                 const std::map<TagId,std::string>& tags);
    void Process(const Id& id,
                 const std::vector<RawRelation::Member>& members,
                 const std::map<TagId,std::string>& tags);

    void Cleanup();
  };

  Preprocessor::Preprocessor(const ImportParameter& parameter,
                             const TypeConfig& config)
   : config(config),
     nodeCount(0),
     wayCount(0),
     areaCount(0),
     relationCount(0)
  {
    nodeWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                    "rawnodes.dat"));
    nodeWriter.Write(nodeCount);

    wayWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                   "rawways.dat"));
    wayWriter.Write(wayCount+areaCount);

    relationWriter.Open(AppendFileToDir(parameter.GetDestinationDirectory(),
                                        "rawrels.dat"));
    relationWriter.Write(relationCount);
  }

  Preprocessor::~Preprocessor()
  {
    // no code
  }

  void Preprocessor::Process(const Id& id,
                             const double& lon, const double& lat,
                             const std::map<TagId,std::string>& tagMap)
  {
    RawNode node;
    TypeId  type=typeIgnore;

    config.GetNodeTypeId(tagMap,type);
    config.ResolveTags(tagMap,tags);

    node.SetId(id);
    node.SetType(type);
    node.SetCoordinates(lon,lat);
    node.SetTags(tags);

    node.Write(nodeWriter);
    nodeCount++;
  }

  void Preprocessor::Process(const Id& id,
                             std::vector<Id>& nodes,
                             const std::map<TagId,std::string>& tagMap)
  {
    TypeId                                      areaType=typeIgnore;
    TypeId                                      wayType=typeIgnore;
    int                                         isArea=0; // 0==unknown, 1==true, -1==false
    std::map<TagId,std::string>::const_iterator areaTag;
    RawWay                                      way;

    way.SetId(id);

    areaTag=tagMap.find(config.tagArea);

    if (areaTag==tagMap.end()) {
      isArea=0;
    }
    else if (areaTag->second=="yes" ||
             areaTag->second=="true" ||
             areaTag->second=="1") {
      isArea=1;
    }
    else {
      isArea=-1;
    }

    config.GetWayAreaTypeId(tagMap,wayType,areaType);
    config.ResolveTags(tagMap,tags);

    if (isArea==1 &&
        areaType==typeIgnore) {
      isArea=0;
    }
    else if (isArea==-1 &&
             wayType==typeIgnore) {
      isArea=0;
    }

    if (isArea==0) {
      if (areaType!=typeIgnore &&
          nodes.size()>1 &&
          nodes[0]==nodes[nodes.size()-1]) {
        isArea=1;
      }
      else if (wayType!=typeIgnore) {
        isArea=-1;
      }
      else if (areaType!=typeIgnore &&
               nodes.size()>1 &&
               wayType==typeIgnore) {

        nodes.push_back(nodes[0]);
        isArea=1;
      }
    }

    if (isArea==1) {
      way.SetType(areaType,true);
      areaCount++;
    }
    else if (isArea==-1) {
      way.SetType(wayType,false);
      wayCount++;
    }
    else {
      way.SetType(typeIgnore,false);
      wayCount++;
      // Unidentified way
      /*
      std::cout << "--- " << id << std::endl;
      for (size_t tag=0; tag<tags.size(); tag++) {
        std::cout << tags[tag].key << "/" << tags[tag].value << std::endl;
      }*/
    }

    way.SetNodes(nodes);
    way.SetTags(tags);

    way.Write(wayWriter);
  }

  void Preprocessor::Process(const Id& id,
                             const std::vector<RawRelation::Member>& members,
                             const std::map<TagId,std::string>& tagMap)
  {
    RawRelation relation;
    TypeId      type;

    relation.SetId(id);
    relation.members=members;

    config.GetRelationTypeId(tagMap,type);
    config.ResolveTags(tagMap,tags);

    relation.SetType(type);

    relation.Write(relationWriter);
    relationCount++;
  }

  void Preprocessor::Cleanup()
  {
    nodeWriter.SetPos(0);
    nodeWriter.Write(nodeCount);

    wayWriter.SetPos(0);
    wayWriter.Write(wayCount+areaCount);

    relationWriter.SetPos(0);
    relationWriter.Write(relationCount);

    nodeWriter.Close();
    wayWriter.Close();
    relationWriter.Close();
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
    std::map<TagId,std::string>      tags;
    std::vector<Id>                  nodes;
    std::vector<RawRelation::Member> members;

  public:
    Parser(Preprocessor& pp,
           const TypeConfig& typeConfig)
    : pp(pp),
      typeConfig(typeConfig)
    {
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

        if (!StringToNumber((const char*)idValue,id)) {
          std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
          return;
        }
        if (!StringToNumber((const char*)latValue,lat)) {
          std::cerr << "Cannot parse latitude: '" << latValue << "'" << std::endl;
          return;
        }
        if (!StringToNumber((const char*)lonValue,lon)) {
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

        if (!StringToNumber((const char*)idValue,id)) {
          std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
          return;
        }
      }
      if (strcmp((const char*)name,"relation")==0) {
        const xmlChar *idValue=NULL;

        context=contextRelation;
        tags.clear();
        nodes.clear();
        members.clear();

        for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
          if (strcmp((const char*)atts[i],"id")==0) {
            idValue=atts[i+1];
          }
        }

        if (!StringToNumber((const char*)idValue,id)) {
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

        TagId id=typeConfig.GetTagId((const char*)keyValue);

        if (id!=tagIgnore) {
          tags[id]=(const char*)valueValue;
        }
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

        if (!StringToNumber((const char*)idValue,node)) {
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

	if (typeValue==NULL) {
          std::cerr << "Member of relation " << id << " does not have a type" << std::endl;
          return;
        }

	if (refValue==NULL) {
          std::cerr << "Member of relation " << id << " does not have a valid reference" << std::endl;
          return;
        }

	if (roleValue==NULL) {
          std::cerr << "Member of relation " << id << " does not have a valid role" << std::endl;
          return;
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

        if (!StringToNumber((const char*)refValue,member.id)) {
          std::cerr << "Cannot parse ref '" << refValue << "' for relation " << id << std::endl;
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

  std::string PreprocessOSM::GetDescription() const
  {
    return "Preprocess";
  }

  bool PreprocessOSM::Import(const ImportParameter& parameter,
                             Progress& progress,
                             const TypeConfig& typeConfig)
  {
    Preprocessor  pp(parameter,
                     typeConfig);
    Parser        parser(pp,typeConfig);

    xmlSAXHandler saxParser;

    memset(&saxParser,0,sizeof(xmlSAXHandler));
    saxParser.initialized=XML_SAX2_MAGIC;
    saxParser.getEntity=GetEntity;
    saxParser.startElement=StartElement;
    saxParser.endElement=EndElement;
    saxParser.serror=StructuredErrorHandler;

    xmlSAXUserParseFile(&saxParser,&parser,parameter.GetMapfile().c_str());

    pp.Cleanup();

    progress.Info(std::string("Nodes:          ")+NumberToString(pp.nodeCount));
    progress.Info(std::string("Ways/Areas/Sum: ")+NumberToString(pp.wayCount)+" "+
                  NumberToString(pp.areaCount)+" "+
                  NumberToString(pp.wayCount+pp.areaCount));
    progress.Info(std::string("Relations:      ")+NumberToString(pp.relationCount));

    return true;
  }
}

