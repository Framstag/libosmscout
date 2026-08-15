/*
  LocationLookup - a demo program for libosmscout
  Copyright (C) 2010  Tim Teulings

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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <osmscout/GeoCoord.h>

#include <osmscout/db/Database.h>

#include <osmscout/location/LocationService.h>

#include <osmscout/feature/NameFeature.h>

#include <osmscout/cli/CmdLineParsing.h>

#ifdef OSMSCOUT_HAVE_LIB_MARISA
#include <osmscout/db/TextSearchIndex.h>
#endif

struct Arguments
{
  bool                   help=false;
  bool                   debug=false;
  std::string            databaseDirectory;
  std::string            defaultAdminRegion;
  bool                   searchForLocation=true;
  bool                   searchForPOI=true;
  bool                   adminRegionOnlyMatch=false;
  bool                   poiOnlyMatch=false;
  bool                   locationOnlyMatch=false;
  bool                   addressOnlyMatch=false;
  bool                   partialMatch=false;
  size_t                 limit=30;
  size_t                 repeat=1;
  std::list<std::string> location;
  bool                   transliterate=false;
  bool                   structuredOnly=false;
  bool                   fulltextOnly=false;
  bool                   hasCenter=false;
  std::string            autoCenterRegion;
  double                 lat=0.0;
  double                 lon=0.0;
  double                 typeWeight=1.0;
  double                 distanceWeight=1.0;
  double                 matchWeight=1.0;
  std::string            weightString;
};

// Rank weights, matching the OSMScout2/JavaScout ranking formula:
// rank = typeRank * distanceRank * matchRank.
// Use --type-weight/--distance-weight/--match-weight to override the
// defaults without recompiling.

/**
 * Rank for an object type, matching the type table of OSMScout2's
 * `locationRank` and JavaScout's `LocationSearchRanker`.
 */
double GetTypeRank(const std::string& objectType)
{
  if (objectType=="boundary_country") {
    return 1.0;
  }
  if (objectType=="boundary_state") {
    return 0.93;
  }
  if (objectType=="boundary_administrative" ||
      objectType=="place_town") {
    return 0.9;
  }
  if (objectType=="highway_residential" ||
      objectType=="address") {
    return 0.8;
  }
  if (objectType=="railway_station" ||
      objectType=="railway_tram_stop" ||
      objectType=="railway_subway_entrance" ||
      objectType=="highway_bus_stop") {
    return 0.7;
  }

  return 0.5;
}

/**
 * Match rank: 1.0 for an exact label match with the pattern, 0.75 for a
 * prefix match, 0.5 otherwise. Case-insensitive, matching the Java/JS
 * implementations (which lowercase with Locale.ROOT).
 */
double GetMatchRank(const std::string& label,
                    const std::string& pattern)
{
  if (pattern.empty()) {
    return 1.0;
  }

  std::string lowerLabel=label;
  std::string lowerPattern=pattern;

  std::transform(lowerLabel.begin(),
                 lowerLabel.end(),
                 lowerLabel.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });
  std::transform(lowerPattern.begin(),
                 lowerPattern.end(),
                 lowerPattern.begin(),
                 [](unsigned char c) {
                   return static_cast<char>(std::tolower(c));
                 });

  if (lowerLabel==lowerPattern) {
    return 1.0;
  }
  if (lowerLabel.size()>=lowerPattern.size() &&
      lowerLabel.compare(0,lowerPattern.size(),lowerPattern)==0) {
    return 0.75;
  }

  return 0.5;
}

/**
 * Distance rank for a distance in meters: 1/log((distance/1000)+e),
 * matching the Java/JS implementations.
 */
double GetDistanceRank(double distanceMeters)
{
  return 1.0/std::log((distanceMeters/1000.0)+std::exp(1.0));
}

bool GetAdminRegionHierachie(const osmscout::LocationServiceRef& locationService,
                             const osmscout::AdminRegionRef& adminRegion,
                             std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap,
                             std::string& path)
{
  if (!locationService->ResolveAdminRegionHierachie(adminRegion,
                                                    adminRegionMap)) {
    return false;
  }

  if (!path.empty()) {
    path.append("/");
  }

  path.append(osmscout::UTF8StringToLocaleString(adminRegion->name));

  osmscout::FileOffset parentRegionOffset=adminRegion->parentRegionOffset;

  while (parentRegionOffset!=0) {
    auto entry=adminRegionMap.find(parentRegionOffset);

    if (entry==adminRegionMap.end()) {
      break;
    }

    osmscout::AdminRegionRef parentRegion=entry->second;

    if (!path.empty()) {
      path.append("/");
    }

    path.append(osmscout::UTF8StringToLocaleString(parentRegion->name));

    parentRegionOffset=parentRegion->parentRegionOffset;
  }

  return true;
}

