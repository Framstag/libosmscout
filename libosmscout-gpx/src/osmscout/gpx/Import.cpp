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
#include <osmscout/util/String.h>

#include <libxml/parser.h>

#include <cstring>
#include <cassert>
#include <functional>
#include <iomanip>
#include <iostream>

using namespace osmscout;
using namespace osmscout::gpx;

class GpxParser;

class GpxParserContext {
protected:
  xmlParserCtxtPtr  ctxt;
  GpxParser         &parser;

public:
  GpxParserContext(xmlParserCtxtPtr ctxt, GpxParser &parser):
      ctxt(ctxt), parser(parser) { }
  virtual ~GpxParserContext() { }

  virtual const char *ContextName() const = 0;

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &/*atts*/);

  virtual void Characters(const std::string &/*str*/){};
};


class DocumentContext : public GpxParserContext {
private:
  GpxFile &output;
public:
  DocumentContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser) :
      GpxParserContext(ctxt, parser), output(output) {}

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
  ProcessCallbackRef callback;

  size_t            errorCnt;

public:
  GpxParser(const std::string &filePath,
         GpxFile &output,
         BreakerRef breaker,
         ProcessCallbackRef callback):
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
      callback->Error("Can't get file size: " + e.GetErrorMsg());
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
          callback->Error("aborted");
        }
        return false;
      }
      position+=res;
      if (callback) {
        callback->Progress(std::min(1.0, ((double) position) / ((double) fileSize)));
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

  void Characters(const std::string &str)
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

  void EndElement(const std::string &/*name*/) {
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
      callback->Error(msg);
    }
  }

  void Warning(std::string msg)
  {
    osmscout::log.Warn() << msg;
  }

  bool ParseDoubleAttr(const std::unordered_map<std::string, std::string> &atts,
                       const std::string &attName,
                       double &target) const
  {
    auto it=atts.find(attName);
    if (it==atts.end()){
      return false;
    }
    const std::string value=it->second;
    return StringToNumber(value,target);
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
  xmlParserWarning(ctxt,"Unexpected element %s start on context %s\n", name.c_str(), ContextName());
  parser.Warning("Unexpected element");
  return NULL;
}

class SimpleValueContext : public GpxParserContext {

  typedef std::function<void(const std::string &)> Callback;

private:
  std::string name;
  std::string buffer;
  Callback callback;
public:
  SimpleValueContext(std::string name,
                     xmlParserCtxtPtr ctxt,
                     GpxParser &parser,
                     Callback callback):
      GpxParserContext(ctxt, parser), name(name), callback(callback) { }
  virtual ~SimpleValueContext() {
    callback(buffer);
  }

  virtual const char *ContextName() const
  {
    return name.c_str();
  };

  virtual void Characters(const std::string &str){
    buffer+=str;
  };

};

class PointLikeContext : public GpxParserContext {
protected:
  TrackPoint point;
public:
  PointLikeContext(xmlParserCtxtPtr ctxt, GpxParser &parser, double lat, double lon) :
      GpxParserContext(ctxt, parser), point(GeoCoord(lat,lon)) { }

