/*
  This source is part of the libosmscout-gpx library
  Copyright (C) 2017 Lukas Karas

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

#include <osmscout/gpx/Import.h>
#include <osmscout/util/Logger.h>

#include <cstring>
#include <list>
#include <cassert>

using namespace osmscout;

void ImportCallback::progress(double)
{
  // no-op
}

void ImportCallback::error(std::string error)
{
  osmscout::log.Error() << error;
}

class GpxParser;

class GpxParserContext {
protected:
  xmlParserCtxtPtr  ctxt;
  GpxFile           &output;
  GpxParser         &parser;

public:
  GpxParserContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      ctxt(ctxt),output(output), parser(parser) { }
  virtual ~GpxParserContext() { }

  virtual const char *ContextName() const = 0;

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &/*atts*/);

  virtual void Characters(std::string str){};
};


class DocumentContext : public GpxParserContext {
public:
  DocumentContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser) :
      GpxParserContext(ctxt, output, parser) {}

  virtual ~DocumentContext() {}

  virtual const char *ContextName() const {
    return "Document";
  }

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &/*atts*/);

};

class GpxParser{

private:
  std::list<GpxParserContext*> contextStack;

  std::FILE         *file;
  xmlSAXHandler     saxParser;
  xmlParserCtxtPtr  ctxt;
  FileOffset        fileSize;

  GpxFile           &output;
  BreakerRef        breaker;
  ImportCallbackRef callback;

  size_t            errorCnt;

public:
  GpxParser(const std::string &filePath,
         GpxFile &output,
         BreakerRef breaker,
         ImportCallbackRef callback):
  file(NULL),
  ctxt(NULL),
  fileSize(0),
  output(output),
  breaker(breaker),
  callback(callback),
  errorCnt(0)
  {
    memset(&saxParser,0,sizeof(xmlSAXHandler));
    saxParser.initialized=XML_SAX2_MAGIC;

    saxParser.startDocument=StartDocumentHandler;
    saxParser.endDocument=EndDocumentHandler;
    saxParser.getEntity=GetEntity;
    saxParser.startElement=StartElement;
    saxParser.characters=Characters;
    saxParser.endElement=EndElement;
    saxParser.warning=WarningHandler;
    saxParser.error=ErrorHandler;
    saxParser.fatalError=ErrorHandler;
    saxParser.serror=StructuredErrorHandler;

    try{
      fileSize=GetFileSize(filePath);
    }catch(const IOException &e){
      callback->error("Can't get file size: "+e.GetErrorMsg());
    }

    file=std::fopen(filePath.c_str(),"rb");
  }

  ~GpxParser()
  {
    for (auto context : contextStack) {
      if (context != NULL) {
        delete context;
      }
    }
    contextStack.clear();
    if (ctxt!=NULL) {
      xmlFreeParserCtxt(ctxt);
    }
    if (file!=NULL){
      std::fclose(file);
    }
  }


  bool Process()
  {
    if (file==NULL) {
      return false;
    }

    char chars[1024];

    int res=std::fread(chars,1,4,file);
    if (res!=4) {
      return false;
    }

    ctxt=xmlCreatePushParserCtxt(&saxParser,this,chars,res,NULL);

    // Resolve entities, do not do any network communication
    xmlCtxtUseOptions(ctxt,XML_PARSE_NOENT|XML_PARSE_NONET);

    FileOffset position=0;
    while ((res=std::fread(chars,1,sizeof(chars),file))>0) {
      if (xmlParseChunk(ctxt,chars,res,0)!=0) {
        xmlParserError(ctxt,"xmlParseChunk");
        return false;
      }
      if (errorCnt>0){
        return false;
      }
      if (breaker && breaker->IsAborted()){
        if (callback) {
          callback->error("aborted");
        }
        return false;
      }
      position+=res;
      if (callback) {
        callback->progress(std::min(1.0, ((double) position) / ((double) fileSize)));
      }
    }

    if (xmlParseChunk(ctxt,chars,0,1)!=0) {
      xmlParserError(ctxt,"xmlParseChunk");
      return false;
    }

    return errorCnt==0;
  }