std::string GetAdminRegionHierachie(const osmscout::LocationServiceRef& locationService,
                                    const osmscout::LocationSearchResult::Entry& entry,
                                    std::map<osmscout::FileOffset,osmscout::AdminRegionRef>& adminRegionMap)
{
  std::string path;

  if (!GetAdminRegionHierachie(locationService,
                               entry.adminRegion,
                               adminRegionMap,
                               path)) {
    return "";
  }

  return path;
}

/**
 * Resolve an object reference to its reference type, coordinates and
 * object type name. Node coordinates come from the node itself, way and
 * area coordinates from the center of the bounding box, mirroring the
 * JNI bridge.
 */
bool GetObjectInfo(const osmscout::DatabaseRef& database,
                   const osmscout::ObjectFileRef& object,
                   std::string& refType,
                   std::string& objectTypeName,
                   double& lat,
                   double& lon)
{
  if (object.GetType()==osmscout::RefType::refNode) {
    osmscout::NodeRef node;

    if (!database->GetNodeByOffset(object.GetFileOffset(),
                                   node)) {
      return false;
    }

    refType="node";
    objectTypeName=node->GetType()->GetName();
    lat=node->GetCoords().GetLat();
    lon=node->GetCoords().GetLon();

    return true;
  }

  if (object.GetType()==osmscout::RefType::refArea) {
    osmscout::AreaRef area;

    if (!database->GetAreaByOffset(object.GetFileOffset(),
                                   area)) {
      return false;
    }

    refType="area";
    objectTypeName=area->GetType()->GetName();
    lat=area->GetBoundingBox().GetCenter().GetLat();
    lon=area->GetBoundingBox().GetCenter().GetLon();

    return true;
  }

  if (object.GetType()==osmscout::RefType::refWay) {
    osmscout::WayRef way;

    if (!database->GetWayByOffset(object.GetFileOffset(),
                                  way)) {
      return false;
    }

    refType="way";
    objectTypeName=way->GetType()->GetName();
    lat=way->GetBoundingBox().GetCenter().GetLat();
    lon=way->GetBoundingBox().GetCenter().GetLon();

    return true;
  }

  return false;
}

// One merged search hit, either from the structured location index ("idx")
// or from the text index ("txt")
struct SearchHit
{
  std::string source;         //!< "idx" or "txt"
  std::string label;
  std::string objectType;     //!< coarse object type used for type rank, mirroring the JNI bridge
  std::string objectTypeName; //!< real object type name
  std::string matchQuality;   //!< "match" or "candidate"
  std::string qualifiers;     //!< per-field match qualities for structured hits
  std::string hierarchy;      //!< admin region hierarchy path
  std::string refType;        //!< node, way or area
  long long   objectFileOffset=0;
  bool        hasCoords=false;
  double      lat=0.0;
  double      lon=0.0;
  double      distanceMeters=0.0;
  double      typeRank=0.0;
  double      distanceRank=1.0;
  double      matchRank=0.5;
  double      rank=0.0;
};

/**
 * Append a per-field match quality marker ("=" exact, "~" candidate, "-"
 * none) for one search part.
 */
void AppendQualifier(std::string& qualifiers,
                     const std::string& part,
                     bool hasField,
                     osmscout::LocationSearchResult::MatchQuality quality)
{
  if (!qualifiers.empty()) {
    qualifiers.append(",");
  }

  if (!hasField) {
    qualifiers.append("-"+part);
  }
  else if (quality==osmscout::LocationSearchResult::match) {
    qualifiers.append("="+part);
  }
  else {
    qualifiers.append("~"+part);
  }
}

/**
 * Build the merged display entry for one structured search result,
 * mirroring the label, object type and match quality construction of the
 * JNI bridge (OSMScoutClient::searchLocations).
 */
