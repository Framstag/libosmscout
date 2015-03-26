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

#include <osmscout/import/PreprocessOSM.h>

#include <algorithm>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <string.h>

#include <libxml/parser.h>

#include <osmscout/util/File.h>
#include <osmscout/util/String.h>

#include <osmscout/import/RawNode.h>
#include <osmscout/import/RawRelation.h>
#include <osmscout/import/RawWay.h>

namespace osmscout {

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
    PreprocessOSM&                   pp;
    const TypeConfig&                typeConfig;
    OSMId                            id;
    double                           lon,lat;
    TagMap                           tags;
    std::vector<OSMId>               nodes;
    std::vector<RawRelation::Member> members;

  public:
    Parser(PreprocessOSM& pp,
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

        OSMId         node;
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
        pp.ProcessNode(typeConfig,
                       id,
                       lon,
                       lat,
                       tags);
        tags.clear();
        context=contextUnknown;
      }
      else if (strcmp((const char*)name,"way")==0) {
        pp.ProcessWay(typeConfig,
                      id,
                      nodes,
                      tags);
        nodes.clear();
        tags.clear();
        context=contextUnknown;
      }
      else if (strcmp((const char*)name,"relation")==0) {
        pp.ProcessRelation(typeConfig,
                           id,
                           members,
                           tags);
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

  static void StructuredErrorHandler(void */*data*/, xmlErrorPtr error)
  {
    std::cerr << "XML error, line " << error->line << ": " << error->message << std::endl;
  }

  std::string PreprocessOSM::GetDescription() const
  {
    return "Preprocess";
  }

  bool PreprocessOSM::Import(const TypeConfigRef& typeConfig,
                             const ImportParameter& parameter,
                             Progress& progress)
  {
    Parser        parser(*this,typeConfig);
    xmlSAXHandler saxParser;

    if (!Initialize(typeConfig,
                    parameter,
                    progress)) {
      return false;
    }

    memset(&saxParser,0,sizeof(xmlSAXHandler));
    saxParser.initialized=XML_SAX2_MAGIC;
    saxParser.getEntity=GetEntity;
    saxParser.startElement=StartElement;
    saxParser.endElement=EndElement;
    saxParser.serror=StructuredErrorHandler;

    xmlSAXUserParseFile(&saxParser,&parser,parameter.GetMapfile().c_str());

    return Cleanup(typeConfig,
                   parameter,
                   progress);
  }
}

