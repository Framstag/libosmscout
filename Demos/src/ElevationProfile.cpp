/*
  GpxPipe - a demo program for libosmscout
  Copyright (C) 2017 Lukas Karas

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <osmscout/gpx/Import.h>

#include <osmscout/util/CmdLineParsing.h>
#include <osmscout/util/Logger.h>
#include <osmscout/util/Distance.h>
#include <osmscout/ElevationService.h>

#include <iostream>
#include <osmscout/Database.h>
#include <osmscout/gpx/Export.h>

struct Arguments
{
  bool        help{false};
  std::string database;
  std::string gpxInput;
  std::string gpxOutput;
};

class DataLoader
{
private:
  osmscout::DatabaseRef database;
  osmscout::TypeInfoSet contourTypes;
  osmscout::EleFeatureValueReader reader;
  size_t count{0};
  double millis{0};

public:
  explicit DataLoader(osmscout::DatabaseRef &database):
    database(database),
    reader(*(database->GetTypeConfig()))
  {
    assert(database);

    for (const auto &type:database->GetTypeConfig()->GetWayTypes()){
      if (type->HasFeature(osmscout::EleFeature::NAME)){
        osmscout::log.Debug() << "Using type " << type->GetName();
        contourTypes.Set(type);
      }
    }
  }

  std::vector<osmscout::ContoursData> LoadContours(const osmscout::GeoBox &box)
  {
    osmscout::StopClock stopClock;
    std::vector<osmscout::FileOffset> offsets;
    osmscout::TypeInfoSet loadedTypes;
    if (!database->GetAreaWayIndex()->GetOffsets(box,contourTypes,offsets,loadedTypes)) {
      assert(false);
    }
    std::vector<osmscout::WayRef> contours;
    if (!database->GetWaysByOffset(offsets, contours)) {
      assert(false);
    }
    stopClock.Stop();
    count++;
    millis+=stopClock.GetMilliseconds();
    return std::vector<osmscout::ContoursData>{osmscout::ContoursData{reader,contours}};
  }

  size_t GetCount() const
  {
    return count;
  }

  double GetMillis() const
  {
    return millis;
  }
};

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("ElevationProfile",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string &s) {
                        args.gpxOutput=s;
                      }),
                      "debugGpxOutput",
                      "file for debug gpx output");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.database=value;
                          }),
                          "DATABASE",
                          "OSMScout database directory");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.gpxInput=value;
                          }),
                          "GPXFILEINPUT",
                          "Gpx file to analyse");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  } else if (!osmscout::ExistsInFilesystem(args.gpxInput)) {
    std::cerr << "ERROR: Input file " << args.gpxInput << " don't exists" << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  } else if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  osmscout::log.Debug(true);

  osmscout::gpx::GpxFile gpxFile;

  if (!ImportGpx(args.gpxInput, gpxFile)){
    return 1;
  }
  if (gpxFile.tracks.empty()){
    std::cerr << "No track in gpx file" << std::endl;
    return 1;
  }
  if (gpxFile.tracks[0].segments.empty()){
    std::cerr << "No segments in first track" << std::endl;
    return 1;
  }

  osmscout::StopClock filterStopClock;
  gpxFile.tracks[0].FilterPoints([](std::vector<osmscout::gpx::TrackPoint> &points){
    osmscout::gpx::FilterInaccuratePoints(points, 30);
    osmscout::gpx::FilterNearPoints(points, osmscout::Distance::Of<osmscout::Meter>(10));
  });
  filterStopClock.Stop();
  osmscout::log.Debug() << "Filter track: " << filterStopClock.ResultString();


  // extract geo points from first segment
  std::vector<osmscout::GeoCoord> way;
  for (const auto &point:gpxFile.tracks[0].segments[0].points){
    way.push_back(point.coord);
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(args.database)) {
    std::cerr << "Cannot open database" << std::endl;
    return 1;
  }

  osmscout::StopClock stopClock;

  DataLoader dataLoader(database);
  osmscout::ElevationService<DataLoader> eleService(dataLoader, osmscout::Magnification::magSuburb);

  osmscout::gpx::GpxFile output;

  std::cout << "Distance \tElevation \tCoord (type)" << std::endl;
  size_t pointCnt=eleService.ElevationProfile(way, [&](const osmscout::Distance&, const std::vector<osmscout::ElevationPoint> &points){
    for (const auto &point: points) {
      std::cout << point.distance << " \t" << point.elevation.AsMeter() << " m \t" << point.coord.GetDisplayText() << " (" << point.contour->GetType()->GetName() << " " << point.contour->GetFileOffset() << ")" << std::endl;

      if (!args.gpxOutput.empty()) {
        osmscout::gpx::Waypoint w(point.coord);
        w.name = std::make_optional<std::string>(point.distance.AsString() + " " + std::to_string(point.elevation.AsMeter()) + " m");
        output.waypoints.push_back(w);
      }
    }
  });

  stopClock.Stop();
  osmscout::log.Debug() << "Evaluating elevation profile: " << stopClock.ResultString() << " (" << dataLoader.GetCount() << "x loading: " << dataLoader.GetMillis()/1000 << " s)";

  if (pointCnt==0){
    std::cout << "No intersection with contours found." << std::endl;
    return 0;
  }
  std::cout << "Found " << pointCnt << " intersections" << std::endl;

  // export debug gpx output
  if (!args.gpxOutput.empty()) {
    output.tracks.push_back(gpxFile.tracks[0]);
    osmscout::gpx::ExportGpx(output, args.gpxOutput);
  }

  return 0;
}
