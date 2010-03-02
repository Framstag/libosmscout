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

#include <osmscout/TypeConfigLoader.h>

#include <string.h>

#include <libxml/parser.h>

#include <iostream>

namespace osmscout {

  class TypeConfigParser
  {
    enum Context {
      contextUnknown,
      contextTop,
      contextTags,
      contextTag,
      contextTypes,
      contextType
    };

  private:
    Context     context;
    TypeConfig& config;

  public:
    TypeConfigParser(TypeConfig& config)
    : config(config)
    {
      context=contextUnknown;
    }

    void StartElement(const xmlChar *name, const xmlChar **atts)
    {
      if (context==contextUnknown) {
        if (strcmp((const char*)name,"TravelJinni-Type")==0) {
          context=contextTop;
        }
        else {
          std::cerr << "Expected tag 'TravelJinni-Type'" << std::endl;
          return;
        }
      }
      else if (context==contextTop) {
        if (strcmp((const char*)name,"tags")==0) {
          context=contextTags;
        }
        else if (strcmp((const char*)name,"types")==0) {
          context=contextTypes;
        }
        else {
          std::cerr << "Expected tag 'tags' or tag 'types'" << std::endl;
          return;
        }
      }
      else if (context==contextTags) {
        if (strcmp((const char*)name,"tag")==0) {
          context=contextTag;
        }
        else {
          std::cerr << "Expected tag 'tag'" << std::endl;
          return;
        }
      }
      else if (context==contextTypes) {
        if (strcmp((const char*)name,"type")==0) {
          context=contextType;
        }
        else {
          std::cerr << "Expected tag 'type'" << std::endl;
          return;
        }
      }
      else {
        std::cerr << "Unknown tag '" << name << "'" << std::endl;
        return;
      }

      if (context==contextTag) {
        const xmlChar *valueValue=NULL;
        const xmlChar *idValue=NULL;
        std::string   value;
        TagId         id;

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"value")==0) {
              valueValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"id")==0) {
              idValue=atts[i+1];
            }
          }
        }

        if (valueValue==NULL || idValue==NULL) {
          std::cerr << "Not all required attributes for tag found" << std::endl;
          return;
        }

        value=(const char*)valueValue;
        if (sscanf((const char*)idValue,"%u",&id)!=1) {
          std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
          return;
        }

        if (config.GetTagId((const char*)valueValue)!=tagIgnore) {
          //std::cerr << "Tag '" << value << "' already internally defined, skipping..." << std::endl;
          return;
        }

        if (id>=tagPrivateBase) {
          std::cerr << "Tag '" << value << "' has id in illegal range (must be <" << tagPrivateBase << ")!" << std::endl;
          return;
        }

        TagInfo tagInfo(value,id);

        config.AddTagInfo(tagInfo);
      }
      else if (context==contextType) {
        const xmlChar *idValue=NULL;
        const xmlChar *nameValue=NULL;
        const xmlChar *valueValue=NULL;
        const xmlChar *nodeValue=NULL;
        const xmlChar *wayValue=NULL;
        const xmlChar *areaValue=NULL;
        const xmlChar *relationValue=NULL;
        const xmlChar *routeValue=NULL;
        const xmlChar *indexValue=NULL;
        TypeId        id;
        bool          node=false;
        bool          way=false;
        bool          area=false;
        bool          relation=false;
        bool          route=false;
        bool          index=false;

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"id")==0) {
              idValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"name")==0) {
              nameValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"value")==0) {
              valueValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"node")==0) {
              nodeValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"way")==0) {
              wayValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"area")==0) {
              areaValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"relation")==0) {
              relationValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"route")==0) {
              routeValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"index")==0) {
              indexValue=atts[i+1];
            }
          }
        }

        if (idValue==NULL || nameValue==NULL || valueValue==NULL) {
          std::cerr << "Not all required attributes for type found" << std::endl;
          return;
        }

        if (sscanf((const char*)idValue,"%u",&id)!=1) {
          std::cerr << "Cannot parse id: '" << idValue << "'" << std::endl;
          return;
        }

        if (nodeValue!=NULL) {
          node=strcmp((const char*)nodeValue,"true")==0;
        }
        if (wayValue!=NULL) {
          way=strcmp((const char*)wayValue,"true")==0;
        }
        if (areaValue!=NULL) {
          area=strcmp((const char*)areaValue,"true")==0;
        }
        if (relationValue!=NULL) {
          relation=strcmp((const char*)relationValue,"true")==0;
        }
        if (routeValue!=NULL) {
          route=strcmp((const char*)routeValue,"true")==0;
        }
        if (indexValue!=NULL) {
          index=strcmp((const char*)indexValue,"true")==0;
        }

        TagId tag=config.GetTagId((const char*)nameValue);

        if (tag==tagIgnore) {
          std::cerr << "Type with tag '" << nameValue <<"/" << valueValue << "' has no corresponding tag definition, skipping!" << std::endl;
          return;
        }

        if (id>=typePrivateBase) {
          std::cerr << "Type '" << nameValue << "/" << valueValue << "' has id in illegal range (must be <" << typePrivateBase << ")!" << std::endl;
          return;
        }

        TypeInfo typeInfo(id,tag,(const char*)valueValue);

        typeInfo.CanBeNode(node);
        typeInfo.CanBeWay(way);
        typeInfo.CanBeArea(area);
        typeInfo.CanBeRelation(relation);
        typeInfo.CanBeRoute(route);
        typeInfo.CanBeIndexed(index);

        config.AddTypeInfo(typeInfo);
      }
    }

    void EndElement(const xmlChar *name)
    {
      if (context==contextTop) {
        if (strcmp((const char*)name,"TravelJinni-Type")==0) {
          context=contextUnknown;
        }
      }
      else if (context==contextTags) {
        if (strcmp((const char*)name,"tags")==0) {
          context=contextTop;
        }
      }
      else if (context==contextTypes) {
        if (strcmp((const char*)name,"types")==0) {
          context=contextTop;
        }
      }
      else if (context==contextTag) {
        if (strcmp((const char*)name,"tag")==0) {
          context=contextTags;
        }
      }
      else if (context==contextType) {
        if (strcmp((const char*)name,"type")==0) {
          context=contextTypes;
        }
      }
      else {
        std::cerr << "Unknown tag '" << (const char*)name << "'" << std::endl;
      }
    }
  };

  static xmlEntityPtr GetEntity(void* /*data*/, const xmlChar *name)
  {
    return xmlGetPredefinedEntity(name);
  }

  static void StartElement(void *data, const xmlChar *name, const xmlChar **atts)
  {
    TypeConfigParser* parser=static_cast<TypeConfigParser*>(data);

    parser->StartElement(name,atts);
  }

  static void EndElement(void *data, const xmlChar *name)
  {
    TypeConfigParser* parser=static_cast<TypeConfigParser*>(data);

    parser->EndElement(name);
  }

  static void StructuredErrorHandler(void *data, xmlErrorPtr error)
  {
    std::cerr << "XML error, line " << error->line << ": " << error->message << std::endl;
  }

  bool LoadTypeConfig(const char* typeFile,
                      TypeConfig& config)
  {
    TypeConfigParser parser(config);
    xmlSAXHandler    saxParser;

    memset(&saxParser,0,sizeof(xmlSAXHandler));
    saxParser.initialized=XML_SAX2_MAGIC;
    saxParser.getEntity=GetEntity;
    saxParser.startElement=StartElement;
    saxParser.endElement=EndElement;
    saxParser.serror=StructuredErrorHandler;

    xmlSAXUserParseFile(&saxParser,&parser,typeFile);

    return true;
  }
}
