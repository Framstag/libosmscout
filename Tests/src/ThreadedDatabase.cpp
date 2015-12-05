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

static const size_t DATAFILEACCESS_THREAD_COUNT=100;
static const size_t DATAFILEACCESS_ITERATION_COUNT=1000000;

static const size_t AREAINDEXACCESS_THREAD_COUNT=5;
static const size_t AREAINDEXACCESS_ITERATION_COUNT=300;

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
  osmscout::TypeInfoSet                nodeTypes;
  osmscout::TypeInfoSet                areaTypes;
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
    osmscout::AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();
    osmscout::AreaWayIndexRef areaWayIndex=database->GetAreaWayIndex();
    osmscout::AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();

    if (!areaNodeIndex ||
        !areaWayIndex ||
        !areaAreaIndex) {
      result=false;
    }

    for (auto& testData : testDataSet) {
      std::vector<osmscout::FileOffset>    nodeOffsets;
      osmscout::TypeInfoSet                loadedNodeTypes;

      std::vector<osmscout::FileOffset>    wayOffsets;
      osmscout::TypeInfoSet                loadedWayTypes;

      std::vector<osmscout::DataBlockSpan> areaOffsets;
      osmscout::TypeInfoSet                loadedAreaTypes;

      if (!areaNodeIndex->GetOffsets(testData.boundingBox,
                                     testData.nodeTypes,
                                     nodeOffsets,
                                     loadedNodeTypes)) {
        result=false;
      }

      if (!areaWayIndex->GetOffsets(testData.boundingBox,
                                    testData.wayTypes,
                                    wayOffsets,
                                    loadedWayTypes)) {
        result=false;
      }

      if (!areaAreaIndex->GetAreasInArea(*testData.typeConfig,
                                         testData.boundingBox,
                                         30,
                                         testData.areaTypes,
                                         areaOffsets,
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

      if (testData.areaOffsets!=areaOffsets) {
        result=false;
      }
    }
  }
}

bool TestAreaIndexAcceess(osmscout::DatabaseRef& database,
                          size_t threadCount,
                          size_t iterationCount)
{
  bool result=true;

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();
  osmscout::TypeInfoSet   nodeTypes;
  osmscout::TypeInfoSet   areaTypes;
  osmscout::TypeInfoSet   wayTypes;

  std::cout << "Collection test data..." << std::endl;

  for (const auto& type : typeConfig->GetTypes()) {
    if (type->CanBeNode()) {
      nodeTypes.Set(type);
    }

    if (type->CanBeWay()) {
      wayTypes.Set(type);
    }

    if (type->CanBeArea()) {
      areaTypes.Set(type);
    }
  }

  std::cout << " - " << nodeTypes.Size() << " node type(s)" << std::endl;
  std::cout << " - " << wayTypes.Size() << " way type(s)" << std::endl;
  std::cout << " - " << areaTypes.Size() << " area type(s)" << std::endl;

  osmscout::GeoBox mapBoundingBox;

  database->GetBoundingBox(mapBoundingBox);

  std::list<AreaIndexTestData> testDataSet;

  AreaIndexTestData testData;

  testData.typeConfig=typeConfig;
  testData.nodeTypes=nodeTypes;
  testData.wayTypes=wayTypes;
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

  osmscout::AreaNodeIndexRef areaNodeIndex=database->GetAreaNodeIndex();
  osmscout::AreaWayIndexRef  areaWayIndex=database->GetAreaWayIndex();
  osmscout::AreaAreaIndexRef areaAreaIndex=database->GetAreaAreaIndex();

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
                                       30,
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
  if (argc!=2) {
    std::cerr << "ThreadedDatabase <database directory>" << std::endl;

    return 1;
  }

  osmscout::DatabaseParameter parameter;
  osmscout::DatabaseRef       database=std::make_shared<osmscout::Database>(parameter);

  std::cout << "Opening database..." << std::endl;

  if (!database->Open(argv[1])) {
    std::cerr << "Cannot open database" << std::endl;

    return 1;
  }

  std::cout << "Done." << std::endl;

  /*
  std::cout << "Testing data file access with " << DATAFILEACCESS_THREAD_COUNT << " threads iterating " << DATAFILEACCESS_ITERATION_COUNT << " times each..." << std::endl;

  if (TestDatafilesAcceess(database,
                           DATAFILEACCESS_THREAD_COUNT,
                           DATAFILEACCESS_ITERATION_COUNT)) {
    std::cout << "Test result: OK" << std::endl;
  }
  else {
    std::cout << "Test result: ERROR" << std::endl;
  }*/

  std::cout << "Testing area index access with " << AREAINDEXACCESS_THREAD_COUNT << " threads iterating " << AREAINDEXACCESS_ITERATION_COUNT << " times..." << std::endl;

  if (TestAreaIndexAcceess(database,
                           AREAINDEXACCESS_THREAD_COUNT,
                           AREAINDEXACCESS_THREAD_COUNT)) {
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
