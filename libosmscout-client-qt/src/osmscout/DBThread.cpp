/*
  OSMScout - a Qt backend for libosmscout and libosmscout-map
  Copyright (C) 2010  Tim Teulings
  Copyright (C) 2016  Lukáš Karas

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

#include <osmscout/MapService.h>

#include <osmscout/DBThread.h>
#include <osmscout/TiledDBThread.h>
#include <osmscout/PlaneDBThread.h>

QBreaker::QBreaker()
  : osmscout::Breaker(),
    aborted(false)
{
}

bool QBreaker::Break()
{
  QMutexLocker locker(&mutex);
  aborted=true;

  return true;
}

bool QBreaker::IsAborted() const
{
  QMutexLocker locker(&mutex);

  return aborted;
}

void QBreaker::Reset()
{
  QMutexLocker locker(&mutex);

  aborted=false;
}

StyleError::StyleError(QString msg){
    QRegExp rx("(\\d+),(\\d+) (Symbol|Error|Warning|Exception):(.*)");
    if(rx.exactMatch(msg)){
        line = rx.cap(1).toInt();
        column = rx.cap(2).toInt();
        if(rx.cap(3) == "Symbol"){
            type = Symbol;
        } else if(rx.cap(3) == "Error"){
            type = Error;
        } else if(rx.cap(3) == "Warning"){
            type = Warning;
        } else {
            type = Exception;
        }
        text = rx.cap(4);
    }
}

QString StyleError::GetTypeName() const
{
    switch(type){
    case Symbol:
        return QString("symbol");
        break;
    case Error:
        return QString("error");
        break;
    case Warning:
        return QString("warning");
        break;
    case Exception:
        return QString("exception");
        break;
    default:
      return QString("???");
    }
}


DBThread::DBThread(QStringList databaseLookupDirs, 
                   QString stylesheetFilename, 
                   QString iconDirectory)
  : databaseLookupDirs(databaseLookupDirs), 
    mapDpi(-1),
    physicalDpi(-1),
    stylesheetFilename(stylesheetFilename),
    iconDirectory(iconDirectory),
    daylight(true),
    renderSea(true)
{
  QScreen *srn=QGuiApplication::screens().at(0);

  physicalDpi = (double)srn->physicalDotsPerInch();
  qDebug() << "Reported screen DPI: " << physicalDpi;
  mapDpi = Settings::GetInstance()->GetMapDPI();
  qDebug() << "Map DPI override: " << mapDpi;

  renderSea = Settings::GetInstance()->GetRenderSea();
  
  connect(Settings::GetInstance(), SIGNAL(MapDPIChange(double)),
          this, SLOT(onMapDPIChange(double)),
          Qt::QueuedConnection);
  
}

DBThread::~DBThread()
{
  osmscout::log.Debug() << "DBThread::~TiledDBThread()";

  for (auto db:databases){
    db->mapService->DeregisterTileStateCallback(db->callbackId);
  }
}


bool DBThread::AssureRouter(osmscout::Vehicle vehicle)
{
  for (auto db:databases){
    if (!db->AssureRouter(vehicle, routerParameter)){
      return false;
    }
  }  
  return true;
}

bool DBThread::isInitialized(){
  QMutexLocker locker(&mutex);
  for (auto db:databases){
    if (!db->database->IsOpen()){
      return false;
    }
  }
  return true;
}

double DBThread::GetMapDpi() const
{
    return mapDpi;
}

double DBThread::GetPhysicalDpi() const
{
    return physicalDpi;
}

const DatabaseLoadedResponse DBThread::loadedResponse() const {
  QMutexLocker locker(&mutex);
  DatabaseLoadedResponse response;
  for (auto db:databases){
    if (response.boundingBox.IsValid()){
      osmscout::GeoBox boundingBox;
      db->database->GetBoundingBox(boundingBox);
      response.boundingBox.Include(boundingBox);
    }else{
      db->database->GetBoundingBox(response.boundingBox);
    }
  }
  return response;
}

void DBThread::TileStateCallback(const osmscout::TileRef& changedTile)
{
  
}

bool DBThread::InitializeDatabases(osmscout::GeoBox& boundingBox)
{  
  QMutexLocker locker(&mutex);
  qDebug() << "Initialize";  
  
  //stylesheetFilename = resourceDirectory + QDir::separator() + "map-styles" + QDir::separator() + "standard.oss";
  // TODO: remove last separator, it should be added by renderer (MapPainter*.cpp)
  //iconDirectory = resourceDirectory + QDir::separator() + "map-icons" + QDir::separator(); // TODO: load icon set for given stylesheet
  if (!iconDirectory.endsWith(QDir::separator())){
    iconDirectory = iconDirectory + QDir::separator();
  }

  for (QString lookupDir:databaseLookupDirs){
    QDirIterator dirIt(lookupDir, QDirIterator::Subdirectories | QDirIterator::FollowSymlinks);
    while (dirIt.hasNext()) {
      dirIt.next();
      QFileInfo fInfo(dirIt.filePath());
      if (fInfo.isFile() && fInfo.fileName() == osmscout::TypeConfig::FILE_TYPES_DAT){
        qDebug() << "found database: " << fInfo.dir().absolutePath();

        osmscout::DatabaseRef database = std::make_shared<osmscout::Database>(databaseParameter);
        osmscout::StyleConfigRef styleConfig;
        if (database->Open(fInfo.dir().absolutePath().toLocal8Bit().data())) {
          osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

          if (typeConfig) {
            styleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

            if (!styleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
              qDebug() << "Cannot load style sheet!";
              styleConfig=NULL;
            }
          }
          else {
            qDebug() << "TypeConfig invalid!";
            styleConfig=NULL;
          }
        }
        else {
          qWarning() << "Cannot open database!";
          continue;
        }

        if (!database->GetBoundingBox(boundingBox)) {
          qWarning() << "Cannot read initial bounding box";
          database->Close();
          continue;
        }
        
        osmscout::MapService::TileStateCallback callback=[this](const osmscout::TileRef& tile) {TileStateCallback(tile);};
        osmscout::MapServiceRef mapService = std::make_shared<osmscout::MapService>(database);
        osmscout::MapService::CallbackId callbackId=mapService->RegisterTileStateCallback(callback);
        
        databases << std::make_shared<DBInstance>(fInfo.dir().absolutePath(), 
                                                  database, 
                                                  std::make_shared<osmscout::LocationService>(database),
                                                  mapService,
                                                  callbackId,
                                                  std::make_shared<QBreaker>(),
                                                  styleConfig);
      }
    }
  }  

  emit stylesheetFilenameChanged();
  return true;
}

void DBThread::Finalize()
{
  qDebug() << "Finalize";
  //FreeMaps();
  for (auto db:databases){
    if (db->router && db->router->IsOpen()) {
      db->router->Close();
    }

    if (db->database->IsOpen()) {
      db->database->Close();
    }
  }
}

void DBThread::CancelCurrentDataLoading()
{
  for (auto db:databases){
    db->dataLoadingBreaker->Break();
  }
}

void DBThread::ToggleDaylight()
{
  {
    QMutexLocker locker(&mutex);

    if (!isInitialized()) {
        return;
    }
    qDebug() << "Toggling daylight from " << daylight << " to " << !daylight << "...";
    daylight=!daylight;
    stylesheetFlags["daylight"] = daylight;
  }

  ReloadStyle();

  qDebug() << "Toggling daylight done.";
}

void DBThread::ReloadStyle(const QString &suffix)
{
  qDebug() << "Reloading style...";
  LoadStyle(stylesheetFilename, stylesheetFlags,suffix);
  qDebug() << "Reloading style done.";
}

void DBThread::LoadStyle(QString stylesheetFilename,
                         std::unordered_map<std::string,bool> stylesheetFlags,
                         const QString &suffix)
{
  QMutexLocker locker(&mutex);

  this->stylesheetFilename = stylesheetFilename;
  this->stylesheetFlags = stylesheetFlags;
  
  bool prevErrs = !styleErrors.isEmpty();
  styleErrors.clear();
  for (auto db: databases){
    db->LoadStyle(stylesheetFilename+suffix, stylesheetFlags, styleErrors);
  }
  if (prevErrs || (!styleErrors.isEmpty())){
    qWarning()<<"Failed to load stylesheet"<<(stylesheetFilename+suffix);
    emit styleErrorsChanged();
  }
  emit stylesheetFilenameChanged();
}

bool DBInstance::LoadStyle(QString stylesheetFilename,
                           std::unordered_map<std::string,bool> stylesheetFlags, 
                           QList<StyleError> &errors)
{


  if (!database->IsOpen()) {
    return false;
  }

  osmscout::TypeConfigRef typeConfig=database->GetTypeConfig();

  if (!typeConfig) {
    return false;
  }

  mapService->FlushTileCache();
  osmscout::StyleConfigRef newStyleConfig=std::make_shared<osmscout::StyleConfig>(typeConfig);

  for (auto flag: stylesheetFlags){
    newStyleConfig->AddFlag(flag.first, flag.second);
  }

  if (newStyleConfig->Load(stylesheetFilename.toLocal8Bit().data())) {
    // Tear down
    if (painter!=NULL){
      delete painter;
      painter=NULL;
    }

    // Recreate
    styleConfig=newStyleConfig;
    painter=new osmscout::MapPainterQt(styleConfig);
  }else{
    std::list<std::string> errorsStrings = newStyleConfig->GetErrors();
    for(std::list<std::string>::iterator it = errorsStrings.begin(); it != errorsStrings.end(); it++){
      StyleError err(QString::fromStdString(*it));
      qWarning() << "Style error:" << err.GetDescription();
      errors.append(err);
    }
    styleConfig=NULL;
    return false;
  }
  
  return true;
}


bool DBInstance::AssureRouter(osmscout::Vehicle /*vehicle*/, 
                              osmscout::RouterParameter routerParameter)
{
  if (!database->IsOpen()) {
    return false;
  }

  if (!router/* ||
      (router && router->GetVehicle()!=vehicle)*/) {
    if (router) {
      if (router->IsOpen()) {
        router->Close();
      }
      router=NULL;
    }

    router=std::make_shared<osmscout::RoutingService>(database,
                                                      routerParameter,
                                                      osmscout::RoutingService::DEFAULT_FILENAME_BASE);

    if (!router->Open()) {
      return false;
    }
  }

  return true;
}

