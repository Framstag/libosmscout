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

#include <osmscout/gpx/Export.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/String.h>

#include <libxml/xmlwriter.h>

#include <cstring>
#include <iomanip>
#include <iostream>

using namespace osmscout;
using namespace osmscout::gpx;

/**
 * Inspired by http://www.xmlsoft.org/examples/testWriter.c
 */
class GpxWritter {
private:
  static constexpr char const *Encoding = "utf-8";

  xmlTextWriterPtr    writer;
  ProcessCallbackRef  callback;
  const GpxFile       &gpxFile;
  BreakerRef          breaker;

private:
  bool WriteGpxHeader();
  bool StartElement(const char *name);
  bool EndElement();

  bool WriteAttribute(const char *name, const char *content);
  bool WriteAttribute(const char *name, double value, std::streamsize precision=6);

  bool WriteTextElement(const char *elementName, const std::string &text);
  bool WriteTextElement(const char *elementName, double value, std::streamsize precision=6);

  /**
   * Writes timestamp in ISO 8601 format, GTM timezone
   * @param elementName
   * @param timestamp
   * @return
   */
  bool WriteTextElement(const char *elementName, const Timestamp &timestamp);

  bool WriteMetadata(const GpxFile file);

  bool WriteWaypoint(const Waypoint &waypoint);
  bool WriteWaypoints(const std::vector<Waypoint> &waypoints);

  bool WriteTrackPoint(const char *elemName, const TrackPoint &point);
  bool WriteTrackPoints(const char *elemName, const std::vector<TrackPoint> &points);

  bool WriteTrackSegment(const TrackSegment &segment);
  bool WriteTrackSegments(const std::vector<TrackSegment> &segments);

  bool WriteTrack(const Track &track);
  bool WriteTracks(const std::vector<Track> &tracks);

  bool WriteRoute(const Route &route);
  bool WriteRoutes(const std::vector<Route> &routes);
public:
  GpxWritter(const GpxFile &gpxFile,
             const std::string &filePath,
             BreakerRef breaker,
             ProcessCallbackRef callback):
      writer(nullptr),
      callback(callback),
      gpxFile(gpxFile),
      breaker(breaker)
  {
    /* Create a new XmlWriter for uri, with no compression. */
    writer = xmlNewTextWriterFilename(filePath.c_str(), 0);
    if (writer == nullptr && callback) {
      callback->Error("Error creating the xml writer");
    }
    if (writer != nullptr) {
      if (xmlTextWriterSetIndent(writer, 1) < 0){
        callback->Error("Error at xmlTextWriterSetIndent");
      }
    }
  }

  ~GpxWritter()
  {
    if (writer!=nullptr){
      xmlFreeTextWriter(writer);
      writer=nullptr;
    }
  }

  bool Process()
  {
    if (writer==nullptr) {
      return false;
    }

    if (xmlTextWriterStartDocument(writer, nullptr, Encoding, nullptr) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterStartDocument");
      }
      return false;
    }

    if (!WriteGpxHeader()){
      return false;
    }

    if (!WriteMetadata(gpxFile)){
      return false;
    }

    if (!WriteWaypoints(gpxFile.waypoints)){
      return false;
    }

    if (!WriteTracks(gpxFile.tracks)){
      return false;
    }

    if (!WriteRoutes(gpxFile.routes)){
      return false;
    }

    if (xmlTextWriterEndDocument(writer) < 0) {
      if (callback) {
        callback->Error("Error at xmlTextWriterEndDocument");
      }
      return false;
    }

    return true;
  }
};

bool GpxWritter::StartElement(const char *name)
{
  if (writer==nullptr){
    return false;
  }
  if (xmlTextWriterStartElement(writer, (const xmlChar *)name) < 0) {
    if (callback) {
      callback->Error("Error at xmlTextWriterStartElement");
    }
    return false;
  }
  return true;
}

bool GpxWritter::EndElement()
{
  if (writer==nullptr){
    return false;
  }
  if (xmlTextWriterEndElement(writer) < 0) {
    if (callback) {
      callback->Error("Error at xmlTextWriterStartElement");
    }
    return false;
  }
  return true;
}

