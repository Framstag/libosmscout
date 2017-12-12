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
    const TypeConfig&                     typeConfig;
    Progress&                             progress;
    PreprocessorCallback&                 callback;
    Context                               context;
    OSMId                                 id;
    double                                lon,lat;
    TagMap                                tags;
    std::vector<OSMId>                    nodes;
    std::vector<RawRelation::Member>      members;
    size_t                                blockDataSize;
    PreprocessorCallback::RawBlockDataRef blockData;

  public:
    Parser(const TypeConfig& typeConfig,
           Progress& progress,
           PreprocessorCallback& callback)
    : typeConfig(typeConfig),
      progress(progress),
      callback(callback),
      context(contextUnknown)
    {
      // no code
    }

    void StartElement(const xmlChar *name, const xmlChar **atts)
    {
      if (!blockData) {
        blockData=std::unique_ptr<PreprocessorCallback::RawBlockData>(new PreprocessorCallback::RawBlockData());
        blockDataSize=0;
        blockData->nodeData.reserve(10000);
        blockData->wayData.reserve(10000);
        blockData->relationData.reserve(10000);
      }

      if (strcmp((const char*)name,"node")==0) {
        const xmlChar *idValue=nullptr;
        const xmlChar *latValue=nullptr;
        const xmlChar *lonValue=nullptr;

        context=contextNode;
        tags.clear();

        for (size_t i=0; atts[i]!=nullptr && atts[i+1]!=nullptr; i+=2) {
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

        if (idValue==nullptr || lonValue==nullptr || latValue==nullptr) {
          progress.Error("Not all required attributes found");
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
        const xmlChar *idValue=nullptr;

        context=contextWay;
        nodes.clear();
        members.clear();
        tags.clear();

        for (size_t i=0; atts[i]!=nullptr && atts[i+1]!=nullptr; i+=2) {
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
        const xmlChar *idValue=nullptr;

        context=contextRelation;
        tags.clear();
        nodes.clear();
        members.clear();

        for (size_t i=0; atts[i]!=nullptr && atts[i+1]!=nullptr; i+=2) {
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

        const xmlChar *keyValue=nullptr;
        const xmlChar *valueValue=nullptr;

        for (size_t i=0; atts[i]!=nullptr && atts[i+1]!=nullptr; i+=2) {
          if (strcmp((const char*)atts[i],"k")==0) {
            keyValue=atts[i+1];
          }
          else if (strcmp((const char*)atts[i],"v")==0) {
            valueValue=atts[i+1];
          }
        }

        if (keyValue==nullptr || valueValue==nullptr) {
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
        const xmlChar *idValue=nullptr;

        for (size_t i=0; atts[i]!=nullptr && atts[i+1]!=nullptr; i+=2) {
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
        const xmlChar       *typeValue=nullptr;
        const xmlChar       *refValue=nullptr;
        const xmlChar       *roleValue=nullptr;

        for (size_t i=0; atts[i]!=nullptr && atts[i+1]!=nullptr; i+=2) {
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

        if (typeValue==nullptr) {
          std::cerr << "Member of relation " << id << " does not have a type" << std::endl;
          return;
        }

        if (refValue==nullptr) {
          std::cerr << "Member of relation " << id << " does not have a valid reference" << std::endl;
          return;
        }

        if (roleValue==nullptr) {
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

        if (roleValue!=nullptr) {
          member.role=(const char*)roleValue;
        }

        members.push_back(member);
      }
    }

    void EndElement(const xmlChar *name)
    {
      try {
        if (strcmp((const char*)name,"node")==0) {
          PreprocessorCallback::RawNodeData data;

          data.id=id;
          data.coord.Set(lat,lon);
          data.tags=std::move(tags);

          blockData->nodeData.push_back(std::move(data));
          blockDataSize++;

          context=contextUnknown;
        }
        else if (strcmp((const char*)name,"way")==0) {
          PreprocessorCallback::RawWayData data;

          data.id=id;
          data.nodes=std::move(nodes);
          data.tags=std::move(tags);

          blockData->wayData.push_back(std::move(data));
          blockDataSize++;

          context=contextUnknown;
        }
        else if (strcmp((const char*)name,"relation")==0) {
          PreprocessorCallback::RawRelationData data;

          data.id=id;
          data.members=std::move(members);
          data.tags=std::move(tags);

          blockData->relationData.push_back(std::move(data));
          blockDataSize++;

          context=contextUnknown;
        }

        if (blockDataSize>10000) {
          callback.ProcessBlock(std::move(blockData));
          blockData=nullptr;
        }
      }
      catch (IOException& e) {
        progress.Error(e.GetDescription());
      }
    }

    void EndDocument()
    {
      if (blockData) {
        callback.ProcessBlock(std::move(blockData));
      }
    }
  };

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

  static xmlEntityPtr GetEntity(void* /*data*/, const xmlChar *name)
  {
    return xmlGetPredefinedEntity(name);
  }

  static void StructuredErrorHandler(void* /*data*/, xmlErrorPtr error)
  {
    std::cerr << "XML error, line " << error->line << ": " << error->message << std::endl;
  }

  static void WarningHandler(void* /*data*/, const char* msg,...)
  {
    std::cerr << "XML warning:" << msg << std::endl;
  }

  static void ErrorHandler(void* /*data*/, const char* msg,...)
  {
    std::cerr << "XML error:" << msg << std::endl;
  }

  static void StartDocumentHandler(void* /*data*/)
  {
    // no code, only for temporary debugging purposes
  }

  static void EndDocumentHandler(void* data)
  {
    Parser* parser=static_cast<Parser*>(data);

    parser->EndDocument();
  }

  PreprocessOSM::PreprocessOSM(PreprocessorCallback& callback)
  : callback(callback)
  {
    // no code
  }

  bool PreprocessOSM::Import(const TypeConfigRef& typeConfig,
                             const ImportParameter& /*parameter*/,
                             Progress& progress,
                             const std::string& filename)
  {
    progress.SetAction(std::string("Parsing *.osm file '")+filename+"'");

    Parser        parser(*typeConfig,
                         progress,
                         callback);
    FILE             *file;
    xmlSAXHandler    saxParser;
    xmlParserCtxtPtr ctxt;

    memset(&saxParser,0,sizeof(xmlSAXHandler));
    saxParser.initialized=XML_SAX2_MAGIC;

    saxParser.startDocument=StartDocumentHandler;
    saxParser.endDocument=EndDocumentHandler;
    saxParser.getEntity=GetEntity;
    saxParser.startElement=StartElement;
    saxParser.endElement=EndElement;
    saxParser.warning=WarningHandler;
    saxParser.error=ErrorHandler;
    saxParser.fatalError=ErrorHandler;
    saxParser.serror=StructuredErrorHandler;

    file=fopen(filename.c_str(),"rb");

    if (file==nullptr) {
      return false;
    }

    char chars[1024];

    int res=fread(chars,1,4,file);
    if (res!=4) {
      fclose(file);
      return false;
    }

    ctxt=xmlCreatePushParserCtxt(&saxParser,&parser,chars,res,nullptr);

    // Resolve entities, do not do any network communication
    xmlCtxtUseOptions(ctxt,XML_PARSE_NOENT|XML_PARSE_NONET);

    while ((res=fread(chars,1,sizeof(chars),file))>0) {
      if (xmlParseChunk(ctxt,chars,res,0)!=0) {
        xmlParserError(ctxt,"xmlParseChunk");
        xmlFreeParserCtxt(ctxt);
        fclose(file);

        return false;
      }
    }

    if (xmlParseChunk(ctxt,chars,0,1)!=0) {
      xmlParserError(ctxt,"xmlParseChunk");
      xmlFreeParserCtxt(ctxt);
      fclose(file);

      return false;
    }

    xmlFreeParserCtxt(ctxt);
    fclose(file);

    return true;
  }
}
