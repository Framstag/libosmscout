/*
  ThreadedDatabase - a test program for libosmscout
  Copyright (C) 2015  Tim Teulings

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

#include <cstdlib>
#include <iostream>
#include <thread>
#include <vector>

#include <osmscout/Database.h>
#include <osmscout/StyleConfig.h>

static const size_t DATAFILEACCESS_THREAD_COUNT=100;
static const size_t DATAFILEACCESS_ITERATION_COUNT=1000000;

static const size_t AREAINDEXACCESS_THREAD_COUNT=10;
static const size_t AREAINDEXACCESS_ITERATION_COUNT=100;
static const size_t AREAINDEXACCESS_AREA_LEVEL=10;

//
// Datafile access
//

void AccessDatafiles(osmscout::DatabaseRef& database,
                     size_t iterationCount,
                     bool& result)
{
  result=true;

  for (size_t i=1; i<=iterationCount; i++) {
    osmscout::NodeDataFileRef         nodeDataFile=database->GetNodeDataFile();
    osmscout::WayDataFileRef          wayDataFile=database->GetWayDataFile();
    osmscout::AreaDataFileRef         areaDataFile=database->GetAreaDataFile();

    osmscout::AreaNodeIndexRef        areaNodeIndex=database->GetAreaNodeIndex();
    osmscout::AreaWayIndexRef         areaWayIndex=database->GetAreaWayIndex();
    osmscout::AreaAreaIndexRef        areaAreaIndex=database->GetAreaAreaIndex();

    osmscout::LocationIndexRef        locationIndex=database->GetLocationIndex();
    osmscout::WaterIndexRef           waterIndex=database->GetWaterIndex();

    osmscout::OptimizeWaysLowZoomRef  optimizedWayIndex=database->GetOptimizeWaysLowZoom();
    osmscout::OptimizeAreasLowZoomRef optimizedAreaIndex=database->GetOptimizeAreasLowZoom();

    if (!nodeDataFile ||
        !wayDataFile ||
        !areaDataFile ||
        !areaNodeIndex ||
        !areaWayIndex ||
        !areaAreaIndex ||
        !locationIndex ||
        !waterIndex ||
        !optimizedWayIndex ||
        !optimizedAreaIndex) {
      result=false;
    }
  }
}

bool TestDatafilesAcceess(osmscout::DatabaseRef& database,
                          size_t threadCount,
                          size_t iterationCount)
{
  bool                     result=true;
  std::vector<std::thread> threads(threadCount);
  bool                     *results;

  results=new bool[threadCount];

  for (size_t i=0; i<threads.size(); i++) {
    threads[i]=std::thread(AccessDatafiles,
                           std::ref(database),
                           iterationCount,
                           std::ref(results[i]));
  }

  for (size_t i=0; i<threads.size(); i++) {
    threads[i].join();

    if (!results[i]) {
      result=false;
    }
  }

  delete [] results;

  return result;
}

//
// Area index access
//

struct AreaIndexTestData
{
  osmscout::GeoBox                     boundingBox;

  osmscout::TypeConfigRef              typeConfig;

  osmscout::Magnification              magnification;

  osmscout::TypeInfoSet                nodeTypes;
  osmscout::TypeInfoSet                optimizedAreaTypes;
  osmscout::TypeInfoSet                areaTypes;
  osmscout::TypeInfoSet                optimizedWayTypes;
  osmscout::TypeInfoSet                wayTypes;

  std::vector<osmscout::FileOffset>    nodeOffsets;
  osmscout::TypeInfoSet                loadedNodeTypes;

  std::vector<osmscout::FileOffset>    wayOffsets;
  osmscout::TypeInfoSet                loadedWayTypes;

  std::vector<osmscout::DataBlockSpan> areaOffsets;
  osmscout::TypeInfoSet                loadedAreaTypes;
};

void AccessAreaIndex(osmscout::DatabaseRef& database,
                     size_t iterationCount,
                     const std::list<AreaIndexTestData>& testDataSet,
                     bool& result)
{
  result=true;

  for (size_t i=1; i<=iterationCount; i++) {
    osmscout::NodeDataFileRef         nodeDataFile=database->GetNodeDataFile();
    osmscout::WayDataFileRef          wayDataFile=database->GetWayDataFile();
    osmscout::AreaDataFileRef         areaDataFile=database->GetAreaDataFile();

    osmscout::AreaNodeIndexRef        areaNodeIndex=database->GetAreaNodeIndex();
    osmscout::AreaWayIndexRef         areaWayIndex=database->GetAreaWayIndex();
    osmscout::AreaAreaIndexRef        areaAreaIndex=database->GetAreaAreaIndex();

    osmscout::OptimizeWaysLowZoomRef  areaOptimizedWayIndex=database->GetOptimizeWaysLowZoom();
    osmscout::OptimizeAreasLowZoomRef areaOptimizedAreaIndex=database->GetOptimizeAreasLowZoom();

    osmscout::WaterIndexRef           waterIndex=database->GetWaterIndex();

    if (!nodeDataFile ||
        !wayDataFile ||
        !areaDataFile ||
        !areaNodeIndex ||
        !areaWayIndex ||
        !areaAreaIndex ||
        !areaOptimizedWayIndex ||
        !areaOptimizedAreaIndex) {
      result=false;
    }

    if ((i % 5) == 0) {
      std::cout << "Thread " << std::this_thread::get_id() << ": iteration " << i << std::endl;
    }

    for (auto& testData : testDataSet) {
      std::vector<osmscout::FileOffset>    nodeOffsets;
      osmscout::TypeInfoSet                loadedNodeTypes;
      std::vector<osmscout::NodeRef>       nodeData;

      std::vector<osmscout::FileOffset>    wayOffsets;
      osmscout::TypeInfoSet                loadedWayTypes;
      osmscout::TypeInfoSet                loadedOptimizedWayTypes;
      std::vector<osmscout::WayRef>        optimizedWayData;
      std::vector<osmscout::WayRef>        wayData;

      std::vector<osmscout::DataBlockSpan> areaSpans;
      osmscout::TypeInfoSet                loadedAreaTypes;
      osmscout::TypeInfoSet                loadedOptimizedAreaTypes;
      std::vector<osmscout::AreaRef>       optimizedAreaData;
      std::vector<osmscout::AreaRef>       areaData;

      std::list<osmscout::GroundTile>      tiles;

      if (!areaNodeIndex->GetOffsets(testData.boundingBox,
                                     testData.nodeTypes,
                                     nodeOffsets,
                                     loadedNodeTypes)) {
        result=false;
      }

      if (areaOptimizedWayIndex->GetWays(testData.boundingBox,
                                         testData.magnification,
                                         testData.optimizedWayTypes,
                                         optimizedWayData,
                                         loadedOptimizedWayTypes)) {

      }

      if (!areaWayIndex->GetOffsets(testData.boundingBox,
                                    testData.wayTypes,
                                    wayOffsets,
                                    loadedWayTypes)) {
        result=false;
      }

      if (areaOptimizedAreaIndex->GetAreas(testData.boundingBox,
                                           testData.magnification,
                                           testData.optimizedAreaTypes,
                                           optimizedAreaData,
                                           loadedOptimizedAreaTypes)) {

      }

      if (!areaAreaIndex->GetAreasInArea(*testData.typeConfig,
                                         testData.boundingBox,
                                         AREAINDEXACCESS_AREA_LEVEL+4,
                                         testData.areaTypes,
                                         areaSpans,
                                         loadedAreaTypes)) {
        result=false;
      }

      if (testData.loadedNodeTypes!=loadedNodeTypes) {
        result=false;
      }

      if (testData.nodeOffsets!=nodeOffsets) {
        result=false;
      }

      if (testData.loadedWayTypes!=loadedWayTypes) {
        result=false;
      }

      if (testData.wayOffsets!=wayOffsets) {
        result=false;
      }

      if (testData.loadedAreaTypes!=loadedAreaTypes) {
        result=false;
      }

      if (testData.areaOffsets!=areaSpans) {
        result=false;
      }

      if (!nodeDataFile->GetByOffset(nodeOffsets.begin(),
                                     nodeOffsets.end(),
                                     nodeOffsets.size(),
                                     nodeData)) {
        result=false;
      }

      if (!wayDataFile->GetByOffset(wayOffsets.begin(),
                                    wayOffsets.end(),
                                    wayOffsets.size(),
                                    wayData)) {
        result=false;
      }

      if (!areaDataFile->GetByBlockSpans(areaSpans.begin(),
                                         areaSpans.end(),
                                         areaData)) {
        result=false;
      }

      if (!waterIndex->GetRegions(testData.boundingBox,
                                  testData.magnification,
                                  tiles)) {
        result=false;
      }

      std::vector<osmscout::FileOffset> loadedNodeOffsets;
      std::vector<osmscout::FileOffset> loadedWayOffsets;
      std::vector<osmscout::FileOffset> loadedAreaOffsets;

      for (const auto& node : nodeData) {
        loadedNodeOffsets.push_back(node->GetFileOffset());
      }

      for (const auto& way : wayData) {
        loadedWayOffsets.push_back(way->GetFileOffset());
      }

      for (const auto& area : areaData) {
        loadedAreaOffsets.push_back(area->GetFileOffset());
      }

      if (nodeOffsets!=loadedNodeOffsets) {
        result=false;
      }

      if (wayOffsets!=loadedWayOffsets) {
        result=false;
      }

      /*
      if (areaOffsets!=loadedAreaOffsets) {
        result=false;
      }*/
    }
  }
}

