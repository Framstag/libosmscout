#include <LaneEvaluationCommon.h>
#include <LaneEvaluationCompare.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <random>

#include <osmscout/cli/CmdLineParsing.h>
#include <osmscout/db/Database.h>
#include <osmscout/io/FileScanner.h>
#include <osmscout/Way.h>
#include <osmscout/routing/RoutingProfile.h>

using namespace osmscout;

static int Describe(int argc, char *argv[])
{
  CmdLineParser argParser("LaneEvaluation describe", argc, argv);

  bool help = false;
  std::string databaseDirectory;
  GeoCoord start;
  GeoCoord target;
  std::string output;

  argParser.AddOption(CmdLineFlag([&help](const bool &value) { help = value; }),
                      std::vector<std::string>{"h", "help"},
                      "Display help", true);

  argParser.AddOption(CmdLineStringOption([&output](const std::string &value) { output = value; }),
                      "output", "Output JSON file (default: stdout)", false);

  argParser.AddPositional(CmdLineStringOption([&databaseDirectory](const std::string &value) {
                            databaseDirectory = value;
                          }),
                          "DATABASE", "Database directory");

  argParser.AddPositional(CmdLineGeoCoordOption([&start](const GeoCoord &value) {
                            start = value;
                          }),
                          "START", "Start coordinate (lat,lon)");

  argParser.AddPositional(CmdLineGeoCoordOption([&target](const GeoCoord &value) {
                            target = value;
                          }),
                          "TARGET", "Target coordinate (lat,lon)");

  auto result = argParser.Parse();
  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  if (help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  RouteDescription description;
  if (!ComputeRoute(databaseDirectory, start, target, description)) {
    return 1;
  }

  if (output.empty()) {
    WriteRouteJson(description, start, target, databaseDirectory, std::cout);
  } else {
    std::ofstream out(output, std::ios::binary);
    if (!out.is_open()) {
      std::cerr << "Cannot open output file: " << output << std::endl;
      return 1;
    }
    WriteRouteJson(description, start, target, databaseDirectory, out);
  }

  return 0;
}

static int Generate(int argc, char *argv[])
{
  CmdLineParser argParser("LaneEvaluation generate", argc, argv);

  bool help = false;
  std::string databaseDirectory;
  std::string outputDir;
  int count = 100;
  int seed = -1;

  argParser.AddOption(CmdLineFlag([&help](const bool &value) { help = value; }),
                      std::vector<std::string>{"h", "help"},
                      "Display help", true);

  argParser.AddOption(CmdLineIntOption([&count](int value) { count = value; }),
                      "count", "Number of routes to generate (default: 100)", false);

  argParser.AddOption(CmdLineIntOption([&seed](int value) { seed = value; }),
                      "seed", "Random seed (default: random)", false);

  argParser.AddPositional(CmdLineStringOption([&databaseDirectory](const std::string &value) {
                            databaseDirectory = value;
                          }),
                          "DATABASE", "Database directory");

  argParser.AddPositional(CmdLineStringOption([&outputDir](const std::string &value) {
                            outputDir = value;
                          }),
                          "OUTPUT_DIR", "Output directory for JSON files");

  auto result = argParser.Parse();
  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  if (help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  DatabaseParameter databaseParameter;
  auto database = std::make_shared<Database>(databaseParameter);
  if (!database->Open(databaseDirectory)) {
    std::cerr << "Cannot open database: " << databaseDirectory << std::endl;
    return 1;
  }

  auto routingProfile = std::make_shared<FastestPathRoutingProfile>(database->GetTypeConfig());
  TypeConfigRef typeConfig = database->GetTypeConfig();
  std::map<std::string, double> carSpeedTable;
  GetCarSpeedTable(carSpeedTable);
  routingProfile->ParametrizeForCar(*typeConfig, carSpeedTable, 160.0);

  std::cout << "Reading routable ways from database..." << std::endl;

  std::vector<GeoCoord> routableCoords;
  try {
    FileScanner wayScanner;
    wayScanner.Open(database->GetWayDataFile()->GetFilename(), FileScanner::Sequential, false);
    uint32_t wayCount = wayScanner.ReadUInt32();

    for (uint32_t n = 0; n < wayCount; n++) {
      Way way;
      way.Read(*typeConfig, wayScanner);
      if (routingProfile->CanUse(way) &&
          std::any_of(way.nodes.begin(), way.nodes.end(),
                      [](const Point &node) { return node.IsRelevant(); })) {
        size_t midIdx = way.nodes.size() / 2;
        routableCoords.push_back(way.nodes[midIdx].GetCoord());
      }
    }
    wayScanner.Close();
  } catch (IOException &e) {
    std::cerr << "Failed to read ways: " << e.GetDescription() << std::endl;
    return 1;
  }

  std::cout << "Found " << routableCoords.size() << " routable ways." << std::endl;

  if (routableCoords.size() < 2) {
    std::cerr << "Not enough routable ways found." << std::endl;
    return 1;
  }

  std::filesystem::create_directories(outputDir);

  std::mt19937 rng;
  if (seed >= 0) {
    rng.seed(static_cast<unsigned>(seed));
  } else {
    std::random_device rd;
    rng.seed(rd());
  }

  std::uniform_int_distribution<size_t> dist(0, routableCoords.size() - 1);

  int successCount = 0;
  int attempts = 0;
  int maxAttempts = count * 3;

  std::cout << "Generating " << count << " random routes..." << std::endl;

  while (successCount < count && attempts < maxAttempts) {
    attempts++;

    size_t fromIdx = dist(rng);
    size_t toIdx = dist(rng);
    if (fromIdx == toIdx) {
      continue;
    }

    GeoCoord from = routableCoords[fromIdx];
    GeoCoord to = routableCoords[toIdx];

    RouteDescription description;
    if (!ComputeRoute(databaseDirectory, from, to, description)) {
      continue;
    }

    std::string filename = RouteFilename(from, to);
    std::string filepath = outputDir + "/" + filename;

    std::ofstream out(filepath, std::ios::binary);
    if (!out.is_open()) {
      std::cerr << "Cannot write to: " << filepath << std::endl;
      continue;
    }

    WriteRouteJson(description, from, to, databaseDirectory, out);
    successCount++;
    std::cout << "[" << successCount << "/" << count << "] " << filename << std::endl;
  }

  std::cout << "\nDone: " << successCount << " routes generated, "
            << (attempts - successCount) << " failed attempts." << std::endl;

  return 0;
}

static int Compare(int argc, char *argv[])
{
  CmdLineParser argParser("LaneEvaluation compare", argc, argv);

  bool help = false;
  std::string oldPath;
  std::string newPath;
  std::string imagesDir;
  std::string stylesheet;
  std::string databaseDir;

  argParser.AddOption(CmdLineFlag([&help](const bool &value) { help = value; }),
                      std::vector<std::string>{"h", "help"},
                      "Display help", true);

  argParser.AddOption(CmdLineStringOption([&imagesDir](const std::string &value) { imagesDir = value; }),
                      "images", "Output directory for comparison images (requires --database and --stylesheet)", false);

  argParser.AddOption(CmdLineStringOption([&stylesheet](const std::string &value) { stylesheet = value; }),
                      "stylesheet", "Stylesheet for map rendering (.oss file)", false);

  argParser.AddOption(CmdLineStringOption([&databaseDir](const std::string &value) { databaseDir = value; }),
                      "database", "Database directory for map rendering", false);

  argParser.AddPositional(CmdLineStringOption([&oldPath](const std::string &value) {
                            oldPath = value;
                          }),
                          "OLD_PATH", "Old route JSON file or directory");

  argParser.AddPositional(CmdLineStringOption([&newPath](const std::string &value) {
                            newPath = value;
                          }),
                          "NEW_PATH", "New route JSON file or directory");

  auto result = argParser.Parse();
  if (result.HasError()) {
    std::cerr << "ERROR: " << result.GetErrorDescription() << std::endl;
    std::cout << argParser.GetHelp() << std::endl;
    return 1;
  }
  if (help) {
    std::cout << argParser.GetHelp() << std::endl;
    return 0;
  }

  CompareOptions options;
  options.imagesDir = imagesDir;
  options.stylesheet = stylesheet;
  options.databaseDir = databaseDir;

  return CompareRoutes(oldPath, newPath, options);
}

static void PrintUsage()
{
  std::cout << "Usage: LaneEvaluation <subcommand> [options]\n\n"
            << "Subcommands:\n"
            << "  describe  Compute route and export JSON with lane info\n"
            << "  generate  Generate random routes and export JSON files\n"
            << "  compare   Compare two route descriptions for lane differences\n"
            << "\nRun 'LaneEvaluation <subcommand> --help' for subcommand options.\n";
}

int main(int argc, char *argv[])
{
  if (argc < 2) {
    PrintUsage();
    return 1;
  }

  std::string subcommand = argv[1];

  if (subcommand == "describe") {
    return Describe(argc - 1, argv + 1);
  } else if (subcommand == "generate") {
    return Generate(argc - 1, argv + 1);
  } else if (subcommand == "compare") {
    return Compare(argc - 1, argv + 1);
  } else if (subcommand == "--help" || subcommand == "-h") {
    PrintUsage();
    return 0;
  } else {
    std::cerr << "Unknown subcommand: " << subcommand << std::endl;
    PrintUsage();
    return 1;
  }
}
