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

#include <cstring>
#include <functional>
#include <iomanip>
#include <iostream>

#include <libxml/parser.h>

#include <osmscout/gpx/Import.h>

#include <osmscout/util/Logger.h>

#include <osmscout/system/Assert.h>

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
  virtual ~GpxParserContext() = default;

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

  ~DocumentContext() override = default;

  const char *ContextName() const override
  {
    return "Document";
  }

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &/*atts*/) override;

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
         const BreakerRef& breaker,
         const ProcessCallbackRef& callback):
  file(nullptr),
  ctxt(nullptr),
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
    // in case of parsing failure, there may still exists some context
    // we may have to delete from the the back (lifo)
    // to avoid use after free, context may use its parent in destructor
    while (!contextStack.empty()) {
      PopContext();
    }

    if (ctxt!=nullptr) {
      xmlFreeParserCtxt(ctxt);
    }
    if (file!=nullptr){
      std::fclose(file);
    }
  }

  bool Process()
  {
    if (file==nullptr) {
      return false;
    }

    char chars[1024];

    size_t res=std::fread(chars,1,4,file);
    if (res!=4) {
      return false;
    }

    ctxt=xmlCreatePushParserCtxt(&saxParser,this,chars,
                                 static_cast<int>(res),
                                 nullptr);

    // Resolve entities, do not do any network communication
    xmlCtxtUseOptions(ctxt,XML_PARSE_NOENT|XML_PARSE_NONET);

    FileOffset position=0;
    while ((res=std::fread(chars,1,sizeof(chars),file))>0) {
      if (xmlParseChunk(ctxt,chars,
                        static_cast<int>(res),0)!=0) {
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
    contextStack.push_back(top==nullptr ? nullptr : top->StartElement(name, atts));
  }

  void Characters(const std::string &str)
  {
    assert(!contextStack.empty());
    GpxParserContext *top=contextStack.back();
    if (top!=nullptr) {
      top->Characters(str);
    }
  }

  void PopContext(){
    assert(!contextStack.empty());
    GpxParserContext *top=contextStack.back();

    delete top;

    contextStack.pop_back();
  }

  void EndElement(const std::string &/*name*/) {
    PopContext();
  }

  void EndDocument()
  {
    PopContext();
  }

  void Error(const std::string& msg)
  {
    errorCnt++;
    if (callback){
      callback->Error(msg);
    }
  }

  void Warning(const std::string& msg)
  {
    osmscout::log.Warn() << msg;
  }

  static bool ParseDoubleAttr(const std::unordered_map<std::string,std::string>& atts,
                              const std::string& attName,
                              double& target)
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
    auto* parser=static_cast<GpxParser*>(data);
    std::unordered_map<std::string,std::string> attsMap;
    if (atts!=nullptr) {
      for (size_t i = 0; atts[i] !=nullptr && atts[i + 1] !=nullptr; i += 2) {
        attsMap[std::string((const char *) atts[i])] = std::string((const char *) atts[i + 1]);
      }
    }
    parser->StartElement(std::string((const char *)name),attsMap);
  }

  static void Characters(void *data, const xmlChar *ch, int len)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->Characters(std::string((const char *)ch,
                                   static_cast<unsigned long>(len)));
  }

  static void EndElement(void *data, const xmlChar *name)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->EndElement(std::string((const char *)name));
  }

  static xmlEntityPtr GetEntity(void* /*data*/, const xmlChar *name)
  {
    return xmlGetPredefinedEntity(name);
  }

  static void StructuredErrorHandler(void* data, xmlErrorPtr error)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->Error("XML error, line " + std::to_string(error->line) + ": " + error->message);
  }

  static void WarningHandler(void* data, const char* msg,...)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->Warning(msg);
  }

  static void ErrorHandler(void* data, const char* msg,...)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->Error(std::string("XML error:") + msg );
  }

  static void StartDocumentHandler(void* data)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->StartDocument();
  }


  static void EndDocumentHandler(void* data)
  {
    auto* parser=static_cast<GpxParser*>(data);
    parser->EndDocument();
  }
};

GpxParserContext* GpxParserContext::StartElement(const std::string &name,
                                                 const std::unordered_map<std::string, std::string> &/*atts*/)
{
  xmlParserWarning(ctxt,"Unexpected element %s start on context %s\n", name.c_str(), ContextName());
  parser.Warning("Unexpected element");
  return nullptr;
}

class SimpleValueContext : public GpxParserContext {

  using Callback = std::function<void (const std::string &)>;

private:
  std::string name;
  std::string buffer;
  Callback callback;
public:
  SimpleValueContext(const std::string& name,
                     xmlParserCtxtPtr ctxt,
                     GpxParser &parser,
                     Callback callback):
      GpxParserContext(ctxt, parser), name(name), callback(callback) { }

  ~SimpleValueContext() override
  {
    callback(buffer);
  }

  const char *ContextName() const override
  {
    return name.c_str();
  };

  void Characters(const std::string &str) override
  {
    buffer+=str;
  };

};