bool DBThread::SearchForLocations(const std::string& searchPattern,
                                  size_t limit,
                                  osmscout::LocationSearchResult& result) const
{
  QMutexLocker locker(&mutex);

  osmscout::LocationSearch search;

  search.limit=limit;
  for (auto db:databases){

    if (!db->locationService->InitializeLocationSearchEntries(searchPattern, search)) {
        return false;
    }

    if (!db->locationService->SearchForLocations(search, result)){
      return false;
    }
  }
  return true;
}

bool DBThread::CalculateRoute(osmscout::Vehicle vehicle,
                              const osmscout::RoutingProfile& routingProfile,
                              const osmscout::ObjectFileRef& startObject,
                              size_t startNodeIndex,
                              const osmscout::ObjectFileRef targetObject,
                              size_t targetNodeIndex,
                              osmscout::RouteData& route)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->CalculateRoute(routingProfile,
                                startObject,
                                startNodeIndex,
                                targetObject,
                                targetNodeIndex,
                                route);
  */
}

bool DBThread::TransformRouteDataToRouteDescription(osmscout::Vehicle vehicle,
                                                    const osmscout::RoutingProfile& routingProfile,
                                                    const osmscout::RouteData& data,
                                                    osmscout::RouteDescription& description,
                                                    const std::string& start,
                                                    const std::string& target)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  if (!router->TransformRouteDataToRouteDescription(data,description)) {
    return false;
  }

  osmscout::TypeConfigRef typeConfig=router->GetTypeConfig();

  std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors;

  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>(start));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>(target));
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>());
  postprocessors.push_back(std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>());

  osmscout::RoutePostprocessor::InstructionPostprocessorRef instructionProcessor=std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>();

  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_motorway_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_trunk"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_trunk"));
  instructionProcessor->AddMotorwayLinkType(typeConfig->GetTypeInfo("highway_trunk_link"));
  instructionProcessor->AddMotorwayType(typeConfig->GetTypeInfo("highway_motorway_primary"));
  postprocessors.push_back(instructionProcessor);

  if (!routePostprocessor.PostprocessRouteDescription(description,
                                                      routingProfile,
                                                      *databases,
                                                      postprocessors)) {
    return false;
  }

  return true;
   */
}