SearchHit BuildStructuredHit(const osmscout::DatabaseRef& database,
                             const osmscout::LocationServiceRef& locationService,
                             const osmscout::LocationSearchResult::Entry& entry,
                             const std::string& pattern,
                             double typeWeight,
                             double distanceWeight,
                             double matchWeight,
                             bool hasCenter,
                             double centerLat,
                             double centerLon)
{
  SearchHit hit;

  hit.source="idx";

  // label, mirroring the JNI bridge
  if (entry.adminRegion &&
      entry.location &&
      entry.address) {
    hit.label=entry.location->name+" "+entry.address->name;
  }
  else if (entry.location) {
    hit.label=entry.location->name;
  }
  else if (entry.poi) {
    hit.label=entry.poi->name;
  }
  else if (entry.adminRegion) {
    hit.label=entry.adminRegion->name;
  }
  else if (entry.address) {
    hit.label=entry.address->name;
  }

  // coarse object type for type ranking, mirroring the JNI bridge
  if (entry.adminRegion) {
    hit.objectType="boundary_administrative";
  }
  else if (entry.poi) {
    hit.objectType="poi";
  }
  else if (entry.location) {
    hit.objectType="place";
  }
  else if (entry.address) {
    hit.objectType="address";
  }

  // match quality: exact match on any part, mirroring the JNI bridge
  hit.matchQuality="candidate";
  if ((entry.location && entry.locationMatchQuality==osmscout::LocationSearchResult::match) ||
      (entry.address && entry.addressMatchQuality==osmscout::LocationSearchResult::match) ||
      (entry.adminRegion && entry.adminRegionMatchQuality==osmscout::LocationSearchResult::match) ||
      (entry.poi && entry.poiMatchQuality==osmscout::LocationSearchResult::match)) {
    hit.matchQuality="match";
  }

  // per-field match qualities
  AppendQualifier(hit.qualifiers,"adm",entry.adminRegion!=nullptr,entry.adminRegionMatchQuality);
  AppendQualifier(hit.qualifiers,"post",entry.postalArea!=nullptr,entry.postalAreaMatchQuality);
  AppendQualifier(hit.qualifiers,"loc",entry.location!=nullptr,entry.locationMatchQuality);
  AppendQualifier(hit.qualifiers,"addr",entry.address!=nullptr,entry.addressMatchQuality);
  AppendQualifier(hit.qualifiers,"poi",entry.poi!=nullptr,entry.poiMatchQuality);

  // admin region hierarchy
  std::map<osmscout::FileOffset,osmscout::AdminRegionRef> adminRegionMap;
  if (entry.adminRegion) {
    hit.hierarchy=GetAdminRegionHierachie(locationService,
                                          entry,
                                          adminRegionMap);
  }

  // object reference, coordinates and type name
  osmscout::ObjectFileRef object;
  if (entry.address) {
    object=entry.address->object;
  }
  else if (entry.poi) {
    object=entry.poi->object;
  }
  else if (entry.location && !entry.location->objects.empty()) {
    object=entry.location->objects.front();
  }
  else if (entry.adminRegion) {
    object=entry.adminRegion->object;
  }

  if (object.Valid()) {
    hit.objectFileOffset=static_cast<long long>(object.GetFileOffset());
    hit.hasCoords=GetObjectInfo(database,
                                object,
                                hit.refType,
                                hit.objectTypeName,
                                hit.lat,
                                hit.lon);
  }

  // ranking
  hit.typeRank=GetTypeRank(hit.objectType)*typeWeight;
  if (hit.hasCoords && hasCenter) {
    osmscout::Distance distance=osmscout::GeoCoord(centerLat,centerLon).GetDistance(osmscout::GeoCoord(hit.lat,hit.lon));
    hit.distanceMeters=distance.AsMeter();
    hit.distanceRank=GetDistanceRank(hit.distanceMeters)*distanceWeight;
  }
  hit.matchRank=GetMatchRank(hit.label,pattern)*matchWeight;
  hit.rank=hit.typeRank*hit.distanceRank*hit.matchRank;

  return hit;
}

#ifdef OSMSCOUT_HAVE_LIB_MARISA
/**
 * Build a merged display entry for one free-text hit, mirroring the JNI
 * bridge's BuildFreeTextEntry.
 */