bool TestAreaIndexAcceess(osmscout::DatabaseRef& database,
                          osmscout::StyleConfig& styleConfig,
                          size_t threadCount,
                          size_t iterationCount)
{
  bool result=true;

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();
  osmscout::TypeInfoSet   nodeTypes;
  osmscout::TypeInfoSet   optimizedWayTypes;
  osmscout::TypeInfoSet   wayTypes;
  osmscout::TypeInfoSet   optimizedAreaTypes;
  osmscout::TypeInfoSet   areaTypes;
  osmscout::Magnification magnification;

  magnification.SetLevel(AREAINDEXACCESS_AREA_LEVEL);

  osmscout::AreaNodeIndexRef        areaNodeIndex=database->GetAreaNodeIndex();
  osmscout::AreaWayIndexRef         areaWayIndex=database->GetAreaWayIndex();
  osmscout::OptimizeWaysLowZoomRef  areaOptimizedWayIndex=database->GetOptimizeWaysLowZoom();
  osmscout::AreaAreaIndexRef        areaAreaIndex=database->GetAreaAreaIndex();
  osmscout::OptimizeAreasLowZoomRef areaOptimizedAreaIndex=database->GetOptimizeAreasLowZoom();

  std::cout << "Collection test data..." << std::endl;

  styleConfig.GetNodeTypesWithMaxMag(magnification,
                                     nodeTypes);

  styleConfig.GetWayTypesWithMaxMag(magnification,
                                    wayTypes);

  styleConfig.GetAreaTypesWithMaxMag(magnification,
                                     areaTypes);

  if (areaOptimizedWayIndex->HasOptimizations(magnification.GetMagnification())) {
    areaOptimizedWayIndex->GetTypes(magnification,
                                    wayTypes,
                                    optimizedWayTypes);

    wayTypes.Remove(optimizedWayTypes);
  }

  if (areaOptimizedAreaIndex->HasOptimizations(magnification.GetMagnification())) {
    areaOptimizedAreaIndex->GetTypes(magnification,
                                     areaTypes,
                                     optimizedAreaTypes);

    areaTypes.Remove(optimizedAreaTypes);
  }

  std::cout << " - " << nodeTypes.Size() << " node type(s)" << std::endl;
  std::cout << " - " << optimizedWayTypes.Size() << " optimized way type(s)" << std::endl;
  std::cout << " - " << wayTypes.Size() << " way type(s)" << std::endl;
  std::cout << " - " << optimizedAreaTypes.Size() << " optimized area type(s)" << std::endl;
  std::cout << " - " << areaTypes.Size() << " area type(s)" << std::endl;

  osmscout::GeoBox mapBoundingBox;

  database->GetBoundingBox(mapBoundingBox);

  std::list<AreaIndexTestData> testDataSet;

  AreaIndexTestData testData;

  testData.typeConfig=typeConfig;
  testData.magnification=magnification;
  testData.nodeTypes=nodeTypes;
  testData.optimizedWayTypes=optimizedWayTypes;
  testData.wayTypes=wayTypes;
  testData.optimizedAreaTypes=optimizedAreaTypes;
  testData.areaTypes=areaTypes;
  testData.boundingBox=mapBoundingBox;

  testDataSet.push_back(testData);

  testData.boundingBox=osmscout::GeoBox(osmscout::GeoCoord(mapBoundingBox.GetMinLat()+mapBoundingBox.GetHeight()/3,
                                                           mapBoundingBox.GetMinLon()+mapBoundingBox.GetWidth()/3),
                                        osmscout::GeoCoord(mapBoundingBox.GetMaxLat()-mapBoundingBox.GetHeight()/3,
                                                           mapBoundingBox.GetMaxLon()-mapBoundingBox.GetWidth()/3));

  testDataSet.push_back(testData);

  testData.boundingBox=osmscout::GeoBox(osmscout::GeoCoord(mapBoundingBox.GetMinLat()+4*mapBoundingBox.GetHeight()/10,
                                                           mapBoundingBox.GetMinLon()+4*mapBoundingBox.GetWidth()/10),
                                        osmscout::GeoCoord(mapBoundingBox.GetMaxLat()-4*mapBoundingBox.GetHeight()/10,
                                                           mapBoundingBox.GetMaxLon()-4*mapBoundingBox.GetWidth()/10));

  testDataSet.push_back(testData);

  for (auto& testData : testDataSet) {
    std::cout << " - BoundingBox " << testData.boundingBox.GetDisplayText() << ":" << std::endl;

    if (!areaNodeIndex->GetOffsets(testData.boundingBox,
                                   testData.nodeTypes,
                                   testData.nodeOffsets,
                                   testData.loadedNodeTypes)) {
      return false;
    }

    std::cout << "   * " << testData.nodeOffsets.size() << " node offset(s)" << std::endl;

    if (!areaWayIndex->GetOffsets(testData.boundingBox,
                                  testData.wayTypes,
                                  testData.wayOffsets,
                                  testData.loadedWayTypes)) {
      return false;
    }

    std::cout << "   * " << testData.wayOffsets.size() << " way offset(s)" << std::endl;

    if (!areaAreaIndex->GetAreasInArea(*testData.typeConfig,
                                       testData.boundingBox,
                                       AREAINDEXACCESS_AREA_LEVEL+4,
                                       testData.areaTypes,
                                       testData.areaOffsets,
                                       testData.loadedAreaTypes)) {
      return false;
    }

    std::cout << "   * " << testData.areaOffsets.size() << " area offset(s)" << std::endl;
  }

  std::cout << "Starting retrieval threads..." << std::endl;

  std::vector<std::thread> threads(threadCount);
  bool                     *results;

  results=new bool[threadCount];

  for (size_t i=0; i<threads.size(); i++) {
    threads[i]=std::thread(AccessAreaIndex,
                           std::ref(database),
                           iterationCount,
                           std::ref(testDataSet),
                           std::ref(results[i]));
  }

  for (size_t i=0; i<threads.size(); i++) {
    threads[i].join();

    if (!results[i]) {
      result=false;
    }
  }

  delete [] results;

  std::cout << "Threads finished." << std::endl;

  return result;
}