bool DBThread::TransformRouteDataToWay(osmscout::Vehicle vehicle,
                                       const osmscout::RouteData& data,
                                       osmscout::Way& way)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  return router->TransformRouteDataToWay(data,way);
   */
}


void DBThread::ClearRoute()
{
  emit Redraw();
}

void DBThread::AddRoute(const osmscout::Way& way)
{
  emit Redraw();
}

bool DBThread::GetClosestRoutableNode(const osmscout::ObjectFileRef& refObject,
                                      const osmscout::Vehicle& vehicle,
                                      double radius,
                                      osmscout::ObjectFileRef& object,
                                      size_t& nodeIndex)
{
  return false; // TODO: implement multi database routing
  /*
  QMutexLocker locker(&mutex);

  if (!AssureRouter(vehicle)) {
    return false;
  }

  object.Invalidate();

  if (refObject.GetType()==osmscout::refNode) {
    osmscout::NodeRef node;

    if (!databases->GetNodeByOffset(refObject.GetFileOffset(),
                                   node)) {
      return false;
    }

    return router->GetClosestRoutableNode(node->GetCoords().GetLat(),
                                          node->GetCoords().GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else if (refObject.GetType()==osmscout::refArea) {
    osmscout::AreaRef area;

    if (!databases->GetAreaByOffset(refObject.GetFileOffset(),
                                   area)) {
      return false;
    }

    osmscout::GeoCoord center;

    area->GetCenter(center);

    return router->GetClosestRoutableNode(center.GetLat(),
                                          center.GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else if (refObject.GetType()==osmscout::refWay) {
    osmscout::WayRef way;

    if (!databases->GetWayByOffset(refObject.GetFileOffset(),
                                  way)) {
      return false;
    }

    return router->GetClosestRoutableNode(way->GetNodes()[0].GetLat(),
                                          way->GetNodes()[0].GetLon(),
                                          vehicle,
                                          radius,
                                          object,
                                          nodeIndex);
  }
  else {
    return true;
  }
    */
}