SearchHit BuildFreeTextHit(const osmscout::DatabaseRef& database,
                           const osmscout::ObjectFileRef& object,
                           const std::string& searchKey,
                           const osmscout::NameFeatureLabelReader& nameLabelReader,
                           const std::string& pattern,
                           double typeWeight,
                           double distanceWeight,
                           double matchWeight,
                           bool hasCenter,
                           double centerLat,
                           double centerLon)
{
  SearchHit hit;

  hit.source="txt";
  hit.matchQuality="match";

  hit.objectFileOffset=static_cast<long long>(object.GetFileOffset());

  if (object.GetType()==osmscout::RefType::refNode) {
    osmscout::NodeRef node;

    if (database->GetNodeByOffset(object.GetFileOffset(),node)) {
      hit.refType="node";
      hit.objectTypeName=node->GetType()->GetName();
      hit.lat=node->GetCoords().GetLat();
      hit.lon=node->GetCoords().GetLon();
      hit.label=nameLabelReader.GetLabel(node->GetFeatureValueBuffer());
      hit.hasCoords=true;
    }
  }
  else if (object.GetType()==osmscout::RefType::refArea) {
    osmscout::AreaRef area;

    if (database->GetAreaByOffset(object.GetFileOffset(),area)) {
      hit.refType="area";
      hit.objectTypeName=area->GetType()->GetName();
      hit.lat=area->GetBoundingBox().GetCenter().GetLat();
      hit.lon=area->GetBoundingBox().GetCenter().GetLon();
      hit.label=nameLabelReader.GetLabel(area->GetFeatureValueBuffer());
      hit.hasCoords=true;
    }
  }
  else if (object.GetType()==osmscout::RefType::refWay) {
    osmscout::WayRef way;

    if (database->GetWayByOffset(object.GetFileOffset(),way)) {
      hit.refType="way";
      hit.objectTypeName=way->GetType()->GetName();
      hit.lat=way->GetBoundingBox().GetCenter().GetLat();
      hit.lon=way->GetBoundingBox().GetCenter().GetLon();
      hit.label=nameLabelReader.GetLabel(way->GetFeatureValueBuffer());
      hit.hasCoords=true;
    }
  }

  if (hit.label.empty()) {
    hit.label=searchKey;
  }

  // free-text hits use the real object type for ranking, mirroring the JNI bridge
  hit.objectType=hit.objectTypeName;

  hit.typeRank=GetTypeRank(hit.objectType)*typeWeight;
  if (hit.hasCoords && hasCenter) {
    osmscout::Distance distance=osmscout::GeoCoord(centerLat,centerLon).GetDistance(osmscout::GeoCoord(hit.lat,hit.lon));
    hit.distanceMeters=distance.AsMeter();
    hit.distanceRank=GetDistanceRank(hit.distanceMeters)*distanceWeight;
  }
  hit.matchRank=GetMatchRank(hit.label,pattern)*matchWeight;
  hit.rank=hit.typeRank*hit.distanceRank*hit.matchRank;

  return hit;
}
#endif

/**
 * Print the merged, rank-sorted result list.
 */
void PrintResults(const std::vector<SearchHit>& hits,
                  bool hasCenter)
{
  std::cout << std::endl;
  std::cout << " #  src  match   rank (T*D*M)       qualifiers  dist     label / type / hierarchy / coords" << std::endl;
  std::cout << "---  ---  ------  ----------------  ----------  -------  ---------------------------------" << std::endl;

  int index=0;
  for (const auto& hit : hits) {
    index++;

    std::cout << std::setw(2) << index << "  ";
    std::cout << hit.source << "  ";

    std::cout << std::setw(7) << hit.matchQuality << "  ";

    std::cout << std::fixed << std::setprecision(3)
              << std::setw(7) << hit.rank << " (";

    std::cout << std::setprecision(2)
              << "T" << hit.typeRank
              << " D" << hit.distanceRank
              << " M" << hit.matchRank
              << ")  ";

    std::string qual=hit.qualifiers;
    if (qual.empty()) {
      qual="-";
    }
    std::cout << std::setw(10) << qual << "  ";

    // Distance from the search center; only meaningful when a center was given
    std::string dist="-";
    if (hasCenter && hit.hasCoords) {
      if (hit.distanceMeters<1000.0) {
        dist=std::to_string(static_cast<int>(hit.distanceMeters))+"m";
      }
      else {
        std::ostringstream stream;
        stream.imbue(std::cout.getloc());
        stream << std::fixed << std::setprecision(1) << (hit.distanceMeters/1000.0) << "km";
        dist=stream.str();
      }
    }
    std::cout << std::setw(7) << dist << "  ";

    std::cout << hit.label;

    if (!hit.objectTypeName.empty()) {
      std::cout << "  [" << hit.objectTypeName << "]";
    }
    else if (!hit.objectType.empty()) {
      std::cout << "  [" << hit.objectType << "]";
    }

    if (!hit.refType.empty()) {
      std::cout << " (" << hit.refType << " " << hit.objectFileOffset << ")";
    }

    if (!hit.hierarchy.empty()) {
      std::cout << "  -> " << hit.hierarchy;
    }

    if (hit.hasCoords) {
      std::cout << "  " << std::setprecision(5) << hit.lat << "," << hit.lon;
    }

    std::cout << std::endl;
  }

  std::cout << "---" << std::endl;
  std::cout << hits.size() << " result(s)" << std::endl;
}

