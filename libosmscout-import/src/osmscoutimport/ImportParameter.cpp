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

#include <osmscoutimport/ImportParameter.h>
#include <osmscoutimport/Preprocessor.h>
#include <osmscoutimport/ImportFeatures.h>

#include <thread>

namespace osmscout {


static const size_t defaultStartStep=1;
#if defined(OSMSCOUT_IMPORT_HAVE_LIB_MARISA)
static const size_t defaultEndStep=28;
#else
static const size_t defaultEndStep=27;
#endif

size_t ImportParameter::GetDefaultStartStep()
{
  return defaultStartStep;
}

size_t ImportParameter::GetDefaultEndStep()
{
  return defaultEndStep;
}


ImportParameter::Router::Router(uint8_t vehicleMask,
                                const std::string& filenamebase)
    : vehicleMask(vehicleMask),
      filenamebase(filenamebase)
{
  // no code
}

ImportParameter::ImportParameter()
    : typefile("map.ost"),
      startStep(defaultStartStep),
      endStep(defaultEndStep),
      eco(false),
      strictAreas(false),
      sortObjects(true),
      sortBlockSize(40000000),
      sortTileMag(14),
      processingQueueSize(std::max((unsigned int)1,std::thread::hardware_concurrency())),
      numericIndexPageSize(1024),
      rawCoordBlockSize(60000000),
      rawNodeDataMemoryMaped(false),
      rawWayIndexMemoryMaped(true),
      rawWayDataMemoryMaped(false),
      rawWayIndexCacheSize(10000),
      rawWayBlockSize(500000),
      coordDataMemoryMaped(false),
      coordIndexCacheSize(1000000),
      coordBlockSize(250000),
      relMaxWays(1500),
      relMaxCoords(150000),
      areaDataMemoryMaped(false),
      areaDataCacheSize(0),
      wayDataMemoryMaped(false),
      wayDataCacheSize(0),
      areaAreaIndexMaxMag(17),
      areaNodeGridMag(14),
      areaNodeSimpleListLimit(500),
      areaNodeTileListLimit(100),
      areaNodeTileListCoordLimit(1000),
      areaNodeBitmapMaxMag(20),
      areaNodeBitmapLimit(20),
      areaWayIndexMinMag(10), // Should not be >= than areaWayIndexMaxMag
      areaWayIndexMaxMag(15),
      areaRouteIndexMinMag(10),
      areaRouteIndexMaxMag(15),
      waterIndexMinMag(6),
      waterIndexMaxMag(14),
      optimizationMaxWayCount(1000000),
      optimizationMaxMag(10),
      optimizationMinMag(0),
      optimizationCellSizeAverage(64),
      optimizationCellSizeMax(255),
      optimizationWayMethod(TransPolygon::quality),
      routeNodeBlockSize(500000),
      routeNodeTileMag(13),
      assumeLand(AssumeLandStrategy::automatic),
      langOrder({"#"}),
      maxAdminLevel(10),
      firstFreeOSMId(((OSMId)1) << ((sizeof(OSMId)*8)-2)),
      fillWaterArea(20),
      textIndexVariant(TextIndexVariant::transliterate)
{
  // no code
}

ImportParameter::~ImportParameter()
{

}

const std::list<std::string>& ImportParameter::GetMapfiles() const
{
  return mapfiles;
}

std::string ImportParameter::GetTypefile() const
{
  return typefile;
}

std::string ImportParameter::GetDestinationDirectory() const
{
  return destinationDirectory;
}

std::string ImportParameter::GetBoundingPolygonFile() const
{
  return boundingPolygonFile;
}

ImportErrorReporterRef ImportParameter::GetErrorReporter() const
{
  return errorReporter;
}

size_t ImportParameter::GetStartStep() const
{
  return startStep;
}

size_t ImportParameter::GetEndStep() const
{
  return endStep;
}

bool ImportParameter::IsEco() const
{
  return eco;
}

const std::list<ImportParameter::Router>& ImportParameter::GetRouter() const
{
  return router;
}

bool ImportParameter::GetStrictAreas() const
{
  return strictAreas;
}

bool ImportParameter::GetSortObjects() const
{
  return sortObjects;
}

size_t ImportParameter::GetSortBlockSize() const
{
  return sortBlockSize;
}

size_t ImportParameter::GetSortTileMag() const
{
  return sortTileMag;
}

size_t ImportParameter::GetProcessingQueueSize() const
{
  return processingQueueSize;
}

size_t ImportParameter::GetNumericIndexPageSize() const
{
  return numericIndexPageSize;
}

size_t ImportParameter::GetRawCoordBlockSize() const
{
  return rawCoordBlockSize;
}

bool ImportParameter::GetRawNodeDataMemoryMaped() const
{
  return rawNodeDataMemoryMaped;
}

bool ImportParameter::GetRawWayIndexMemoryMaped() const
{
  return rawWayIndexMemoryMaped;
}

size_t ImportParameter::GetRawWayIndexCacheSize() const
{
  return rawWayIndexCacheSize;
}

bool ImportParameter::GetRawWayDataMemoryMaped() const
{
  return rawWayDataMemoryMaped;
}

size_t ImportParameter::GetRawWayBlockSize() const
{
  return rawWayBlockSize;
}

bool ImportParameter::GetCoordDataMemoryMaped() const
{
  return coordDataMemoryMaped;
}

size_t ImportParameter::GetCoordIndexCacheSize() const
{
  return coordIndexCacheSize;
}

size_t ImportParameter::GetCoordBlockSize() const
{
  return coordBlockSize;
}

size_t ImportParameter::GetRelMaxWays() const
{
  return relMaxWays;
}

size_t ImportParameter::GetRelMaxCoords() const
{
  return relMaxCoords;
}

size_t ImportParameter::GetAreaDataCacheSize() const
{
  return areaDataCacheSize;
}

bool ImportParameter::GetAreaDataMemoryMaped() const
{
  return areaDataMemoryMaped;
}

size_t ImportParameter::GetWayDataCacheSize() const
{
  return wayDataCacheSize;
}

bool ImportParameter::GetWayDataMemoryMaped() const
{
  return wayDataMemoryMaped;
}

MagnificationLevel ImportParameter::GetAreaNodeGridMag() const
{
  return areaNodeGridMag;
}

uint16_t ImportParameter::GetAreaNodeSimpleListLimit() const
{
  return areaNodeSimpleListLimit;
}

uint16_t ImportParameter::GetAreaNodeTileListLimit() const
{
  return areaNodeTileListLimit;
}

uint16_t ImportParameter::GetAreaNodeTileListCoordLimit() const
{
  return areaNodeTileListCoordLimit;
}

MagnificationLevel ImportParameter::GetAreaNodeBitmapMaxMag() const
{
  return areaNodeBitmapMaxMag;
}

uint16_t ImportParameter::GetAreaNodeBitmapLimit() const
{
  return areaNodeBitmapLimit;
}

MagnificationLevel ImportParameter::GetAreaWayIndexMinMag() const
{
  return areaWayIndexMinMag;
}

MagnificationLevel ImportParameter::GetAreaWayIndexMaxMag() const
{
  return areaWayIndexMaxMag;
}

MagnificationLevel ImportParameter::GetAreaRouteIndexMinMag() const
{
  return areaRouteIndexMinMag;
}

MagnificationLevel ImportParameter::GetAreaRouteIndexMaxMag() const
{
  return areaRouteIndexMaxMag;
}

size_t ImportParameter::GetAreaAreaIndexMaxMag() const
{
  return areaAreaIndexMaxMag;
}

uint32_t ImportParameter::GetWaterIndexMinMag() const
{
  return waterIndexMinMag;
}

uint32_t ImportParameter::GetWaterIndexMaxMag() const
{
  return waterIndexMaxMag;
}

size_t ImportParameter::GetOptimizationMaxWayCount() const
{
  return optimizationMaxWayCount;
}

MagnificationLevel ImportParameter::GetOptimizationMaxMag() const
{
  return optimizationMaxMag;
}

MagnificationLevel ImportParameter::GetOptimizationMinMag() const
{
  return optimizationMinMag;
}

size_t ImportParameter::GetOptimizationCellSizeAverage() const
{
  return optimizationCellSizeAverage;
}

size_t ImportParameter::GetOptimizationCellSizeMax() const
{
  return optimizationCellSizeMax;
}

TransPolygon::OptimizeMethod ImportParameter::GetOptimizationWayMethod() const
{
  return optimizationWayMethod;
}

size_t ImportParameter::GetRouteNodeBlockSize() const
{
  return routeNodeBlockSize;
}

uint32_t ImportParameter::GetRouteNodeTileMag() const
{
  return routeNodeTileMag;
}

ImportParameter::AssumeLandStrategy ImportParameter::GetAssumeLand() const
{
  return assumeLand;
}

OSMId ImportParameter::GetFirstFreeOSMId() const
{
  return firstFreeOSMId;
}

const std::vector<std::string>& ImportParameter::GetLangOrder() const
{
  return this->langOrder;
}

const std::vector<std::string>& ImportParameter::GetAltLangOrder() const
{
  return this->altLangOrder;
}

size_t ImportParameter::GetMaxAdminLevel() const
{
  return maxAdminLevel;
}

void ImportParameter::SetMapfiles(const std::list<std::string>& mapfiles)
{
  this->mapfiles=mapfiles;
}

void ImportParameter::SetTypefile(const std::string& typefile)
{
  this->typefile=typefile;
}

void ImportParameter::SetDestinationDirectory(const std::string& destinationDirectory)
{
  this->destinationDirectory=destinationDirectory;
}

void ImportParameter::SetBoundingPolygonFile(const std::string& boundingPolygonFile)
{
  this->boundingPolygonFile=boundingPolygonFile;
}

void ImportParameter::SetErrorReporter(const ImportErrorReporterRef& errorReporter)
{
  this->errorReporter=errorReporter;
}

void ImportParameter::SetStartStep(size_t startStep)
{
  this->startStep=startStep;
  this->endStep=defaultEndStep;
}

void ImportParameter::SetSteps(size_t startStep, size_t endStep)
{
  this->startStep=startStep;
  this->endStep=endStep;
}

void ImportParameter::SetEco(bool eco)
{
  this->eco=eco;
}

void ImportParameter::ClearRouter()
{
  router.clear();
}

void ImportParameter::AddRouter(const Router& router)
{
  this->router.push_back(router);
}

void ImportParameter::SetStrictAreas(bool strictAreas)
{
  this->strictAreas=strictAreas;
}

void ImportParameter::SetSortObjects(bool renumberIds)
{
  this->sortObjects=renumberIds;
}

void ImportParameter::SetSortBlockSize(size_t sortBlockSize)
{
  this->sortBlockSize=sortBlockSize;
}

void ImportParameter::SetSortTileMag(size_t sortTileMag)
{
  this->sortTileMag=sortTileMag;
}

void ImportParameter::SetProcessingQueueSize(size_t processingQueueSize)
{
  this->processingQueueSize=processingQueueSize;
}

void ImportParameter::SetNumericIndexPageSize(size_t numericIndexPageSize)
{
  this->numericIndexPageSize=numericIndexPageSize;
}

void ImportParameter::SetRawCoordBlockSize(size_t blockSize)
{
  this->rawCoordBlockSize=blockSize;
}

void ImportParameter::SetRawNodeDataMemoryMaped(bool memoryMaped)
{
  this->rawNodeDataMemoryMaped=memoryMaped;
}

void ImportParameter::SetRawWayIndexMemoryMaped(bool memoryMaped)
{
  this->rawWayIndexMemoryMaped=memoryMaped;
}

void ImportParameter::SetRawWayDataMemoryMaped(bool memoryMaped)
{
  this->rawWayDataMemoryMaped=memoryMaped;
}

void ImportParameter::SetRawWayIndexCacheSize(size_t wayIndexCacheSize)
{
  this->rawWayIndexCacheSize=wayIndexCacheSize;
}

void ImportParameter::SetRawWayBlockSize(size_t blockSize)
{
  this->rawWayBlockSize=blockSize;
}

void ImportParameter::SetCoordDataMemoryMaped(bool memoryMaped)
{
  this->coordDataMemoryMaped=memoryMaped;
}

void ImportParameter::SetCoordIndexCacheSize(size_t coordIndexCacheSize)
{
  this->coordIndexCacheSize=coordIndexCacheSize;
}

void ImportParameter::SetCoordBlockSize(size_t coordBlockSize)
{
  this->coordBlockSize=coordBlockSize;
}

void ImportParameter::SetRelMaxWays(size_t relMaxWays)
{
  this->relMaxWays=relMaxWays;
}

void ImportParameter::SetRelMaxCoords(size_t relMaxCoords)
{
  this->relMaxCoords=relMaxCoords;
}

void ImportParameter::SetAreaDataMemoryMaped(bool memoryMaped)
{
  this->areaDataMemoryMaped=memoryMaped;
}

void ImportParameter::SetAreaDataCacheSize(size_t areaDataCacheSize)
{
  this->areaDataCacheSize=areaDataCacheSize;
}

void ImportParameter::SetWayDataMemoryMaped(bool memoryMaped)
{
  this->wayDataMemoryMaped=memoryMaped;
}

void ImportParameter::SetWayDataCacheSize(size_t wayDataCacheSize)
{
  this->wayDataCacheSize=wayDataCacheSize;
}

void ImportParameter::SetAreaAreaIndexMaxMag(size_t areaAreaIndexMaxMag)
{
  this->areaAreaIndexMaxMag=areaAreaIndexMaxMag;
}

void ImportParameter::SetAreaNodeGridMag(MagnificationLevel areaNodeGridMag)
{
  this->areaNodeGridMag=areaNodeGridMag;
}

void ImportParameter::SetAreaNodeSimpleListLimit(uint16_t areaNodeSimpleListLimit)
{
  this->areaNodeSimpleListLimit=areaNodeSimpleListLimit;
}

void ImportParameter::SetAreaNodeTileListLimit(uint16_t areaNodeTileListLimit)
{
  this->areaNodeTileListLimit=areaNodeTileListLimit;
}

void ImportParameter::SetAreaNodeTileListCoordLimit(uint16_t areaNodeTileListCoordLimit)
{
  this->areaNodeTileListCoordLimit=areaNodeTileListCoordLimit;
}

void ImportParameter::SetAreaNodeBitmapMaxMag(const MagnificationLevel& areaNodeBitmapMaxMag)
{
  this->areaNodeBitmapMaxMag=areaNodeBitmapMaxMag;
}

void ImportParameter::SetAreaNodeBitmapLimit(uint16_t areaNodeBitmapLimit)
{
  this->areaNodeBitmapLimit=areaNodeBitmapLimit;
}

void ImportParameter::SetTextIndexVariant(TextIndexVariant textIndexVariant)
{
  this->textIndexVariant=textIndexVariant;
}

ImportParameter::TextIndexVariant ImportParameter::GetTextIndexVariant() const
{
  return textIndexVariant;
}

void ImportParameter::SetAreaWayIndexMinMag(MagnificationLevel areaWayMinMag)
{
  this->areaWayIndexMinMag=areaWayMinMag;
}

void ImportParameter::SetAreaWayIndexMaxMag( MagnificationLevel areaWayIndexMaxMag)
{
  this->areaWayIndexMaxMag=areaWayIndexMaxMag;
}

void ImportParameter::SetAreaRouteIndexMinMag(MagnificationLevel areaRouteIndexMinMag)
{
  this->areaRouteIndexMinMag=areaRouteIndexMinMag;
}

void ImportParameter::SetAreaRouteIndexMaxMag(MagnificationLevel areaRouteIndexMaxMag)
{
  this->areaRouteIndexMaxMag=areaRouteIndexMaxMag;
}

void ImportParameter::SetWaterIndexMinMag(uint32_t waterIndexMinMag)
{
  this->waterIndexMinMag=waterIndexMinMag;
}

void ImportParameter::SetWaterIndexMaxMag(uint32_t waterIndexMaxMag)
{
  this->waterIndexMaxMag=waterIndexMaxMag;
}

void ImportParameter::SetOptimizationMaxWayCount(size_t optimizationMaxWayCount)
{
  this->optimizationMaxWayCount=optimizationMaxWayCount;
}

void ImportParameter::SetOptimizationMaxMag(MagnificationLevel optimizationMaxMag)
{
  this->optimizationMaxMag=optimizationMaxMag;
}

void ImportParameter::SetOptimizationMinMag(MagnificationLevel optimizationMinMag)
{
  this->optimizationMinMag=optimizationMinMag;
}

void ImportParameter::SetOptimizationCellSizeAverage(size_t optimizationCellSizeAverage)
{
  this->optimizationCellSizeAverage=optimizationCellSizeAverage;
}


void ImportParameter::SetOptimizationCellSizeMax(size_t optimizationCellSizeMax)
{
  this->optimizationCellSizeMax=optimizationCellSizeMax;
}


void ImportParameter::SetOptimizationWayMethod(TransPolygon::OptimizeMethod optimizationWayMethod)
{
  this->optimizationWayMethod=optimizationWayMethod;
}

void ImportParameter::SetRouteNodeBlockSize(size_t blockSize)
{
  this->routeNodeBlockSize=blockSize;
}

void ImportParameter::SetRouteNodeTileMag(uint32_t routeNodeTileMag)
{
  this->routeNodeTileMag=routeNodeTileMag;
}

void ImportParameter::SetAssumeLand(AssumeLandStrategy assumeLand)
{
  this->assumeLand=assumeLand;
}

void ImportParameter::SetLangOrder(const std::vector<std::string>& langOrder)
{
  this->langOrder = langOrder;
}

void ImportParameter::SetAltLangOrder(const std::vector<std::string>& altLangOrder)
{
  this->altLangOrder = altLangOrder;
}

void ImportParameter::SetMaxAdminLevel(size_t maxAdminLevel)
{
  this->maxAdminLevel=maxAdminLevel;
}

void ImportParameter::SetFirstFreeOSMId(OSMId id)
{
  firstFreeOSMId=id;
}

void ImportParameter::SetFillWaterArea(size_t fillWaterArea)
{
  this->fillWaterArea=fillWaterArea;
}

size_t ImportParameter::GetFillWaterArea() const
{
  return fillWaterArea;
}

void ImportParameter::SetPreprocessorFactory(const PreprocessorFactoryRef& factory)
{
  this->preprocessorFactory=factory;
}

std::unique_ptr<Preprocessor> ImportParameter::GetPreprocessor(const std::string& filename,
                                                               PreprocessorCallback& callback) const
{
  if (preprocessorFactory) {
    return preprocessorFactory->GetProcessor(filename,
                                             callback);
  }
  return nullptr;
}

}