QStringList DBThread::BuildAdminRegionList(const osmscout::LocationServiceRef& locationService,
                                           const osmscout::AdminRegionRef& adminRegion,
                                           std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap)
{
  if (!adminRegion){
    return QStringList();
  }

  QStringList list;
  locationService->ResolveAdminRegionHierachie(adminRegion, regionMap);
  QString name = QString::fromStdString(adminRegion->name);
  list << name;
  QString last = name;
  osmscout::FileOffset parentOffset = adminRegion->parentRegionOffset;
  while (parentOffset != 0){
    osmscout::AdminRegionRef region = regionMap[parentOffset];
    name = QString::fromStdString(region->name);
    if (last != name){ // skip duplicates in admin region names
      list << name;
    }
    last = name;
    parentOffset = region->parentRegionOffset;
  }
  return list;
}

void DBThread::requestLocationDescription(const osmscout::GeoCoord location)
{
  if (!isInitialized()){
      return; // ignore request if db is not initialized
  }
  QMutexLocker locker(&mutex);
    
  osmscout::LocationDescription description;
  int count = 0;
  for (auto db:databases){
    osmscout::GeoBox dbBox;
    if (!db->database->GetBoundingBox(dbBox)){
      continue;
    }
    if (!dbBox.Includes(location)){
      continue;
    }
    
    std::map<osmscout::FileOffset,osmscout::AdminRegionRef> regionMap;
    if (!db->locationService->DescribeLocationByAddress(location, description)) {
      std::cerr << "Error during generation of location description" << std::endl;
      continue;
    }

    if (description.GetAtAddressDescription()){
      count++;
      
      auto place = description.GetAtAddressDescription()->GetPlace();
      emit locationDescription(location, db->path, description, 
                               BuildAdminRegionList(db->locationService, place.GetAdminRegion(), regionMap));
    }
    
    if (!db->locationService->DescribeLocationByPOI(location, description)) {
      std::cerr << "Error during generation of location description" << std::endl;
      continue;
    }

    if (description.GetAtPOIDescription()){
      count++;

      auto place = description.GetAtPOIDescription()->GetPlace();
      emit locationDescription(location, db->path, description, 
                               BuildAdminRegionList(db->locationService, place.GetAdminRegion(), regionMap));
    }
  }
  
  emit locationDescriptionFinished(location);
}