bool GpxWritter::WriteTextElement(const char *elementName, const std::string &text)
{
  if (writer==nullptr){
    return false;
  }
  if (xmlTextWriterWriteElement(writer, (const xmlChar *)elementName, (const xmlChar *)text.c_str()) < 0) {
    if (callback) {
      callback->Error("Error at xmlTextWriterWriteElement");
    }
    return false;
  }
  return true;
}

bool GpxWritter::WriteTextElement(const char *elementName, double value, std::streamsize precision)
{
  std::ostringstream stream;
  stream.setf(std::ios::fixed,std::ios::floatfield);
  stream.imbue(std::locale("C"));
  stream.precision(precision);
  stream << value;

  return WriteTextElement(elementName, stream.str());
}

bool GpxWritter::WriteTextElement(const char *elementName, const Timestamp &timestamp)
{
  return WriteTextElement(elementName, TimestampToISO8601TimeString(timestamp));
}

bool GpxWritter::WriteAttribute(const char *name, const char *content)
{
  if (writer==nullptr){
    return false;
  }
  if (xmlTextWriterWriteAttribute(writer, (const xmlChar *)name, (const xmlChar *)content) < 0) {
    if (callback) {
      callback->Error("Error at xmlTextWriterWriteAttribute");
    }
    return false;
  }
  return true;
}

bool GpxWritter::WriteAttribute(const char *name, double value, std::streamsize precision)
{
  std::ostringstream stream;
  stream.setf(std::ios::fixed,std::ios::floatfield);
  stream.imbue(std::locale("C"));
  stream.precision(precision);
  stream << value;

  return WriteAttribute(name, stream.str().c_str());
}

bool GpxWritter::WriteGpxHeader()
{
  return StartElement("gpx") &&
      WriteAttribute("version", "1.1") &&
      WriteAttribute("creator", "libosmscout") &&
      WriteAttribute("xmlns", "http://www.topografix.com/GPX/1/1") &&
      WriteAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance") &&
      WriteAttribute("xsi:schemaLocation", "http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd");
}

bool GpxWritter::WriteWaypoint(const Waypoint &waypoint)
{
  if (!(StartElement("wpt") &&
      WriteAttribute("lat", waypoint.coord.GetLat()) &&
      WriteAttribute("lon", waypoint.coord.GetLon()))){
    return false;
  }
  // <xsd:sequence> is used in gpx xsd, elements must appear in exact order:
  //  ele, time, magvar, geoidheight, name, cmt, desc, src, link, sym, type, fix, sat, hdop, vdop, pdop, ageofdgpsdata, dgpsid, extensions
  // see http://www.topografix.com/GPX/1/1/gpx.xsd
  if (waypoint.elevation){
    if (!WriteTextElement("ele", *waypoint.elevation, 2)){
      return false;
    }
  }
  if (waypoint.time){
    if (!WriteTextElement("time", *waypoint.time)){
      return false;
    }
  }
  if (waypoint.course){
    if (!WriteTextElement("magvar", *waypoint.course, 2)){
      return false;
    }
  }
  if (waypoint.name){
    if (!WriteTextElement("name", *waypoint.name)){
      return false;
    }
  }
  if (waypoint.description){
    if (!WriteTextElement("desc", *waypoint.description)){
      return false;
    }
  }
  if (waypoint.symbol){
    if (!WriteTextElement("sym", *waypoint.symbol)){
      return false;
    }
  }
  if (waypoint.hdop){
    if (!WriteTextElement("hdop", *waypoint.hdop, 2)){
      return false;
    }
  }
  if (waypoint.vdop){
    if (!WriteTextElement("vdop", *waypoint.vdop, 2)){
      return false;
    }
  }
  if (waypoint.pdop){
    if (!WriteTextElement("pdop", *waypoint.pdop, 2)){
      return false;
    }
  }
  return EndElement();
}