  virtual ~PointLikeContext() {}

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &/*atts*/) {
    if (name == "ele") {
      return new SimpleValueContext("EleContext", ctxt, parser, [&](const std::string &value){
        double ele;
        if (StringToNumber(value, ele)){
          point.elevation=Optional<double>::of(ele);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    } else if (name == "course") {
      return new SimpleValueContext("CourseContext", ctxt, parser, [&](const std::string &value){
        double course;
        if (StringToNumber(value, course)){
          point.course=Optional<double>::of(course);
        }else{
          xmlParserWarning(ctxt,"Can't parse Course value\n");
          parser.Warning("Can't parse Course value");
        }
      });
    } else if (name == "hdop") {
      return new SimpleValueContext("HDopContext", ctxt, parser, [&](const std::string &value){
        double hdop;
        if (StringToNumber(value, hdop)){
          point.hdop=Optional<double>::of(hdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse HDop value\n");
          parser.Warning("Can't parse HDop value");
        }
      });
    } else if (name == "vdop") {
      return new SimpleValueContext("VDopContext", ctxt, parser, [&](const std::string &value){
        double vdop;
        if (StringToNumber(value, vdop)){
          point.vdop=Optional<double>::of(vdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    } else if (name == "pdop") {
      return new SimpleValueContext("PDopContext", ctxt, parser, [&](const std::string &value){
        double pdop;
        if (StringToNumber(value, pdop)){
          point.pdop=Optional<double>::of(pdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse PDop value\n");
          parser.Warning("Can't parse PDop value");
        }
      });
    } else if (name == "time") {
      return new SimpleValueContext("TimeContext", ctxt, parser, [&](const std::string &value){
        Timestamp time;
        if (ParseISO8601TimeString(value, time)){
          point.time=Optional<Timestamp>::of(time);
        }else{
          xmlParserWarning(ctxt,"Can't parse PDop value\n");
          parser.Warning("Can't parse PDop value");
        }
      });
    }

    return NULL; // silently ignore unknown elements
  }
};

class TrkptContext : public PointLikeContext {
private:
  TrackSegment &segment;
public:
  TrkptContext(xmlParserCtxtPtr ctxt, TrackSegment &segment, GpxParser &parser, double lat, double lon) :
      PointLikeContext(ctxt, parser, lat, lon), segment(segment) {}

  virtual ~TrkptContext() {
    segment.points.push_back(std::move(point));
  }

  virtual const char *ContextName() const
  {
    return "Trkpt";
  }
};

class RteptContext : public PointLikeContext {
private:
  Route &route;
public:
  RteptContext(xmlParserCtxtPtr ctxt, Route &route, GpxParser &parser, double lat, double lon) :
    PointLikeContext(ctxt, parser, lat, lon), route(route) {}

  virtual ~RteptContext() {
    route.points.push_back(std::move(point));
  }

  virtual const char *ContextName() const
  {
    return "Rtept";
  }
};

class WaypointContext : public GpxParserContext {
private:
  GpxFile &output;
  Waypoint waypoint;
public:
  WaypointContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser, double lat, double lon) :
      GpxParserContext(ctxt, parser), output(output), waypoint(GeoCoord(lat,lon)) { }

  virtual ~WaypointContext() {
    output.waypoints.push_back(std::move(waypoint));
  }

  virtual const char *ContextName() const
  {
    return "Waypoint";
  }

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &/*atts*/) {
    if (name=="name") {
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name) {
        waypoint.name = Optional<std::string>::of(name);
      });
    } else if (name == "ele") {
      return new SimpleValueContext("EleContext", ctxt, parser, [&](const std::string &value){
        double ele;
        if (StringToNumber(value, ele)){
          waypoint.elevation=Optional<double>::of(ele);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    } else if (name == "hdop") {
      return new SimpleValueContext("HDopContext", ctxt, parser, [&](const std::string &value){
        double hdop;
        if (StringToNumber(value, hdop)){
          waypoint.hdop=Optional<double>::of(hdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse HDop value\n");
          parser.Warning("Can't parse HDop value");
        }
      });
    } else if (name == "vdop") {
      return new SimpleValueContext("VDopContext", ctxt, parser, [&](const std::string &value){
        double vdop;
        if (StringToNumber(value, vdop)){
          waypoint.vdop=Optional<double>::of(vdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    } else if (name == "pdop") {
      return new SimpleValueContext("PDopContext", ctxt, parser, [&](const std::string &value){
        double pdop;
        if (StringToNumber(value, pdop)){
          waypoint.pdop=Optional<double>::of(pdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse PDop value\n");
          parser.Warning("Can't parse PDop value");
        }
      });
    } else if (name == "time") {
      return new SimpleValueContext("TimeContext", ctxt, parser, [&](const std::string &value){
        Timestamp time;
        if (ParseISO8601TimeString(value, time)){
          waypoint.time=Optional<Timestamp>::of(time);
        }else{
          xmlParserWarning(ctxt,"Can't parse Time value\n");
          parser.Warning("Can't parse Time value");
        }
      });
    }

    return NULL; // silently ignore unknown elements
  }
};

class TrkSegContext : public GpxParserContext {
private:
  Track &track;
  TrackSegment segment;
public:
  TrkSegContext(xmlParserCtxtPtr ctxt, Track &track, GpxParser &parser) :
      GpxParserContext(ctxt, parser), track(track) {}

  virtual ~TrkSegContext() {
    track.segments.push_back(std::move(segment));
  }

  virtual const char *ContextName() const {
    return "TrkSeg";
  }

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &atts)
  {
    if (name=="trkpt"){
      double lat;
      double lon;
      if (parser.ParseDoubleAttr(atts, "lat", lat) &&
          parser.ParseDoubleAttr(atts, "lon", lon)
          ) {
        return new TrkptContext(ctxt, segment, parser, lat, lon);
      }else{
        xmlParserError(ctxt,"Can't parse trkpt lan/lon\n");
        parser.Error("Can't parse trkpt lan/lon");
        return NULL;
      }
    }

    return GpxParserContext::StartElement(name, atts);
  }
};

class RouteContext : public GpxParserContext {
private:
  GpxFile &output;
  Route route;
public:
  RouteContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser) :
  GpxParserContext(ctxt, parser), output(output) {}

  virtual ~RouteContext() {
    output.routes.push_back(std::move(route));
  }

  virtual const char *ContextName() const {
    return "Route";
  }

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &atts)
  {
    if (name=="rtept") {
      double lat;
      double lon;
      if (parser.ParseDoubleAttr(atts, "lat", lat) &&
          parser.ParseDoubleAttr(atts, "lon", lon)
          ) {
        return new RteptContext(ctxt, route, parser, lat, lon);
      } else {
        xmlParserError(ctxt, "Can't parse trkpt lan/lon\n");
        parser.Error("Can't parse trkpt lan/lon");
        return NULL;
      }
    } else if (name=="name"){
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name){
        route.name=Optional<std::string>::of(name);
      });
    }

    return NULL; // silently ignore unknown elements
  }
};

class TrkContext : public GpxParserContext {
private:
  Track track;
  GpxFile &output;
public:
  TrkContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      GpxParserContext(ctxt, parser), output(output) { }
  virtual ~TrkContext() {
    output.tracks.push_back(std::move(track));
  }

  virtual const char *ContextName() const
  {
    return "Trk";
  };

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &/*atts*/)
  {
    if (name=="name"){
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name){
        track.name=Optional<std::string>::of(name);
      });
    } else if (name=="desc"){
      return new SimpleValueContext("DescContext", ctxt, parser, [&](const std::string &desc){
        track.desc=Optional<std::string>::of(desc);
      });
    }else if (name=="trkseg"){
      return new TrkSegContext(ctxt, track, parser);
    }

    return NULL; // silently ignore unknown elements
  }
};

class GpxDocumentContext : public GpxParserContext {
  GpxFile &output;
public:
  GpxDocumentContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      GpxParserContext(ctxt, parser), output(output) {}
  virtual ~GpxDocumentContext() {}

  virtual const char *ContextName() const {
    return "Gpx";
  }

  virtual GpxParserContext* StartElement(const std::string &name,
                                         const std::unordered_map<std::string, std::string> &atts)
  {
    if (name=="trk"){
      return new TrkContext(ctxt,output,parser);
    } else if (name=="wpt"){
      double lat;
      double lon;
      if (parser.ParseDoubleAttr(atts, "lat", lat) &&
          parser.ParseDoubleAttr(atts, "lon", lon)
          ) {
        return new WaypointContext(ctxt, output, parser, lat, lon);
      }else{
        xmlParserError(ctxt,"Can't parse wpt lan/lon\n");
        parser.Error("Can't parse wpt lan/lon");
        return NULL;
      }
    } else if (name=="rte"){
      return new RouteContext(ctxt,output,parser);
    }
    return NULL; // silently ignore unknown elements
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

bool gpx::ImportGpx(const std::string &filePath,
                    GpxFile &output,
                    BreakerRef breaker,
                    ProcessCallbackRef callback)
{

  GpxParser parser(filePath, output, breaker, callback);
  return parser.Process();
}