void DBThread::onMapDPIChange(double dpi)
{
    QMutexLocker locker(&mutex);
    mapDpi = dpi;
    emit Redraw();
}

void DBThread::onRenderSeaChanged(bool b)
{
    {
        QMutexLocker threadLocker(&mutex);
        renderSea = b;
    }
    emit Redraw();
}


/*
osmscout::TypeConfigRef DBThread::GetTypeConfig() const
{
  return databases->GetTypeConfig();
}

bool DBThread::GetNodeByOffset(osmscout::FileOffset offset,
                               osmscout::NodeRef& node) const
{
  return databases->GetNodeByOffset(offset,node);
}

bool DBThread::GetAreaByOffset(osmscout::FileOffset offset,
                               osmscout::AreaRef& area) const
{
  return databases->GetAreaByOffset(offset,area);
}

bool DBThread::GetWayByOffset(osmscout::FileOffset offset,
                              osmscout::WayRef& way) const
{
  return databases->GetWayByOffset(offset,way);
}

bool DBThread::ResolveAdminRegionHierachie(const osmscout::AdminRegionRef& adminRegion,
                                           std::map<osmscout::FileOffset,osmscout::AdminRegionRef >& refs) const
{
  QMutexLocker locker(&mutex);

  return locationService->ResolveAdminRegionHierachie(adminRegion,
                                                      refs);
}
*/



static DBThread* dbThreadInstance=NULL;

bool DBThread::InitializeTiledInstance(QStringList databaseLookupDirectory, 
                                       QString stylesheetFilename, 
                                       QString iconDirectory,
                                       QString tileCacheDirectory,
                                       size_t onlineTileCacheSize, 
                                       size_t offlineTileCacheSize)
{
  if (dbThreadInstance!=NULL) {
    return false;
  }

  dbThreadInstance=new TiledDBThread(databaseLookupDirectory, 
                                     stylesheetFilename, 
                                     iconDirectory,
                                     tileCacheDirectory,
                                     onlineTileCacheSize, 
                                     offlineTileCacheSize);

  return true;
}

bool DBThread::InitializePlaneInstance(QStringList databaseLookupDirectory, 
                                       QString stylesheetFilename, 
                                       QString iconDirectory)
{
  if (dbThreadInstance!=NULL) {
    return false;
  }

  dbThreadInstance=new PlaneDBThread(databaseLookupDirectory, 
                                     stylesheetFilename, 
                                     iconDirectory);

  return true;
}



DBThread* DBThread::GetInstance()
{
  return dbThreadInstance;
}

void DBThread::FreeInstance()
{
  delete dbThreadInstance;

  dbThreadInstance=NULL;
}