bool GpxWritter::WriteTrackPoint(const char *elemName, const TrackPoint &point)
{
  if (!(StartElement(elemName) &&
        WriteAttribute("lat", point.coord.GetLat()) &&
        WriteAttribute("lon", point.coord.GetLon()))){
    return false;
  }
  // <xsd:sequence> is used in gpx xsd, elements must appear in exact order:
  //  ele, time, magvar, geoidheight, name, cmt, desc, src, link, sym, type, fix, sat, hdop, vdop, pdop, ageofdgpsdata, dgpsid, extensions
  // see http://www.topografix.com/GPX/1/1/gpx.xsd
  if (point.elevation){
    if (!WriteTextElement("ele", *point.elevation, 2)){
      return false;
    }
  }
  if (point.time){
    if (!WriteTextElement("time", *point.time)){
      return false;
    }
  }
  if (point.course){
    if (!WriteTextElement("magvar", *point.course, 2)){
      return false;
    }
  }
  if (point.hdop){
    if (!WriteTextElement("hdop", *point.hdop, 2)){
      return false;
    }
  }
  if (point.vdop){
    if (!WriteTextElement("vdop", *point.vdop, 2)){
      return false;
    }
  }
  if (point.pdop){
    if (!WriteTextElement("pdop", *point.pdop, 2)){
      return false;
    }
  }

  return EndElement();
}

bool GpxWritter::WriteTrackPoints(const char *elemName, const std::vector<TrackPoint> &points)
{
  for (const auto& point: points){
    if (breaker && breaker->IsAborted()){
      if (callback) {
        callback->Error("aborted");
      }
      return false;
    }

    if (!WriteTrackPoint(elemName, point)){
      return false;
    }
  }
  return true;
}

bool GpxWritter::WriteTrackSegment(const TrackSegment &segment)
{
  return StartElement("trkseg") &&
      WriteTrackPoints("trkpt", segment.points) &&
      EndElement();
}

bool GpxWritter::WriteTrackSegments(const std::vector<TrackSegment> &segments)
{
  for (const auto& segment: segments){
    if (!WriteTrackSegment(segment)){
      return false;
    }
  }
  return true;
}

bool GpxWritter::WriteTrack(const Track &track)
{
  if (!StartElement("trk")){
    return false;
  }
  if (track.name){
    if (!WriteTextElement("name", *track.name)){
      return false;
    }
  }
  if (track.desc){
    if (!WriteTextElement("desc", *track.desc)){
      return false;
    }
  }

  return WriteTrackSegments(track.segments) &&
      EndElement();
}

bool GpxWritter::WriteTracks(const std::vector<Track> &tracks)
{
  for (const auto& track: tracks){
    if (!WriteTrack(track)){
      return false;
    }
  }
  return true;
}

bool GpxWritter::WriteRoute(const Route &route){
  if (!StartElement("rte")){
    return false;
  }
  if (route.name){
    if (!WriteTextElement("name", *route.name)){
      return false;
    }
  }

  return WriteTrackPoints("rtept", route.points) &&
      EndElement();
}

bool GpxWritter::WriteRoutes(const std::vector<Route> &routes)
{
  for (const auto& route: routes){
    if (!WriteRoute(route)){
      return false;
    }
  }
  return true;
}

bool GpxWritter::WriteMetadata(const GpxFile file)
{
  if (!file.name && !file.desc && !file.time){
    return true;
  }

  if (!StartElement("metadata")) {
    return false;
  }

  if (file.name){
    if (!WriteTextElement("name", *file.name)){
      return false;
    }
  }
  if (file.desc){
    if (!WriteTextElement("desc", *file.desc)){
      return false;
    }
  }
  if (file.time){
    if (!WriteTextElement("time", *file.time)){
      return false;
    }
  }

  return EndElement();
}

bool GpxWritter::WriteWaypoints(const std::vector<Waypoint> &waypoints)
{
  for (const auto& waypoint: waypoints){
    if (breaker && breaker->IsAborted()){
      if (callback) {
        callback->Error("aborted");
      }
      return false;
    }
    if (!WriteWaypoint(waypoint)){
      return false;
    }
  }
  return true;
}

bool gpx::ExportGpx(const GpxFile &gpxFile,
                    const std::string &filePath,
                    BreakerRef breaker,
                    ProcessCallbackRef callback)
{
  GpxWritter writter(gpxFile, filePath, breaker, callback);
  return writter.Process();
}