int main(int argc, char* argv[])
{
  if (argc!=3) {
    std::cerr << "ThreadedDatabase <database directory> <style config>" << std::endl;

    return 1;
  }

  // Database

  osmscout::DatabaseParameter parameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(parameter);

  std::cout << "Opening database..." << std::endl;

  if (!database->Open(argv[1])) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  // Style Config

  std::cout << "Loading style config..." << std::endl;

  osmscout::StyleConfigRef styleConfig=std::make_shared<osmscout::StyleConfig>(database->GetTypeConfig());

  if (!styleConfig->Load(argv[2])) {
    std::cerr << "Cannot open style config" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  std::cout << "Testing data file access with " << DATAFILEACCESS_THREAD_COUNT << " threads iterating " << DATAFILEACCESS_ITERATION_COUNT << " times each..." << std::endl;

  if (TestDatafilesAcceess(database,
                           DATAFILEACCESS_THREAD_COUNT,
                           DATAFILEACCESS_ITERATION_COUNT)) {
    std::cout << "Test result: OK" << std::endl;
  }
  else {
    std::cout << "Test result: ERROR" << std::endl;
  }

  std::cout << "Testing area index access with " << AREAINDEXACCESS_THREAD_COUNT << " threads iterating " << AREAINDEXACCESS_ITERATION_COUNT << " times..." << std::endl;

  if (TestAreaIndexAcceess(database,
                           *styleConfig,
                           AREAINDEXACCESS_THREAD_COUNT,
                           AREAINDEXACCESS_ITERATION_COUNT)) {
    std::cout << "Test result: OK" << std::endl;
  }
  else {
    std::cout << "Test result: ERROR" << std::endl;
  }

  std::cout << "Closing database..." << std::endl;
  database->Close();
  database=NULL;
  std::cout << "Done." << std::endl;

  return 0;
}