class PointLikeContext : public GpxParserContext {
protected:
  TrackPoint point;
public:
  PointLikeContext(xmlParserCtxtPtr ctxt, GpxParser &parser, double lat, double lon) :
      GpxParserContext(ctxt, parser), point(GeoCoord(lat,lon)) { }

  ~PointLikeContext() override = default;

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &/*atts*/) override
  {
    if (name == "ele") {
      return new SimpleValueContext("EleContext", ctxt, parser, [&](const std::string &value){
        double ele;
        if (StringToNumber(value, ele)){
          point.elevation=std::make_optional<double>(ele);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    }

    if (name == "magvar") {
      return new SimpleValueContext("MagvarContext", ctxt, parser, [&](const std::string &value){
        double course;
        if (StringToNumber(value, course)){
          point.course=std::make_optional<double>(course);
        }else{
          xmlParserWarning(ctxt,"Can't parse Magvar value\n");
          parser.Warning("Can't parse Magvar value");
        }
      });
    }

    if (name == "hdop") {
      return new SimpleValueContext("HDopContext", ctxt, parser, [&](const std::string &value){
        double hdop;
        if (StringToNumber(value, hdop)){
          point.hdop=std::make_optional<double>(hdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse HDop value\n");
          parser.Warning("Can't parse HDop value");
        }
      });
    }

    if (name == "vdop") {
      return new SimpleValueContext("VDopContext", ctxt, parser, [&](const std::string &value){
        double vdop;
        if (StringToNumber(value, vdop)){
          point.vdop=std::make_optional<double>(vdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    }

    if (name == "pdop") {
      return new SimpleValueContext("PDopContext", ctxt, parser, [&](const std::string &value){
        double pdop;
        if (StringToNumber(value, pdop)){
          point.pdop=std::make_optional<double>(pdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse PDop value\n");
          parser.Warning("Can't parse PDop value");
        }
      });
    }

    if (name == "time") {
      return new SimpleValueContext("TimeContext", ctxt, parser, [&](const std::string &value){
        Timestamp time;
        if (ParseISO8601TimeString(value, time)){
          point.time=std::make_optional<Timestamp>(time);
        }else{
          xmlParserWarning(ctxt,"Can't parse PDop value\n");
          parser.Warning("Can't parse PDop value");
        }
      });
    }

    return nullptr; // silently ignore unknown elements
  }
};

class TrkptContext : public PointLikeContext {
private:
  TrackSegment &segment;
public:
  TrkptContext(xmlParserCtxtPtr ctxt, TrackSegment &segment, GpxParser &parser, double lat, double lon) :
      PointLikeContext(ctxt, parser, lat, lon), segment(segment) {}

  ~TrkptContext() override
  {
    segment.points.push_back(std::move(point));
  }

  const char *ContextName() const override
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

  ~RteptContext() override
  {
    route.points.push_back(std::move(point));
  }

  const char *ContextName() const override
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

  ~WaypointContext() override
  {
    output.waypoints.push_back(std::move(waypoint));
  }

  const char *ContextName() const override
  {
    return "Waypoint";
  }

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &/*atts*/) override
  {
    if (name=="name") {
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name) {
        waypoint.name = std::make_optional<std::string>(name);
      });
    }

    if (name == "desc") {
      return new SimpleValueContext("DescContext", ctxt, parser, [&](const std::string &description) {
        waypoint.description = std::make_optional<std::string>(description);
      });
    }

    if (name == "sym") {
      return new SimpleValueContext("SymContext", ctxt, parser, [&](const std::string &symbol) {
        waypoint.symbol = std::make_optional<std::string>(symbol);
      });
    }

    if (name == "ele") {
      return new SimpleValueContext("EleContext", ctxt, parser, [&](const std::string &value){
        double ele;
        if (StringToNumber(value, ele)){
          waypoint.elevation=std::make_optional<double>(ele);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    }

    if (name == "hdop") {
      return new SimpleValueContext("HDopContext", ctxt, parser, [&](const std::string &value){
        double hdop;
        if (StringToNumber(value, hdop)){
          waypoint.hdop=std::make_optional<double>(hdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse HDop value\n");
          parser.Warning("Can't parse HDop value");
        }
      });
    }

    if (name == "vdop") {
      return new SimpleValueContext("VDopContext", ctxt, parser, [&](const std::string &value){
        double vdop;
        if (StringToNumber(value, vdop)){
          waypoint.vdop=std::make_optional<double>(vdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse Ele value\n");
          parser.Warning("Can't parse Ele value");
        }
      });
    }

    if (name == "pdop") {
      return new SimpleValueContext("PDopContext", ctxt, parser, [&](const std::string &value){
        double pdop;
        if (StringToNumber(value, pdop)){
          waypoint.pdop=std::make_optional<double>(pdop);
        }else{
          xmlParserWarning(ctxt,"Can't parse PDop value\n");
          parser.Warning("Can't parse PDop value");
        }
      });
    }

    if (name == "time") {
      return new SimpleValueContext("TimeContext", ctxt, parser, [&](const std::string &value){
        Timestamp time;
        if (ParseISO8601TimeString(value, time)){
          waypoint.time=std::make_optional<Timestamp>(time);
        }else{
          xmlParserWarning(ctxt,"Can't parse Time value\n");
          parser.Warning("Can't parse Time value");
        }
      });
    }

    return nullptr; // silently ignore unknown elements
  }
};

class MetadataContext : public GpxParserContext {
private:
  GpxFile &output;
public:
  MetadataContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser) :
  GpxParserContext(ctxt, parser), output(output) { }

  ~MetadataContext() override
  {
  }

  const char *ContextName() const override
  {
    return "Metadata";
  }

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &/*atts*/) override
  {
    if (name=="name") {
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name) {
        output.name = std::make_optional<std::string>(name);
      });
    }

    if (name == "desc") {
      return new SimpleValueContext("DescContext", ctxt, parser, [&](const std::string &description) {
        output.desc = std::make_optional<std::string>(description);
      });
    }

    if (name == "time") {
      return new SimpleValueContext("TimeContext", ctxt, parser, [&](const std::string &value){
        Timestamp time;
        if (ParseISO8601TimeString(value, time)){
          output.time=std::make_optional<Timestamp>(time);
        }else{
          xmlParserWarning(ctxt,"Can't parse Time value\n");
          parser.Warning("Can't parse Time value");
        }
      });
    }

    return nullptr; // silently ignore unknown elements
  }
};

class TrkSegContext : public GpxParserContext {
private:
  Track &track;
  TrackSegment segment;
public:
  TrkSegContext(xmlParserCtxtPtr ctxt, Track &track, GpxParser &parser) :
      GpxParserContext(ctxt, parser), track(track) {}

  ~TrkSegContext() override
  {
    track.segments.push_back(std::move(segment));
  }

  const char *ContextName() const override
  {
    return "TrkSeg";
  }

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &atts) override
  {
    if (name=="trkpt"){
      double lat;
      double lon;
      if (GpxParser::ParseDoubleAttr(atts, "lat", lat) &&
          GpxParser::ParseDoubleAttr(atts, "lon", lon)
          ) {
        return new TrkptContext(ctxt, segment, parser, lat, lon);
      }

      xmlParserError(ctxt,"Can't parse trkpt lan/lon\n");
      parser.Error("Can't parse trkpt lan/lon");
      return nullptr;
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

  ~RouteContext() override
  {
    output.routes.push_back(std::move(route));
  }

  const char *ContextName() const override
  {
    return "Route";
  }

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &atts) override
  {
    if (name=="rtept") {
      double lat;
      double lon;
      if (GpxParser::ParseDoubleAttr(atts, "lat", lat) &&
          GpxParser::ParseDoubleAttr(atts, "lon", lon)
          ) {
        return new RteptContext(ctxt, route, parser, lat, lon);
      }

      xmlParserError(ctxt, "Can't parse trkpt lan/lon\n");
      parser.Error("Can't parse trkpt lan/lon");
      return nullptr;
    }

    if (name=="name"){
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name){
        route.name=std::make_optional<std::string>(name);
      });
    }

    return nullptr; // silently ignore unknown elements
  }
};

class TrkContext : public GpxParserContext {
private:
  Track track;
  GpxFile &output;
public:
  TrkContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      GpxParserContext(ctxt, parser), output(output) { }

  ~TrkContext() override
  {
    output.tracks.push_back(std::move(track));
  }

  const char *ContextName() const override
  {
    return "Trk";
  };

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &/*atts*/) override
  {
    if (name=="name"){
      return new SimpleValueContext("NameContext", ctxt, parser, [&](const std::string &name){
        track.name=std::make_optional<std::string>(name);
      });
    }

    if (name=="desc"){
      return new SimpleValueContext("DescContext", ctxt, parser, [&](const std::string &desc){
        track.desc=std::make_optional<std::string>(desc);
      });
    }

    if (name=="trkseg"){
      return new TrkSegContext(ctxt, track, parser);
    }

    return nullptr; // silently ignore unknown elements
  }
};

class GpxDocumentContext : public GpxParserContext {
  GpxFile &output;
public:
  GpxDocumentContext(xmlParserCtxtPtr ctxt, GpxFile &output, GpxParser &parser):
      GpxParserContext(ctxt, parser), output(output) {}

  ~GpxDocumentContext() override = default;

  const char *ContextName() const override
  {
    return "Gpx";
  }

  GpxParserContext* StartElement(const std::string &name,
                                 const std::unordered_map<std::string, std::string> &atts) override
  {
    if (name=="trk"){
      return new TrkContext(ctxt,output,parser);
    }

    if (name=="wpt"){
      double lat;
      double lon;
      if (GpxParser::ParseDoubleAttr(atts, "lat", lat) &&
          GpxParser::ParseDoubleAttr(atts, "lon", lon)
          ) {
        return new WaypointContext(ctxt, output, parser, lat, lon);
      }

      xmlParserError(ctxt,"Can't parse wpt lan/lon\n");
      parser.Error("Can't parse wpt lan/lon");
      return nullptr;
    }

    if (name=="rte"){
      return new RouteContext(ctxt,output,parser);
    }

    if (name=="metadata"){
      return new MetadataContext(ctxt,output,parser);
    }
    return nullptr; // silently ignore unknown elements
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