int main(int argc, char* argv[])
{
  osmscout::CmdLineParser   argParser("LocationLookup",
                                      argc,argv);
  std::vector<std::string>  helpArgs{"h","help"};
  Arguments                 args;

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.help=value;
                      }),
                      helpArgs,
                      "Return argument help",
                      true);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.debug=value;
                      }),
                      "debug",
                      "Enable debug output",
                      false);

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.searchForLocation=value;
                      }),
                      "location",
                      "Search for a location");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.searchForPOI=value;
                      }),
                      "poi",
                      "Search for a point of interest (POI)");

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.structuredOnly=value;
                      }),
                      "structured-only",
                      "Search the structured location index only",
                      false);

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.fulltextOnly=value;
                      }),
                      "fulltext-only",
                      "Search the text index only (requires marisa support)",
                      false);

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.defaultAdminRegion=value;
                      }),
                      "adminRegion",
                      "The default admin region to search in");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.adminRegionOnlyMatch=value;
                      }),
                      "adminRegionOnlyMatch",
                      "Return only exact matches for the admin region");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.poiOnlyMatch=value;
                      }),
                      "poiOnlyMatch",
                      "Return only exact matches for the POI");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.locationOnlyMatch=value;
                      }),
                      "locationOnlyMatch",
                      "Return only exact matches for the location");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.addressOnlyMatch=value;
                      }),
                      "addressOnlyMatch",
                      "Return only exact matches for the address");

  argParser.AddOption(osmscout::CmdLineBoolOption([&args](bool value) {
                        args.partialMatch=value;
                      }),
                      "partialMatch",
                      "Return only matches that match the complete search string");

  argParser.AddOption(osmscout::CmdLineSizeTOption([&args](size_t value) {
                        args.limit=value;
                      }),
                      "limit",
                      "Maximum number of results");

  argParser.AddOption(osmscout::CmdLineSizeTOption([&args](size_t value) {
                        args.repeat=value;
                      }),
                      "repeat",
                      "Count of repeat for performance test");

  argParser.AddOption(osmscout::CmdLineFlag([&args](const bool& value) {
                        args.transliterate=value;
                      }),
                      "transliterate",
                      "Transliterate non-ascii characters for matching",
                      false);

  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](double value) {
                        args.hasCenter=true;
                        args.lat=value;
                      }),
                      "lat",
                      "Latitude of the search center (for distance ranking)");

  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](double value) {
                        args.hasCenter=true;
                        args.lon=value;
                      }),
                      "lon",
                      "Longitude of the search center (for distance ranking)");

  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](double value) {
                        args.typeWeight=value;
                      }),
                      "type-weight",
                      "Override the type rank weight");

  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](double value) {
                        args.distanceWeight=value;
                      }),
                      "distance-weight",
                      "Override the distance rank weight");

  argParser.AddOption(osmscout::CmdLineDoubleOption([&args](double value) {
                        args.matchWeight=value;
                      }),
                      "match-weight",
                      "Override the match rank weight");

  argParser.AddOption(osmscout::CmdLineStringOption([&args](const std::string& value) {
                        args.weightString=value;
                      }),
                      "weights",
                      "Set all rank weights at once, space separated: \"T D M\"");

  argParser.AddPositional(osmscout::CmdLineStringOption([&args](const std::string& value) {
                            args.databaseDirectory=value;
                          }),
                          "DATABASE",
                          "Directory of the db to use");

  argParser.AddPositional(osmscout::CmdLineStringListOption([&args](const std::string& value) {
                            args.location.push_back(value);
                          }),
                          "LOCATION",
                          "List of location search attributes");

  osmscout::CmdLineParseResult result=argParser.Parse();

  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  if (args.help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  if (args.structuredOnly && args.fulltextOnly) {
    std::cerr << "ERROR: --structured-only and --fulltext-only are mutually exclusive" << std::endl;
    return 1;
  }

  // --weights "T D M" overrides all three rank weights at once
  if (!args.weightString.empty()) {
    std::istringstream stream(args.weightString);
    double typeWeight=0.0;
    double distanceWeight=0.0;
    double matchWeight=0.0;

    if (!(stream >> typeWeight >> distanceWeight >> matchWeight)) {
      std::cerr << "ERROR: --weights expects three numbers, e.g. \"1 0 1\"" << std::endl;
      return 1;
    }

    args.typeWeight=typeWeight;
    args.distanceWeight=distanceWeight;
    args.matchWeight=matchWeight;
  }

  osmscout::log.Debug(args.debug);
  osmscout::log.Info(true);
  osmscout::log.Warn(true);
  osmscout::log.Error(true);

  try {
    std::locale::global(std::locale(""));
  }
  catch (const std::runtime_error& e) {
    std::cerr << "Cannot set locale: \"" << e.what() << "\"" << std::endl;
  }

  std::string searchPattern;

  for (const auto& location : args.location) {
    if (!searchPattern.empty()) {
      searchPattern.append(" ");
    }

    searchPattern.append(location);
  }

  if (searchPattern.empty()) {
    std::cerr << "ERROR: No search pattern given" << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }

  osmscout::DatabaseParameter databaseParameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(databaseParameter);

  if (!database->Open(args.databaseDirectory)) {
    std::cerr << "Cannot open db" << std::endl;

    return 1;
  }

  osmscout::StringMatcherFactoryRef matcherFactory;
  if (args.transliterate) {
    matcherFactory=std::make_shared<osmscout::StringMatcherTransliterateFactory>();
  }
  else {
    matcherFactory=std::make_shared<osmscout::StringMatcherCIFactory>();
  }

  osmscout::LocationServiceRef locationService=std::make_shared<osmscout::LocationService>(database);

  osmscout::LocationStringSearchParameter searchParameter(osmscout::LocaleStringToUTF8String(searchPattern));

  searchParameter.SetSearchForLocation(args.searchForLocation);
  searchParameter.SetSearchForPOI(args.searchForPOI);
  searchParameter.SetAdminRegionOnlyMatch(args.adminRegionOnlyMatch);
  searchParameter.SetPOIOnlyMatch(args.poiOnlyMatch);
  searchParameter.SetLocationOnlyMatch(args.locationOnlyMatch);
  searchParameter.SetAddressOnlyMatch(args.addressOnlyMatch);
  searchParameter.SetPartialMatch(args.partialMatch);
  searchParameter.SetStringMatcherFactory(matcherFactory);
  searchParameter.SetLimit(args.limit);

  if (!args.defaultAdminRegion.empty()) {
    osmscout::StopClock                   adminRegionSearchTime;
    osmscout::LocationFormSearchParameter patternSearchParams;
    osmscout::LocationSearchResult        adminRegionSearchResult;

    patternSearchParams.SetStringMatcherFactory(matcherFactory);
    patternSearchParams.SetAdminRegionSearchString(args.defaultAdminRegion);
    patternSearchParams.SetLimit(50);

    if (!locationService->SearchForLocationByForm(patternSearchParams,
                                                  adminRegionSearchResult)) {
      std::cerr << "Error while resolving default admin region" << std::endl;
      return 1;
    }

    auto adminRegionEntry=std::find_if(adminRegionSearchResult.results.cbegin(),
      adminRegionSearchResult.results.cend(),
      [](const osmscout::LocationSearchResult::Entry& entry) {
        return entry.adminRegion &&
               entry.adminRegionMatchQuality==osmscout::LocationSearchResult::match;
    });

    if (adminRegionEntry!=adminRegionSearchResult.results.end()) {
      searchParameter.SetAdminRegionOnlyMatch(true);
      searchParameter.SetDefaultAdminRegion(adminRegionEntry->adminRegion);

      // No explicit center given: derive the search center from the admin
      // region's object, so distance ranking still works
      if (!args.hasCenter &&
          adminRegionEntry->adminRegion->object.Valid()) {
        std::string refType;
        std::string objectTypeName;

        if (GetObjectInfo(database,
                          adminRegionEntry->adminRegion->object,
                          refType,
                          objectTypeName,
                          args.lat,
                          args.lon)) {
          args.hasCenter=true;
          args.autoCenterRegion=adminRegionEntry->adminRegion->name;
        }
      }
    }

    adminRegionSearchTime.Stop();

    std::cout << "Admin region search time: " << adminRegionSearchTime.ResultString() << std::endl;
    std::cout << std::endl;
  }

#ifdef OSMSCOUT_HAVE_LIB_MARISA
  const bool searchStructured=!args.fulltextOnly;
  const bool searchFulltext=!args.structuredOnly;
#else
  const bool searchStructured=!args.fulltextOnly;
  const bool searchFulltext=false;
#endif

  if (args.fulltextOnly) {
#ifdef OSMSCOUT_HAVE_LIB_MARISA
    // checked below when loading the text index
#else
    std::cerr << "ERROR: This build has no fulltext support (compiled without marisa)" << std::endl;
    database->Close();
    return 1;
#endif
  }

  // Load the text index once; it is reused for every search run
#ifdef OSMSCOUT_HAVE_LIB_MARISA
  std::unique_ptr<osmscout::TextSearchIndex> textSearch;
#endif

#ifdef OSMSCOUT_HAVE_LIB_MARISA
  if (searchFulltext) {
    textSearch=std::make_unique<osmscout::TextSearchIndex>();
    if (!textSearch->Load(args.databaseDirectory)) {
      if (args.fulltextOnly) {
        std::cerr << "ERROR: Failed to load text index files in '" << args.databaseDirectory << "'" << std::endl;
        database->Close();
        return 1;
      }
      std::cerr << "WARNING: Failed to load text index files, search only for locations" << std::endl;
      textSearch.reset();
    }
  }
#endif

  std::cout << "Database:                " << args.databaseDirectory << std::endl;
  std::cout << "Search pattern:          " << searchParameter.GetSearchString() << std::endl;
  if (args.transliterate) {
    std::cout << "Transliterated pattern:  " << osmscout::UTF8Transliterate(osmscout::UTF8StringToUpper(searchParameter.GetSearchString())) << std::endl;
  }
  std::cout << "Search for location:     " << (searchParameter.GetSearchForLocation() ? "true" : "false") << std::endl;
  std::cout << "Search for POI:          " << (searchParameter.GetSearchForPOI() ? "true" : "false") << std::endl;
  std::cout << "Admin region only match: " << (searchParameter.GetAdminRegionOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "POI only match:          " << (searchParameter.GetPOIOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "Location only match:     " << (searchParameter.GetLocationOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "Address only match:      " << (searchParameter.GetAddressOnlyMatch() ? "true" : "false") << std::endl;
  std::cout << "Partial match:           " << (searchParameter.GetPartialMatch() ? "true" : "false") << std::endl;
  std::cout << "Transliterate:           " << (args.transliterate ? "true" : "false") << std::endl;
  std::cout << "Source:                  " << (args.structuredOnly ? "structured only" :
                                               args.fulltextOnly ? "fulltext only" : "both") << std::endl;
  if (args.hasCenter) {
    std::cout << "Search center:           " << args.lat << "," << args.lon;
    if (!args.autoCenterRegion.empty()) {
      std::cout << " (auto from admin region '" << args.autoCenterRegion << "')";
    }
    std::cout << std::endl;
  }
  else {
    std::cout << "Search center:           none (distance rank neutral)" << std::endl;
  }
  std::cout << "Rank weights:            T=" << args.typeWeight
            << " D=" << args.distanceWeight
            << " M=" << args.matchWeight << std::endl;

  if (searchParameter.GetDefaultAdminRegion()) {
    std::cout << "Default admin region:    " << searchParameter.GetDefaultAdminRegion()->name << " (" << searchParameter.GetDefaultAdminRegion()->object.GetName() << ")" << std::endl;
  }

  std::cout << "Limit:                   " << searchParameter.GetLimit() << std::endl;

  std::cout << std::endl;

  double structuredMs=0.0;
  double fulltextMs=0.0;

  osmscout::StopClock locationSearchTime;

  for (size_t i=0; i<args.repeat; i++) {
    std::vector<SearchHit> hits;
    size_t                 truncatedFulltext=0;

    if (searchStructured) {
      osmscout::StopClock  structuredTime;
      osmscout::LocationSearchResult searchResult;

      if (!locationService->SearchForLocationByString(searchParameter,
                                                      searchResult)) {
        std::cerr << "Error while searching for location" << std::endl;
        return 1;
      }

      structuredTime.Stop();
      structuredMs+=structuredTime.GetMilliseconds();

      if (args.repeat==1) {
        for (const auto& entry : searchResult.results) {
          hits.push_back(BuildStructuredHit(database,
                                            locationService,
                                            entry,
                                            searchPattern,
                                            args.typeWeight,
                                            args.distanceWeight,
                                            args.matchWeight,
                                            args.hasCenter,
                                            args.lat,
                                            args.lon));
        }
      }
    }

#ifdef OSMSCOUT_HAVE_LIB_MARISA
    if (searchFulltext && textSearch) {
      osmscout::StopClock fulltextTime;
      osmscout::TextSearchIndex::ResultsMap resultsTxt;

      textSearch->Search(osmscout::LocaleStringToUTF8String(searchPattern),
                         /*searchPOIs*/ true, /*searchLocations*/ true,
                         /*searchRegions*/ true, /*searchOther*/ true,
                         /*transliterate*/ true,
                         resultsTxt);

      fulltextTime.Stop();
      fulltextMs+=fulltextTime.GetMilliseconds();

      if (args.repeat==1) {
        osmscout::TypeConfigRef          typeConfig=database->GetTypeConfig();
        osmscout::NameFeatureLabelReader nameLabelReader(*typeConfig);

        // Offset of structured results, so free-text hits of the same
        // object are not returned twice (mirrors the JNI bridge)
        std::set<long long> seenOffsets;
        for (const auto& hit : hits) {
          seenOffsets.insert(hit.objectFileOffset);
        }

        for (const auto& entry : resultsTxt) {
          for (const auto& ref : entry.second) {
            if (hits.size()>=args.limit) {
              truncatedFulltext++;
              continue;
            }
            if (seenOffsets.count(static_cast<long long>(ref.GetFileOffset()))!=0) {
              continue;
            }
            seenOffsets.insert(static_cast<long long>(ref.GetFileOffset()));
            hits.push_back(BuildFreeTextHit(database,
                                            ref,
                                            entry.first,
                                            nameLabelReader,
                                            searchPattern,
                                            args.typeWeight,
                                            args.distanceWeight,
                                            args.matchWeight,
                                            args.hasCenter,
                                            args.lat,
                                            args.lon));
          }
        }
      }
    }
#endif

    if (args.repeat==1) {
      // Sort: exact matches first, then by rank descending (mirrors
      // JavaScout's LocationSearchRanker comparator)
      std::sort(hits.begin(),
                hits.end(),
                [](const SearchHit& a, const SearchHit& b) {
                  if (a.matchQuality!=b.matchQuality) {
                    return a.matchQuality=="match";
                  }
                  return a.rank>b.rank;
                });

      PrintResults(hits,
                   args.hasCenter);

      if (truncatedFulltext>0) {
        std::cout << "truncated: " << truncatedFulltext << " fulltext hit(s) cut by limit" << std::endl;
      }

      std::cout << std::endl;
    }
  }

  locationSearchTime.Stop();

  std::cout << "Location search time: " << locationSearchTime.ResultString();
  if (searchStructured || searchFulltext) {
    std::cout << " (structured " << std::fixed << std::setprecision(1) << structuredMs
              << "ms";
    if (searchFulltext) {
      std::cout << ", fulltext " << std::fixed << std::setprecision(1) << fulltextMs
                << "ms";
    }
    std::cout << ")";
  }
  std::cout << std::endl;

  database->Close();

  return 0;
}