  void StartDocument()
  {
    assert(contextStack.empty());
    contextStack.push_back(new DocumentContext(ctxt, output, *this));
  }

  void StartElement(const std::string &name,
                    const std::unordered_map<std::string, std::string> &atts)
  {
    assert(!contextStack.empty());
    GpxParserContext *top=contextStack.back();
    contextStack.push_back(top==NULL? NULL : top->StartElement(name, atts));
  }

  void Characters(std::string str)
  {
    assert(!contextStack.empty());
    GpxParserContext *top=contextStack.back();
    if (top!=NULL) {
      top->Characters(str);
    }
  }

  void PopContext(){
    assert(!contextStack.empty());
    GpxParserContext *top=contextStack.back();
    if (top!=NULL){
      delete top;
    }
    contextStack.pop_back();
  }

  void EndElement(const std::string &name) {
    PopContext();
  }

  void EndDocument()
  {
    PopContext();
  }

  void Error(std::string msg)
  {
    errorCnt++;
    if (callback){
      callback->error(msg);
    }
  }

  void Warning(std::string msg)
  {
    osmscout::log.Warn() << msg;
  }

  static void StartElement(void *data, const xmlChar *name, const xmlChar **atts)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    std::unordered_map<std::string,std::string> attsMap;
    if (atts!=NULL) {
      for (size_t i = 0; atts[i] != NULL && atts[i + 1] != NULL; i += 2) {
        attsMap[std::string((const char *) atts[i])] = std::string((const char *) atts[i + 1]);
      }
    }
    parser->StartElement(std::string((const char *)name),attsMap);
  }

  static void Characters(void *data, const xmlChar *ch, int len)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->Characters(std::string((const char *)ch,len));
  }

  static void EndElement(void *data, const xmlChar *name)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->EndElement(std::string((const char *)name));
  }

  static xmlEntityPtr GetEntity(void* /*data*/, const xmlChar *name)
  {
    return xmlGetPredefinedEntity(name);
  }

  static void StructuredErrorHandler(void* data, xmlErrorPtr error)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->Error("XML error, line " + std::to_string(error->line) + ": " + error->message);
  }

  static void WarningHandler(void* data, const char* msg,...)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->Warning(msg);
  }

  static void ErrorHandler(void* data, const char* msg,...)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->Error(std::string("XML error:") + msg );
  }

  static void StartDocumentHandler(void* data)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->StartDocument();
  }


  static void EndDocumentHandler(void* data)
  {
    GpxParser* parser=static_cast<GpxParser*>(data);
    parser->EndDocument();
  }

};

GpxParserContext* GpxParserContext::StartElement(const std::string &name,
                                                 const std::unordered_map<std::string, std::string> &/*atts*/)
{
  xmlParserError(ctxt,"Unexpected element %s start on context %s\n", name.c_str(), ContextName());
  parser.Error("Unexpected element");
  return NULL;
}

class TrkContext : public GpxParserContext {
public:
  TrkContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      GpxParserContext(ctxt, output, parser) { }
  virtual ~TrkContext() { }

  virtual const char *ContextName() const
  {
    return "Trk";
  };

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &atts)
  {
    // if (name=="trk"){
    //   return new TrkContext(ctxt,output,parser);
    // }
    return GpxParserContext::StartElement(name, atts);
  }
};

class GpxDocumentContext : public GpxParserContext {
public:
  GpxDocumentContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      GpxParserContext(ctxt, output, parser) {}
  virtual ~GpxDocumentContext() {}

  virtual const char *ContextName() const {
    return "Gpx";
  }

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &atts)
  {
    if (name=="trk"){
      return new TrkContext(ctxt,output,parser);
    }
    return GpxParserContext::StartElement(name, atts);
  }
};

GpxParserContext* DocumentContext::StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &atts)
{
  if (name=="gpx"){
    return new GpxDocumentContext(ctxt,output,parser);
  }
  return GpxParserContext::StartElement(name, atts);
}

bool Import::ImportGpx(const std::string &filePath,
                       GpxFile &output,
                       BreakerRef breaker,
                       ImportCallbackRef callback)
{

  GpxParser parser(filePath, output, breaker, callback);
  return parser.Process();
}
