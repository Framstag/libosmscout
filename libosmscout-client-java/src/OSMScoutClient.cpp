#include <osmscoutclientjava/ClientJavaImportExport.h>

#include <osmscoutclient/MapDownloadService.h>

#include <optional>
#include <osmscoutclient/json/json.hpp>

#include <atomic>
#include <algorithm>
#include <cstdarg>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include <osmscout/lib/CoreFeatures.h>

#include <osmscout/async/Breaker.h>
#include <osmscout/async/CancelableFuture.h>
#include <osmscoutclient/DBInstance.h>
#include <osmscoutclient/DBThread.h>
#include <osmscoutclient/MapManager.h>
#include <osmscoutclient/FavoriteLocationService.h>
#include <osmscoutclient/Settings.h>

#include <osmscoutmap/MapData.h>
#include <osmscoutmap/MapPainter.h>
#include <osmscoutmap/MapParameter.h>
#include <osmscoutmap/MapService.h>

#ifdef HAVE_MAP_CAIRO
#include <osmscoutmapcairo/MapPainterCairo.h>
#endif

#include <osmscout/projection/MercatorProjection.h>

#include <osmscout/location/LocationService.h>

#include <osmscout/util/StringMatcher.h>

#include <osmscout/db/Database.h>
#ifdef OSMSCOUT_HAVE_LIB_MARISA
#include <osmscout/db/TextSearchIndex.h>
#endif
#include <osmscout/feature/NameFeature.h>
#include <osmscout/feature/LayerFeature.h>
#include <osmscout/feature/OperatorFeature.h>
#include <osmscout/feature/RefFeature.h>

#include <osmscout/poi/POIService.h>

#include <osmscout/description/DescriptionService.h>
#include <osmscout/location/LocationDescriptionService.h>

#include <osmscout/log/Logger.h>
#include <osmscout/routing/MultiDBRoutingService.h>
#include <osmscout/routing/RouteDescriptionPostprocessor.h>
#include <osmscout/routing/RoutePostprocessor.h>
#include <osmscout/routing/RoutingProfile.h>
#include <osmscout/routing/RoutingService.h>

#include <osmscout/navigation/Navigation.h>
#include <osmscout/navigation/Engine.h>

// A basemap database may also appear in the regular databases list (e.g. when
// the basemap directory is located inside the maps lookup directory). In that
// case IsBasemap() is false, so detect it via its basemap-specific types.
bool IsBasemapDatabase(const osmscout::DBInstanceRef& db)
{
  auto database = db->GetDatabase();
  if (!database) {
    return false;
  }
  if (database->IsBasemap()) {
    return true;
  }
  auto typeConfig = database->GetTypeConfig();
  if (!typeConfig) {
    return false;
  }
  return typeConfig->GetTypeInfo("basemap_boundary_country") != nullptr;
}

// Search state shared across JNI search calls so that a new query or an
// explicit cancel aborts the currently running search (matching OSMScout2's
// Breaker support).
namespace {
  std::mutex           g_searchMutex;
  osmscout::BreakerRef g_currentBreaker;

#ifdef OSMSCOUT_HAVE_LIB_MARISA
  // A fully resolved free-text search hit (name, coordinates, type) ready to
  // be serialized into a Java LocationEntry.
  struct FreeTextEntry {
    std::string label;
    std::string objectType;
    std::string objectTypeName;
    double      lat{0.0};
    double      lon{0.0};
    long long   objectFileOffset{0};
    std::string refType;
  };

  // Look up a free-text hit in the database and fill a FreeTextEntry.
  // Returns false if the object cannot be loaded.
  bool BuildFreeTextEntry(const osmscout::DBInstanceRef& db,
                          const osmscout::ObjectFileRef& ref,
                          const std::string& searchKey,
                          FreeTextEntry& entry)
  {
    auto database = db->GetDatabase();
    if (!database) {
      return false;
    }
    auto typeConfig = database->GetTypeConfig();
    if (!typeConfig) {
      return false;
    }

    osmscout::NameFeatureValueReader nameReader(*typeConfig);
    entry.objectFileOffset = static_cast<long long>(ref.GetFileOffset());

    if (ref.GetType() == osmscout::RefType::refNode) {
      osmscout::NodeRef node;
      if (!database->GetNodeByOffset(ref.GetFileOffset(), node)) {
        return false;
      }
      entry.lat = node->GetCoords().GetLat();
      entry.lon = node->GetCoords().GetLon();
      entry.objectType = node->GetType()->GetName();
      if (auto val = nameReader.GetValue(node->GetFeatureValueBuffer())) {
        entry.label = val->GetName();
      }
      entry.refType = "node";
    } else if (ref.GetType() == osmscout::RefType::refArea) {
      osmscout::AreaRef area;
      if (!database->GetAreaByOffset(ref.GetFileOffset(), area)) {
        return false;
      }
      entry.lat = area->GetBoundingBox().GetCenter().GetLat();
      entry.lon = area->GetBoundingBox().GetCenter().GetLon();
      entry.objectType = area->GetType()->GetName();
      if (auto val = nameReader.GetValue(area->GetFeatureValueBuffer())) {
        entry.label = val->GetName();
      }
      entry.refType = "area";
    } else if (ref.GetType() == osmscout::RefType::refWay) {
      osmscout::WayRef way;
      if (!database->GetWayByOffset(ref.GetFileOffset(), way)) {
        return false;
      }
      entry.lat = way->GetBoundingBox().GetCenter().GetLat();
      entry.lon = way->GetBoundingBox().GetCenter().GetLon();
      entry.objectType = way->GetType()->GetName();
      if (auto val = nameReader.GetValue(way->GetFeatureValueBuffer())) {
        entry.label = val->GetName();
      }
      entry.refType = "way";
    }

    if (entry.label.empty()) {
      entry.label = searchKey;
    }
    entry.objectTypeName = entry.objectType;
    return true;
  }
#endif

  // Parse a query like "51.5, 7.4" / "51.5 7.4" / "51.5;7.4" as a coordinate.
  // Returns false if the text is not a valid lat/lon pair.
  bool ParseCoordinate(const std::string& text, double& lat, double& lon)
  {
    static const std::regex re(R"(^\s*([-+]?\d+(?:\.\d+)?)\s*[,;\s]\s*([-+]?\d+(?:\.\d+)?)\s*$)");
    std::smatch m;
    if (!std::regex_match(text, m, re)) {
      return false;
    }
    try {
      lat = std::stod(m[1].str());
      lon = std::stod(m[2].str());
    } catch (const std::exception&) {
      return false;
    }
    return lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0;
  }
} // namespace
#include <osmscout/navigation/Agents.h>
#include <osmscout/navigation/DataAgent.h>
#include <osmscout/navigation/PositionAgent.h>
#include <osmscout/navigation/RouteStateAgent.h>
#include <osmscout/navigation/BearingAgent.h>
#include <osmscout/navigation/ArrivalEstimateAgent.h>
#include <osmscout/navigation/SpeedAgent.h>
#include <osmscout/navigation/LaneAgent.h>
#include <osmscout/navigation/VoiceInstructionAgent.h>
#include <osmscout/navigation/RouteInstructionAgent.h>

#ifdef HAVE_MAP_CAIRO
#include <cairo.h>
#endif

#include <jni.h>

#if __has_include(<osmscoutgpx/GPXFeatures.h>)
#include <osmscoutgpx/GPXFeatures.h>
#endif

#if defined(OSMSCOUT_GPX_HAVE_LIB_XML)
#include <osmscoutgpx/Import.h>
#include <osmscoutgpx/GpxFile.h>
#include <osmscoutgpx/Utils.h>
#include <osmscout/util/String.h>
#endif

// --------------------------------------------------------------------------
// InMemorySettingsStorage — lightweight SettingsStorage for JVM clients
// --------------------------------------------------------------------------

class InMemorySettingsStorage: public osmscout::SettingsStorage
{
private:
  std::map<std::string, std::string> storage;   //!< Key-value storage
  mutable std::mutex mutex;                      //!< Thread safety

public:
  void SetValue(const std::string& key, double d) override
  {
    std::scoped_lock lock(mutex);
    storage[key] = std::to_string(d);
  }

  void SetValue(const std::string& key, uint32_t i) override
  {
    std::scoped_lock lock(mutex);
    storage[key] = std::to_string(i);
  }

  void SetValue(const std::string& key, const std::string& str) override
  {
    std::scoped_lock lock(mutex);
    storage[key] = str;
  }

  void SetValue(const std::string& key, bool b) override
  {
    std::scoped_lock lock(mutex);
    storage[key] = b ? "true" : "false";
  }

  void SetValue(const std::string& key, std::vector<char> bytes) override
  {
    std::scoped_lock lock(mutex);
    storage[key] = std::string(bytes.data(), bytes.size());
  }

  double GetDouble(const std::string& key, double defaultValue) override
  {
    std::scoped_lock lock(mutex);
    auto it = storage.find(key);
    return it != storage.end() ? std::stod(it->second) : defaultValue;
  }

  uint32_t GetUInt(const std::string& key, uint32_t defaultValue) override
  {
    std::scoped_lock lock(mutex);
    auto it = storage.find(key);
    return it != storage.end() ? static_cast<uint32_t>(std::stoul(it->second)) : defaultValue;
  }

  std::string GetString(const std::string& key, const std::string& defaultValue) override
  {
    std::scoped_lock lock(mutex);
    auto it = storage.find(key);
    return it != storage.end() ? it->second : defaultValue;
  }

  bool GetBool(const std::string& key, bool defaultValue) override
  {
    std::scoped_lock lock(mutex);
    auto it = storage.find(key);
    if (it != storage.end()) {
      return it->second == "true";
    }
    return defaultValue;
  }

  std::vector<char> GetBytes(const std::string& key) override
  {
    std::scoped_lock lock(mutex);
    auto it = storage.find(key);
    if (it != storage.end()) {
      return std::vector<char>(it->second.begin(), it->second.end());
    }
    return {};
  }

  std::vector<std::string> Keys(const std::string& prefix) override
  {
    std::vector<std::string> result;
    std::scoped_lock lock(mutex);
    for (const auto &kv : storage) {
      if (kv.first.find(prefix) == 0) {
        result.push_back(kv.first);
      }
    }
    return result;
  }
};

// Forward declarations for navigation support
struct NavigationListenerMethods;
class JavaNavigationController;
using JavaNavigationControllerRef = std::shared_ptr<JavaNavigationController>;

// --------------------------------------------------------------------------
// ClientData — opaque C++ side data attached to each OSMScoutClient
// --------------------------------------------------------------------------

struct ClientData
{
  osmscout::SettingsRef settings;                    //!< Application settings
  osmscout::MapManagerRef mapManager;                //!< Map manager instance
  osmscout::DBThreadRef dbThread;                    //!< Database thread instance
  osmscout::DescriptionService descriptionService;   //!< Object description service
  osmscout::FavoriteLocationService *favService;     //!< Favorite location service (owned)
  osmscout::MapDownloadServiceRef mapDownloadService; //!< Map download service
  std::vector<std::filesystem::path> knownPaths;     //!< Known map paths

  // Routing state
  std::shared_ptr<std::thread> routingThread;        //!< Background routing thread
  osmscout::BreakerRef breaker;                      //!< Breaker for route cancellation
  std::mutex routingMutex;                           //!< Guards routing thread + breaker

  // Route descriptions retained for navigation
  std::mutex routeDescriptionMutex;
  long nextRouteHandle{1};
  std::map<long, osmscout::RouteDescriptionRef> routeDescriptions;
  std::map<JavaNavigationController *, JavaNavigationControllerRef> navigationControllers;
};

// Global singleton pointer (one active instance at a time, like OSMScoutQt)
static ClientData *activeClient = nullptr;
static std::mutex initMutex;

// --------------------------------------------------------------------------
// JNI method helpers
// --------------------------------------------------------------------------

static jfieldID getHandleField(JNIEnv *env, jobject obj)
{
  jclass cls = env->GetObjectClass(obj);
  return env->GetFieldID(cls, "nativeHandle", "J");
}

static ClientData *getClientData(JNIEnv *env, jobject obj)
{
  jfieldID field = getHandleField(env, obj);
  jlong handle = env->GetLongField(obj, field);
  return reinterpret_cast<ClientData *>(static_cast<intptr_t>(handle));
}

static void setClientData(JNIEnv *env, jobject obj, ClientData *data)
{
  jfieldID field = getHandleField(env, obj);
  env->SetLongField(obj, field, static_cast<jlong>(reinterpret_cast<intptr_t>(data)));
}

// --------------------------------------------------------------------------
// OSMScoutClientBuilder::build()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClientBuilder_build(JNIEnv *env, jobject self)
{
  std::scoped_lock lock(initMutex);

  if (activeClient != nullptr) {
    // Already initialised — return null (call close() first)
    return nullptr;
  }

  // --- Read builder fields ---
  jclass builderCls = env->GetObjectClass(self);

  jfieldID basemapField = env->GetFieldID(builderCls, "basemapLookupDirectory", "Ljava/lang/String;");
  jfieldID iconField = env->GetFieldID(builderCls, "iconDirectory", "Ljava/lang/String;");
  jfieldID dirsField = env->GetFieldID(builderCls, "mapLookupDirectories", "[Ljava/lang/String;");
  jfieldID dpiField = env->GetFieldID(builderCls, "physicalDpi", "D");
  jfieldID unitsField = env->GetFieldID(builderCls, "units", "Ljava/lang/String;");
  jfieldID styleDirField = env->GetFieldID(builderCls, "stylesheetDirectory", "Ljava/lang/String;");
  jfieldID customPoiField = env->GetFieldID(builderCls, "customPoiTypes", "[Ljava/lang/String;");
  jfieldID mapsDirField = env->GetFieldID(builderCls, "mapsDirectory", "Ljava/lang/String;");

  jstring basemapJStr = (jstring)env->GetObjectField(self, basemapField);
  jstring iconJStr = (jstring)env->GetObjectField(self, iconField);
  jobjectArray dirsArray = (jobjectArray)env->GetObjectField(self, dirsField);
  jdouble dpi = env->GetDoubleField(self, dpiField);
  jstring unitsJStr = (jstring)env->GetObjectField(self, unitsField);
  jstring styleDirJStr = (jstring)env->GetObjectField(self, styleDirField);
  jobjectArray customPoiArray = (jobjectArray)env->GetObjectField(self, customPoiField);

  const char *basemapCStr = basemapJStr ? env->GetStringUTFChars(basemapJStr, nullptr) : "";
  const char *iconCStr = iconJStr ? env->GetStringUTFChars(iconJStr, nullptr) : "";
  const char *unitsCStr = unitsJStr ? env->GetStringUTFChars(unitsJStr, nullptr) : "metrics";
  const char *styleDirCStr = styleDirJStr ? env->GetStringUTFChars(styleDirJStr, nullptr) : nullptr;

  std::string basemapDir(basemapCStr);
  std::string iconDir(iconCStr);
  std::string units(unitsCStr);

  if (basemapJStr) env->ReleaseStringUTFChars(basemapJStr, basemapCStr);
  if (iconJStr) env->ReleaseStringUTFChars(iconJStr, iconCStr);
  if (unitsJStr) env->ReleaseStringUTFChars(unitsJStr, unitsCStr);

  // Collect map lookup directories
  std::vector<std::filesystem::path> mapLookupPaths;
  if (dirsArray != nullptr) {
    jsize len = env->GetArrayLength(dirsArray);
    for (jsize i = 0; i < len; i++) {
      jstring dirJStr = (jstring)env->GetObjectArrayElement(dirsArray, i);
      if (dirJStr) {
        const char *dirCStr = env->GetStringUTFChars(dirJStr, nullptr);
        mapLookupPaths.emplace_back(dirCStr);
        env->ReleaseStringUTFChars(dirJStr, dirCStr);
        env->DeleteLocalRef(dirJStr);
      }
    }
  }

  // Collect custom POI types
  std::vector<std::string> customPoiTypes;
  if (customPoiArray != nullptr) {
    jsize len = env->GetArrayLength(customPoiArray);
    for (jsize i = 0; i < len; i++) {
      jstring typeJStr = (jstring)env->GetObjectArrayElement(customPoiArray, i);
      if (typeJStr) {
        const char *typeCStr = env->GetStringUTFChars(typeJStr, nullptr);
        if (typeCStr) {
          customPoiTypes.emplace_back(typeCStr);
          env->ReleaseStringUTFChars(typeJStr, typeCStr);
        }
        env->DeleteLocalRef(typeJStr);
      }
    }
  }

  // --- Create C++ objects ---
  auto clientData = std::make_unique<ClientData>();

  // Settings with in-memory storage
  auto storage = std::make_shared<InMemorySettingsStorage>();

  // Set stylesheet directory if provided
  if (styleDirCStr) {
    storage->SetValue("OSMScoutLib/Rendering/StylesheetDirectory", std::string(styleDirCStr));
    env->ReleaseStringUTFChars(styleDirJStr, styleDirCStr);
  }

  clientData->settings = std::make_shared<osmscout::Settings>(storage, dpi, units);

  // MapManager
  clientData->mapManager = std::make_shared<osmscout::MapManager>(mapLookupPaths);

  // DBThread
  clientData->dbThread = std::make_shared<osmscout::DBThread>(
    basemapDir,
    iconDir,
    clientData->settings,
    clientData->mapManager,
    customPoiTypes
  );

  // Paths explicitly opened via openDatabase() are tracked separately.
  // Lookup directories (including the default download maps parent) are
  // scanned recursively by MapManager; don't treat the parent as a database.
  clientData->knownPaths = {};

  // Initialise DBThread (triggers initial database scan)
  clientData->dbThread->Initialize();

  // Store the configured maps directory in settings, but do NOT create
  // MapDownloadService here. The Java side performs HTTP map-list fetches
  // in Java code and only calls nativeParseMapList for JSON parsing.
  // Downloads run synchronously on the Java thread via
  // MapDownloadService::DownloadMapSync, so no C++ background worker thread
  // is needed. Creating that worker thread from a Java thread triggers a JVM
  // crash in OpenJDK 17.0.2's GC barrier.
  jstring mapsDirJStr = (jstring)env->GetObjectField(self, mapsDirField);
  if (mapsDirJStr) {
    const char *mapsDirCStr = env->GetStringUTFChars(mapsDirJStr, nullptr);
    if (mapsDirCStr && strlen(mapsDirCStr) > 0) {
      clientData->settings->SetMapsDirectory(mapsDirCStr);
    }
    env->ReleaseStringUTFChars(mapsDirJStr, mapsDirCStr);
    env->DeleteLocalRef(mapsDirJStr);
  }

  // --- Create Java OSMScoutClient and set native handle ---
  jclass clientCls = env->FindClass("com/framstag/libosmscout/client/OSMScoutClient");
  jmethodID ctor = env->GetMethodID(clientCls, "<init>", "()V");
  jobject clientObj = env->NewObject(clientCls, ctor);

  ClientData *rawPtr = clientData.release();
  activeClient = rawPtr;
  setClientData(env, clientObj, rawPtr);

  return clientObj;
}

// --------------------------------------------------------------------------
// OSMScoutClient::openDatabase(String path)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_openDatabase(JNIEnv *env, jobject self, jstring pathJStr)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    return JNI_FALSE;
  }

  const char *pathCStr = env->GetStringUTFChars(pathJStr, nullptr);
  if (pathCStr == nullptr) {
    return JNI_FALSE;
  }

  std::filesystem::path fsPath(pathCStr);
  env->ReleaseStringUTFChars(pathJStr, pathCStr);

  // Add to known paths if not already present
  auto it = std::find(data->knownPaths.begin(), data->knownPaths.end(), fsPath);
  if (it == data->knownPaths.end()) {
    data->knownPaths.push_back(fsPath);
  }

  // Trigger DBThread to process the updated path list
  data->dbThread->OnDatabaseListChanged(data->knownPaths);

  return JNI_TRUE;
}

// --------------------------------------------------------------------------
// OSMScoutClient::close()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_close(JNIEnv *env, jobject self)
{
  std::scoped_lock lock(initMutex);

  ClientData *data = getClientData(env, self);
  if (data == nullptr) {
    return JNI_FALSE;
  }

  // Reset native handle on Java side
  setClientData(env, self, nullptr);

  // Cancel and join any in-progress routing thread
  {
    std::scoped_lock lock(data->routingMutex);
    if (data->breaker) {
      data->breaker->Break();
    }
    if (data->routingThread && data->routingThread->joinable()) {
      data->routingThread->join();
    }
  }

  // Drain pending async ops before destroying DBThread
  if (data->dbThread) {
    auto future = data->dbThread->OnDatabaseListChanged({});
    future.StdFuture().wait();
  }

  // Clear retained route descriptions
  {
    std::scoped_lock lock(data->routeDescriptionMutex);
    data->routeDescriptions.clear();
  }

  // Release C++ resources (shared_ptr destructors run here)
  delete data->favService;
  delete data;
  activeClient = nullptr;

  return JNI_TRUE;
}

// --------------------------------------------------------------------------
// OSMScoutClient::isInitialized()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_isInitialized(JNIEnv *env, jobject self)
{
  ClientData *data = getClientData(env, self);
  return (data != nullptr && data->dbThread != nullptr) ? JNI_TRUE : JNI_FALSE;
}

// --------------------------------------------------------------------------
// Forward declaration for renderWithRoute (called by render)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jintArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_renderWithRouteAndPois(JNIEnv *env, jobject self,
                                                                             jint width, jint height,
                                                                             jdouble lat, jdouble lon,
                                                                             jdouble angle,
                                                                             jint mag,
                                                                             jdoubleArray routeLats,
                                                                             jdoubleArray routeLons,
                                                                             jdoubleArray favoriteLats,
                                                                             jdoubleArray favoriteLons,
                                                                             jdouble searchSelLat,
                                                                             jdouble searchSelLon,
                                                                             jdoubleArray trackLats,
                                                                             jdoubleArray trackLons);

// --------------------------------------------------------------------------
// OSMScoutClient::importGpxTrack(String filePath)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_importGpxTrack(JNIEnv *env,
                                                                   jobject /*self*/,
                                                                   jstring filePathJStr)
{
  auto makeEmptyResult = [env]() -> jobjectArray {
    jclass trackPointCls = env->FindClass("com/framstag/libosmscout/client/TrackPoint");
    return env->NewObjectArray(0, trackPointCls, nullptr);
  };

#if !defined(OSMSCOUT_GPX_HAVE_LIB_XML)
  (void)filePathJStr;
  osmscout::log.Warn() << "importGpxTrack: GPX support is not compiled in";
  return makeEmptyResult();
#else
  jclass trackPointCls = env->FindClass("com/framstag/libosmscout/client/TrackPoint");
  jmethodID ctor = env->GetMethodID(trackPointCls, "<init>", "()V");
  jfieldID latField = env->GetFieldID(trackPointCls, "lat", "D");
  jfieldID lonField = env->GetFieldID(trackPointCls, "lon", "D");
  jfieldID timestampField = env->GetFieldID(trackPointCls, "timestamp", "Ljava/lang/String;");

  if (filePathJStr == nullptr) {
    return makeEmptyResult();
  }

  const char *pathCStr = env->GetStringUTFChars(filePathJStr, nullptr);
  if (pathCStr == nullptr) {
    return makeEmptyResult();
  }
  std::string filePath(pathCStr);
  env->ReleaseStringUTFChars(filePathJStr, pathCStr);

  osmscout::gpx::GpxFile gpxFile;
  if (!osmscout::gpx::ImportGpx(filePath, gpxFile)) {
    osmscout::log.Error() << "importGpxTrack: failed to import GPX file: " << filePath;
    return makeEmptyResult();
  }

  // Collect points from the first track, all segments
  std::vector<osmscout::gpx::TrackPoint> points;
  for (const auto &track : gpxFile.tracks) {
    for (const auto &segment : track.segments) {
      points.insert(points.end(), segment.points.begin(), segment.points.end());
    }
    break; // only first track
  }

  if (points.empty()) {
    return makeEmptyResult();
  }

  jobjectArray result = env->NewObjectArray(static_cast<jsize>(points.size()),
                                            trackPointCls, nullptr);
  if (result == nullptr) {
    return makeEmptyResult();
  }

  for (size_t i = 0; i < points.size(); i++) {
    jobject pointObj = env->NewObject(trackPointCls, ctor);
    env->SetDoubleField(pointObj, latField, points[i].coord.GetLat());
    env->SetDoubleField(pointObj, lonField, points[i].coord.GetLon());
    if (points[i].timestamp.has_value()) {
      env->SetObjectField(pointObj, timestampField,
                          env->NewStringUTF(osmscout::TimestampToISO8601TimeString(
                              points[i].timestamp.value()).c_str()));
    }
    env->SetObjectArrayElement(result, static_cast<jsize>(i), pointObj);
    env->DeleteLocalRef(pointObj);
  }

  return result;
#endif
}

// --------------------------------------------------------------------------
// OSMScoutClient::render(int width, int height, double lat, double lon, double angle, int mag)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jintArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_render(JNIEnv *env, jobject self,
                                                           jint width, jint height,
                                                           jdouble lat, jdouble lon,
                                                           jdouble angle,
                                                           jint mag)
{
  return Java_com_framstag_libosmscout_client_OSMScoutClient_renderWithRouteAndPois(
      env, self, width, height, lat, lon, angle, mag, nullptr, nullptr, nullptr, nullptr,
      std::numeric_limits<jdouble>::quiet_NaN(),
      std::numeric_limits<jdouble>::quiet_NaN(),
      nullptr, nullptr);
}

// --------------------------------------------------------------------------
// OSMScoutClient::renderWithRouteAndPois(int width, int height, double lat, double lon,
//                                         double angle, int mag, double[] routeLats, double[] routeLons,
//                                         double[] favoriteLats, double[] favoriteLons,
//                                         double searchSelLat, double searchSelLon)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jintArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_renderWithRouteAndPois(JNIEnv *env, jobject self,
                                                                             jint width, jint height,
                                                                             jdouble lat, jdouble lon,
                                                                             jdouble angle,
                                                                             jint mag,
                                                                             jdoubleArray routeLats,
                                                                             jdoubleArray routeLons,
                                                                             jdoubleArray favoriteLats,
                                                                             jdoubleArray favoriteLons,
                                                                             jdouble searchSelLat,
                                                                             jdouble searchSelLon,
                                                                             jdoubleArray trackLats,
                                                                             jdoubleArray trackLons)
{

  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    return nullptr;
  }

  if (width <= 0 || height <= 0) {
    return nullptr;
  }

  osmscout::GeoCoord center(lat, lon);
  if (!center.IsValid()) {
    return nullptr;
  }

  osmscout::Magnification magnification;
  magnification.SetLevel(osmscout::MagnificationLevel(static_cast<uint32_t>(mag)));

  double dpi = data->settings ? data->settings->GetMapDPI() : 96.0;

  // Extract route overlay data if provided
  std::vector<osmscout::Point> routePoints;
  bool hasRoute = (routeLats != nullptr && routeLons != nullptr);
  if (hasRoute) {
    jsize latCount = env->GetArrayLength(routeLats);
    jsize lonCount = env->GetArrayLength(routeLons);
    jsize count = std::min(latCount, lonCount);

    std::vector<jdouble> latValues(static_cast<size_t>(count));
    std::vector<jdouble> lonValues(static_cast<size_t>(count));
    env->GetDoubleArrayRegion(routeLats, 0, count, latValues.data());
    env->GetDoubleArrayRegion(routeLons, 0, count, lonValues.data());

    routePoints.reserve(static_cast<size_t>(count));
    for (jsize i = 0; i < count; i++) {
      routePoints.emplace_back(0,
          osmscout::GeoCoord(latValues[i], lonValues[i]));
    }
  }

  // Extract favorite marker data if provided
  std::vector<osmscout::GeoCoord> favoriteCoords;
  bool hasFavorites = (favoriteLats != nullptr && favoriteLons != nullptr);
  if (hasFavorites) {
    jsize latCount = env->GetArrayLength(favoriteLats);
    jsize lonCount = env->GetArrayLength(favoriteLons);
    jsize count = std::min(latCount, lonCount);

    std::vector<jdouble> latValues(static_cast<size_t>(count));
    std::vector<jdouble> lonValues(static_cast<size_t>(count));
    env->GetDoubleArrayRegion(favoriteLats, 0, count, latValues.data());
    env->GetDoubleArrayRegion(favoriteLons, 0, count, lonValues.data());

    favoriteCoords.reserve(static_cast<size_t>(count));
    for (jsize i = 0; i < count; i++) {
      osmscout::GeoCoord coord(latValues[i], lonValues[i]);
      if (coord.IsValid()) {
        favoriteCoords.push_back(coord);
      }
    }
  }

  bool hasSearchSelected = !std::isnan(static_cast<double>(searchSelLat)) &&
                           !std::isnan(static_cast<double>(searchSelLon));

  // Extract track overlay data if provided
  std::vector<osmscout::Point> trackPoints;
  bool hasTrack = (trackLats != nullptr && trackLons != nullptr);
  if (hasTrack) {
    jsize latCount = env->GetArrayLength(trackLats);
    jsize lonCount = env->GetArrayLength(trackLons);
    jsize count = std::min(latCount, lonCount);

    std::vector<jdouble> latValues(static_cast<size_t>(count));
    std::vector<jdouble> lonValues(static_cast<size_t>(count));
    env->GetDoubleArrayRegion(trackLats, 0, count, latValues.data());
    env->GetDoubleArrayRegion(trackLons, 0, count, lonValues.data());

    trackPoints.reserve(static_cast<size_t>(count));
    for (jsize i = 0; i < count; i++) {
      osmscout::GeoCoord coord(latValues[i], lonValues[i]);
      if (coord.IsValid()) {
        trackPoints.emplace_back(0, coord);
      }
    }
  }

  // Pixel buffer for result
  size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
  std::vector<uint32_t> argbPixels(pixelCount, 0xFF000000); // default: opaque black

  bool rendered = false;

  data->dbThread->RunSynchronousJob(
    [&](const std::list<osmscout::DBInstanceRef> &databases,
        const osmscout::DBInstanceRef &basemapDatabase) {
      if (databases.empty() && !basemapDatabase) {
        return;
      }

      // Create projection
      osmscout::MercatorProjection projection;
      if (!projection.Set(center, angle, magnification, dpi,
                          static_cast<size_t>(width),
                          static_cast<size_t>(height))) {
        return;
      }

      // Create map parameter
      osmscout::MapParameter params;
      params.SetFontName("sans-serif");
      params.SetFontSize(3.0);
      params.SetRenderSeaLand(true);
      params.SetRenderBackground(true);
      params.SetRenderUnknowns(true);
      params.SetIconMode(osmscout::MapParameter::IconMode::ScaledPixmap);

      std::string iconDir = data->dbThread->GetIconDirectory();
      if (!iconDir.empty()) {
        params.SetIconPaths({iconDir});
        osmscout::log.Debug() << "Using icon directory: " << iconDir;
      } else {
        std::cerr << "[OSMScoutClient] no icon directory configured" << std::endl;
      }

      // Collect map data from all databases
      std::vector<osmscout::MapData> batch;

      // Helper lambda to load map data for one database
      auto loadDbData = [&](const osmscout::DBInstanceRef &db) {
        if (!db->GetStyleConfig()) {
          return;
        }

        osmscout::MapData mapData;
        mapData.styleConfig = db->GetStyleConfig();
        mapData.basemap = db->GetDatabase()->IsBasemap();

        // Lookup tiles for this projection
        std::list<osmscout::TileRef> tiles;
        db->GetMapService()->LookupTiles(projection, tiles);

        // Load tile data
        osmscout::AreaSearchParameter searchParam;
        searchParam.SetUseMultithreading(false);
        searchParam.SetMaximumAreaLevel(4);
        db->GetMapService()->LoadMissingTileData(searchParam,
                                                  *db->GetStyleConfig(),
                                                  tiles);

        // Add tile data to map data
        db->GetMapService()->AddTileDataToMapData(tiles, mapData);

        // Get ground tiles (sea/land)
        // Basemap databases use baseMapTiles, regular databases use groundTiles.
        // When a basemap is present, let it provide the global sea/land background;
        // regional ground tiles would otherwise draw unknown/water over basemap land.
        if (params.GetRenderSeaLand()) {
          if (IsBasemapDatabase(db)) {
            db->GetMapService()->GetGroundTiles(projection,
                                                mapData.baseMapTiles);
          } else if (!basemapDatabase) {
            db->GetMapService()->GetGroundTiles(projection,
                                                mapData.groundTiles);
          }
        }

        batch.emplace_back(std::move(mapData));
      };

      // Load regular databases first
      for (const auto &db : databases) {
        loadDbData(db);
      }

      // If no regular databases are loaded but a basemap is available,
      // create a separate MapData entry for the basemap so it renders on its own.
      if (batch.empty() && basemapDatabase && basemapDatabase->GetStyleConfig()) {
        osmscout::MapData mapData;
        mapData.styleConfig = basemapDatabase->GetStyleConfig();
        mapData.basemap = basemapDatabase->GetDatabase()->IsBasemap();

        if (params.GetRenderSeaLand()) {
          basemapDatabase->GetMapService()->GetGroundTiles(projection,
                                                              mapData.baseMapTiles);
        }

        batch.emplace_back(std::move(mapData));
      }

      // Load basemap ground tiles into the first database's baseMapTiles
      // (rendered underneath regional maps when a regional map exists)
      if (basemapDatabase && basemapDatabase->GetStyleConfig() && !batch.empty()) {
        std::list<osmscout::GroundTile> baseTiles;
        basemapDatabase->GetMapService()->GetGroundTiles(projection,
                                                             baseTiles);
        if (!baseTiles.empty()) {
          batch.front().baseMapTiles.splice(batch.front().baseMapTiles.end(),
                                            baseTiles);
        }
      }

      // Add route overlay + marker overlays to the LAST database's map data
      if (!batch.empty()) {
        const osmscout::DBInstanceRef &lastDbInstance = databases.empty() ? basemapDatabase : databases.back();
        osmscout::TypeConfigRef typeConfig;
        if (lastDbInstance) {
          typeConfig = lastDbInstance->GetDatabase()->GetTypeConfig();
        }
        if (typeConfig) {
          auto addPoiNode = [&](const osmscout::GeoCoord &coord,
                                const std::string &typeName) {
            osmscout::TypeInfoRef type = typeConfig->GetTypeInfo(typeName);
            if (!type) {
              return;
            }
            osmscout::NodeRef node = std::make_shared<osmscout::Node>();
            node->SetCoords(coord);
            node->SetType(type);
            batch.back().poiNodes.push_back(node);
          };

          if (hasRoute && !routePoints.empty()) {
            // Create route way
            osmscout::WayRef routeWay = std::make_shared<osmscout::Way>();
            routeWay->nodes = routePoints;

            osmscout::TypeInfoRef routeType = typeConfig->GetTypeInfo("_route");
            if (routeType) {
              routeWay->SetType(routeType);

              // Stack the active route above all map ways (bridges, tunnels):
              // set the layer feature value on the route way. The renderer is
              // generic and just honors the layer of each way.
              osmscout::FeatureValueBuffer routeFeatures;
              routeFeatures.SetType(routeType);
              size_t featureIndex;
              if (routeType->GetFeature(osmscout::LayerFeature::NAME,
                                        featureIndex)) {
                auto* value=static_cast<osmscout::LayerFeatureValue*>(routeFeatures.AllocateValue(featureIndex));
                value->SetLayer(osmscout::MapPainter::routeLayer);
              }
              routeWay->SetFeatures(routeFeatures);

              batch.back().poiWays.push_back(routeWay);
            }

            // Create start/end marker nodes
            if (routePoints.size() >= 1) {
              addPoiNode(routePoints.front().GetCoord(), "_route_start");
            }
            if (routePoints.size() >= 2) {
              addPoiNode(routePoints.back().GetCoord(), "_route_end");
            }
          }

          // Favorite markers
          for (const auto &coord : favoriteCoords) {
            addPoiNode(coord, "_favorite");
          }

          // Selected search result marker
          if (hasSearchSelected) {
            osmscout::GeoCoord selCoord(searchSelLat, searchSelLon);
            if (selCoord.IsValid()) {
              addPoiNode(selCoord, "_search_selected");
            }
          }

          // Imported track polyline
          if (hasTrack && !trackPoints.empty()) {
            osmscout::WayRef trackWay = std::make_shared<osmscout::Way>();
            trackWay->nodes = trackPoints;

            osmscout::TypeInfoRef trackType = typeConfig->GetTypeInfo("_track");
            if (trackType) {
              trackWay->SetType(trackType);
              batch.back().poiWays.push_back(trackWay);
            }
          }
        }
      }

      if (batch.empty()) {
        return;
      }

      // Create Cairo surface and render
#ifdef HAVE_MAP_CAIRO
      cairo_surface_t *surface = cairo_image_surface_create(
          CAIRO_FORMAT_RGB24,
          static_cast<int>(width),
          static_cast<int>(height));
      if (cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS) {
        return;
      }

      cairo_t *cr = cairo_create(surface);

      osmscout::MapPainterCairo painter;
      if (!painter.DrawMap(projection, params, batch, cr)) {
        cairo_destroy(cr);
        cairo_surface_destroy(surface);
        return;
      }

      cairo_destroy(cr);

      // Flush surface so pixel data is valid for reading
      cairo_surface_flush(surface);

      // Read pixel data from surface
      // CAIRO_FORMAT_RGB24 is stored as 4 bytes per pixel (BGRx on little-endian)
      unsigned char *cairoData = cairo_image_surface_get_data(surface);
      int stride = cairo_image_surface_get_stride(surface);

      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
          int offset = y * stride + x * 4;
          uint8_t b = cairoData[offset + 0];
          uint8_t g = cairoData[offset + 1];
          uint8_t r = cairoData[offset + 2];
          // Alpha is ignored in CAIRO_FORMAT_RGB24, set to fully opaque
          argbPixels[static_cast<size_t>(y) * width + x] =
              0xFF000000 | (static_cast<uint32_t>(r) << 16) |
              (static_cast<uint32_t>(g) << 8) | b;
        }
      }

      cairo_surface_destroy(surface);
      rendered = true;
#else
      osmscout::log.Warn() << "Cairo backend not available, cannot render";
      return;
#endif
    }
  );

  if (!rendered) {
    return nullptr;
  }

  // Create Java int[] and copy pixels
  jintArray result = env->NewIntArray(static_cast<jsize>(pixelCount));
  if (result == nullptr) {
    return nullptr;
  }

  env->SetIntArrayRegion(result, 0, static_cast<jsize>(pixelCount),
                         reinterpret_cast<const jint *>(argbPixels.data()));

  return result;
}

// --------------------------------------------------------------------------
// OSMScoutClient::projectToPixel(int width, int height, double centerLat,
//                                 double centerLon, int mag, double dpi,
//                                 double angle, double lat, double lon)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jdoubleArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_projectToPixel(JNIEnv *env,
                                                                   jobject /*self*/,
                                                                   jint width,
                                                                   jint height,
                                                                   jdouble centerLat,
                                                                   jdouble centerLon,
                                                                   jint mag,
                                                                   jdouble dpi,
                                                                   jdouble angle,
                                                                   jdouble lat,
                                                                   jdouble lon)
{
  osmscout::GeoCoord center(centerLat, centerLon);
  osmscout::GeoCoord coord(lat, lon);
  if (!center.IsValid() || !coord.IsValid() || width <= 0 || height <= 0 || dpi <= 0.0) {
    return nullptr;
  }

  osmscout::Magnification magnification;
  magnification.SetLevel(osmscout::MagnificationLevel(static_cast<uint32_t>(mag)));

  osmscout::MercatorProjection projection;
  if (!projection.Set(center, angle, magnification, static_cast<double>(dpi),
                      static_cast<size_t>(width),
                      static_cast<size_t>(height))) {
    return nullptr;
  }

  osmscout::Vertex2D pixel;
  if (!projection.GeoToPixel(coord, pixel)) {
    return nullptr;
  }

  jdoubleArray result = env->NewDoubleArray(2);
  if (result != nullptr) {
    jdouble values[2] = { pixel.GetX(), pixel.GetY() };
    env->SetDoubleArrayRegion(result, 0, 2, values);
  }
  return result;
}

// --------------------------------------------------------------------------
// OSMScoutClient::searchLocations(String query, int limit)
// --------------------------------------------------------------------------

// --------------------------------------------------------------------------
// Navigation support
// --------------------------------------------------------------------------

static std::string LaneTurnToString(osmscout::LaneTurn turn)
{
  return osmscout::LaneTurnString(turn);
}

static std::string PositionStateToString(osmscout::PositionAgent::PositionState state)
{
  switch (state) {
    case osmscout::PositionAgent::PositionState::Uninitialised: return "Uninitialised";
    case osmscout::PositionAgent::PositionState::NoGpsSignal: return "NoGpsSignal";
    case osmscout::PositionAgent::PositionState::OnRoute: return "OnRoute";
    case osmscout::PositionAgent::PositionState::OffRoute: return "OffRoute";
    case osmscout::PositionAgent::PositionState::EstimateInTunnel: return "EstimateInTunnel";
  }
  return "Uninitialised";
}

struct NavigationListenerMethods
{
  jmethodID onPositionEstimate{nullptr};
  jmethodID onRerouteRequest{nullptr};
  jmethodID onTargetReached{nullptr};
  jmethodID onArrivalEstimate{nullptr};
  jmethodID onCurrentSpeed{nullptr};
  jmethodID onMaxAllowedSpeed{nullptr};
  jmethodID onLaneUpdate{nullptr};
  jmethodID onVoiceInstruction{nullptr};
  jmethodID onError{nullptr};
  jmethodID onRouteInstructions{nullptr};
  jmethodID onNextRouteInstruction{nullptr};

  jobject positionClsGlobal{nullptr};
  jmethodID positionCtor{nullptr};

  jobject stateClsGlobal{nullptr};
  jmethodID stateValueOf{nullptr};

  jobject instructionClsGlobal{nullptr};
  jmethodID instructionCtor{nullptr};

  jobject turnTypeClsGlobal{nullptr};
  jmethodID turnTypeValueOf{nullptr};

  jobject laneTurnClsGlobal{nullptr};
  jmethodID laneTurnFromId{nullptr};
};

static bool GetNavigationListenerMethods(JNIEnv *env, jobject listener,
                                         NavigationListenerMethods &methods)
{
  jclass listenerCls = env->GetObjectClass(listener);
  if (!listenerCls) {
    return false;
  }

  methods.onPositionEstimate = env->GetMethodID(
      listenerCls, "onPositionEstimate",
      "(Lcom/framstag/libosmscout/client/NavigationPosition;)V");
  methods.onRerouteRequest = env->GetMethodID(
      listenerCls, "onRerouteRequest",
      "(DDDDD)V");
  methods.onTargetReached = env->GetMethodID(
      listenerCls, "onTargetReached",
      "(DD)V");
  methods.onArrivalEstimate = env->GetMethodID(
      listenerCls, "onArrivalEstimate",
      "(JD)V");
  methods.onCurrentSpeed = env->GetMethodID(
      listenerCls, "onCurrentSpeed",
      "(D)V");
  methods.onMaxAllowedSpeed = env->GetMethodID(
      listenerCls, "onMaxAllowedSpeed",
      "(D)V");
  methods.onLaneUpdate = env->GetMethodID(
      listenerCls, "onLaneUpdate",
      "(ZIZIILjava/lang/String;[Lcom/framstag/libosmscout/client/LaneTurn;)V");
  methods.onVoiceInstruction = env->GetMethodID(
      listenerCls, "onVoiceInstruction",
      "([I)V");
  methods.onError = env->GetMethodID(
      listenerCls, "onError",
      "(Ljava/lang/String;)V");

  jclass positionCls = env->FindClass("com/framstag/libosmscout/client/NavigationPosition");
  if (positionCls) {
    methods.positionClsGlobal = env->NewGlobalRef(positionCls);
    methods.positionCtor = env->GetMethodID(
        positionCls, "<init>",
        "(Lcom/framstag/libosmscout/client/NavigationState;DDDD)V");
  }

  jclass stateCls = env->FindClass("com/framstag/libosmscout/client/NavigationState");
  if (stateCls) {
    methods.stateClsGlobal = env->NewGlobalRef(stateCls);
    methods.stateValueOf = env->GetStaticMethodID(
        stateCls, "valueOf",
        "(Ljava/lang/String;)Lcom/framstag/libosmscout/client/NavigationState;");
  }

  // Route instruction callbacks
  methods.onRouteInstructions = env->GetMethodID(
      listenerCls, "onRouteInstructions",
      "([Lcom/framstag/libosmscout/client/RouteInstruction;)V");
  methods.onNextRouteInstruction = env->GetMethodID(
      listenerCls, "onNextRouteInstruction",
      "(Lcom/framstag/libosmscout/client/RouteInstruction;)V");

  jclass instructionCls = env->FindClass("com/framstag/libosmscout/client/RouteInstruction");
  if (instructionCls) {
    methods.instructionClsGlobal = env->NewGlobalRef(instructionCls);
    methods.instructionCtor = env->GetMethodID(
        instructionCls, "<init>",
        "(DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;DLcom/framstag/libosmscout/client/TurnType;Ljava/lang/String;Ljava/lang/String;)V");
  }

  jclass turnTypeCls = env->FindClass("com/framstag/libosmscout/client/TurnType");
  if (turnTypeCls) {
    methods.turnTypeClsGlobal = env->NewGlobalRef(turnTypeCls);
    methods.turnTypeValueOf = env->GetStaticMethodID(
        turnTypeCls, "fromString",
        "(Ljava/lang/String;)Lcom/framstag/libosmscout/client/TurnType;");
  }

  jclass laneTurnCls = env->FindClass("com/framstag/libosmscout/client/LaneTurn");
  if (laneTurnCls) {
    methods.laneTurnClsGlobal = env->NewGlobalRef(laneTurnCls);
    methods.laneTurnFromId = env->GetStaticMethodID(
        laneTurnCls, "fromId",
        "(I)Lcom/framstag/libosmscout/client/LaneTurn;");
  }

  return true;
}

// --------------------------------------------------------------------------
// JavaRouteInstruction — lightweight instruction type for JNI bridge
// --------------------------------------------------------------------------

struct JavaRouteInstruction
{
  double distanceTo{0.0};       // meters to next manoeuvre
  std::string turnType;         // "sharpLeft", "left", "straightOn", etc.
  std::string streetName;       // street to turn into
  std::string description;      // "Turn left into Hauptstrasse"
  std::string shortDescription; // "Turn left"

  // Optional "next next" hint — populated when the following instruction
  // is within CloseHintDistance of the current one (e.g. roundabout exit).
  double nextNextDistanceTo{0.0};
  std::string nextNextTurnType;
  std::string nextNextDescription;
  std::string nextNextShortDescription;

  static constexpr double CloseHintDistance = 200.0; // meters

  osmscout::Distance GetDistance() const
  {
    return osmscout::Distance::Of<osmscout::Meter>(distanceTo);
  }
};

// --------------------------------------------------------------------------
// JavaRouteInstructionBuilder — walks RouteDescription nodes via
// RouteDescriptionPostprocessor and produces JavaRouteInstruction lists.
// Duplicates some logic from Qt's RouteDescriptionBuilder but avoids
// pulling Qt into the Java JNI build.
// --------------------------------------------------------------------------

class JavaRouteInstructionBuilder
{
public:
  std::list<JavaRouteInstruction> GenerateRouteInstructions(
      osmscout::RouteDescription::NodeIterator first,
      osmscout::RouteDescription::NodeIterator last) const
  {
    std::list<JavaRouteInstruction> result;
    CollectCallback callback(result);
    osmscout::RouteDescriptionPostprocessor postprocessor;
    postprocessor.GenerateDescription(first, last, callback);
    return result;
  }

  JavaRouteInstruction GenerateNextRouteInstruction(
      osmscout::RouteDescription::NodeIterator previous,
      osmscout::RouteDescription::NodeIterator last,
      const osmscout::GeoCoord &coord) const
  {
    if (previous == last) {
      return JavaRouteInstruction{};
    }

    // Collect instructions from current position forward.
    // Use generous stopAfter so we get enough nodes ahead for "next next".
    double nodeDist = previous->GetDistance().AsMeter();
    osmscout::Distance stopAfter = previous->GetDistance()
        + osmscout::Distance::Of<osmscout::Meter>(JavaRouteInstruction::CloseHintDistance * 2);

    std::list<JavaRouteInstruction> collected;
    CollectCallback callback(collected, stopAfter);
    osmscout::RouteDescriptionPostprocessor postprocessor;
    postprocessor.GenerateDescription(previous, last, callback);

    if (collected.empty()) {
      return JavaRouteInstruction{};
    }

    // Find first instruction past current node → "next"
    auto it = collected.begin();
    while (it != collected.end() && it->distanceTo <= nodeDist) {
      ++it;
    }
    if (it == collected.end()) {
      return JavaRouteInstruction{};
    }

    double nextAbs = it->distanceTo; // absolute distance of next instruction
    JavaRouteInstruction next = *it;
    ++it;
    bool hasNextNext = (it != collected.end());

    // Convert absolute distances to remaining distances
    double travelled = osmscout::GetEllipsoidalDistance(coord, previous->GetLocation()).AsMeter();
    double raw = nextAbs - nodeDist - travelled;
    next.distanceTo = (raw > 0.0) ? raw : 0.0;

    // Populate "next next" hint
    if (hasNextNext) {
      double nnAbs = it->distanceTo;
      double gap = nnAbs - nextAbs;
      if (gap > 0 && gap <= JavaRouteInstruction::CloseHintDistance) {
        next.nextNextDistanceTo = gap; // relative to "next", not absolute
        next.nextNextTurnType = it->turnType;
        next.nextNextDescription = it->description;
        next.nextNextShortDescription = it->shortDescription;
      }
    }

    return next;
  }

private:
  // -- Turn type helpers (mirrors Demos/src/Navigation.cpp::MoveToTurnCommand) --

  static std::string MoveToTurnType(osmscout::RouteDescription::DirectionDescription::Move move)
  {
    switch (move) {
      case osmscout::RouteDescription::DirectionDescription::sharpLeft:    return "sharpLeft";
      case osmscout::RouteDescription::DirectionDescription::left:         return "left";
      case osmscout::RouteDescription::DirectionDescription::slightlyLeft: return "slightlyLeft";
      case osmscout::RouteDescription::DirectionDescription::straightOn:   return "straightOn";
      case osmscout::RouteDescription::DirectionDescription::slightlyRight:return "slightlyRight";
      case osmscout::RouteDescription::DirectionDescription::right:        return "right";
      case osmscout::RouteDescription::DirectionDescription::sharpRight:   return "sharpRight";
    }
    return "straightOn";
  }

  static std::string MoveToDescription(osmscout::RouteDescription::DirectionDescription::Move move)
  {
    switch (move) {
      case osmscout::RouteDescription::DirectionDescription::sharpLeft:    return "Turn sharp left";
      case osmscout::RouteDescription::DirectionDescription::left:         return "Turn left";
      case osmscout::RouteDescription::DirectionDescription::slightlyLeft: return "Turn slightly left";
      case osmscout::RouteDescription::DirectionDescription::straightOn:   return "Straight on";
      case osmscout::RouteDescription::DirectionDescription::slightlyRight:return "Turn slightly right";
      case osmscout::RouteDescription::DirectionDescription::right:        return "Turn right";
      case osmscout::RouteDescription::DirectionDescription::sharpRight:   return "Turn sharp right";
    }
    return "Straight on";
  }

  static std::string NameOrRef(const osmscout::RouteDescription::NameDescriptionRef &nameDesc)
  {
    if (!nameDesc) return "";
    if (nameDesc->HasName()) return nameDesc->GetName();
    std::string ref = nameDesc->GetRef();
    return ref.empty() ? "" : ref;
  }

  // -- Callback that collects JavaRouteInstruction from RouteDescriptionPostprocessor --

  class CollectCallback : public osmscout::RouteDescriptionPostprocessor::Callback
  {
  private:
    std::list<JavaRouteInstruction> &result;
    osmscout::Distance stopAfter; // < 0 = unlimited
    osmscout::GeoCoord coord;
    osmscout::Distance distance;
    std::string currentStreet;

  public:
    CollectCallback(std::list<JavaRouteInstruction> &result,
                    osmscout::Distance stopAfter = osmscout::Distance::Lowest())
      : result(result), stopAfter(stopAfter) {}

    void BeforeNode(const osmscout::RouteDescription::Node &node) override
    {
      distance = node.GetDistance();
      coord = node.GetLocation();
    }

    bool Continue() const override
    {
      return stopAfter < osmscout::Distance::Zero() ||
             result.empty() ||
             result.back().distanceTo <= stopAfter.AsMeter();
    }

    void OnStart(const osmscout::RouteDescription::StartDescriptionRef &startDesc,
                 const osmscout::RouteDescription::TypeNameDescriptionRef & /*typeNameDesc*/,
                 const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
    {
      currentStreet = NameOrRef(nameDesc);
      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = "start";
      instr.streetName = currentStreet;
      instr.description = startDesc ? startDesc->GetDescription() : "Start";
      instr.shortDescription = "Start";
      result.push_back(instr);
    }

    void OnTargetReached(const osmscout::RouteDescription::TargetDescriptionRef &targetDesc) override
    {
      JavaRouteInstruction instr;
      instr.distanceTo = 0.0;
      instr.turnType = "targetReached";
      instr.description = targetDesc ? targetDesc->GetDescription() : "Destination reached";
      instr.shortDescription = "Arrive";
      result.push_back(instr);
    }

    void OnTurn(const osmscout::RouteDescription::TurnDescriptionRef &turnDesc,
                const osmscout::RouteDescription::CrossingWaysDescriptionRef &crossingWaysDesc,
                const osmscout::RouteDescription::DirectionDescriptionRef &directionDesc,
                const osmscout::RouteDescription::TypeNameDescriptionRef & /*typeNameDesc*/,
                const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
    {
      auto move = turnDesc ? turnDesc->GetDirection()
                 : (directionDesc ? directionDesc->GetCurve()
                    : osmscout::RouteDescription::DirectionDescription::straightOn);

      std::string street = NameOrRef(nameDesc);
      if (street.empty() && crossingWaysDesc) {
        street = NameOrRef(crossingWaysDesc->GetTargetDesccription());
      }

      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = MoveToTurnType(move);
      instr.streetName = street;
      instr.description = MoveToDescription(move) + (street.empty() ? "" : " into " + street);
      instr.shortDescription = MoveToDescription(move);
      result.push_back(instr);
    }

    void OnRoundaboutEnter(const osmscout::RouteDescription::RoundaboutEnterDescriptionRef & /*roundaboutEnterDesc*/,
                           const osmscout::RouteDescription::CrossingWaysDescriptionRef & /*crossingWaysDesc*/) override
    {
      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = "roundaboutEnter";
      instr.description = "Enter roundabout";
      instr.shortDescription = "Roundabout";
      result.push_back(instr);
    }

    void OnRoundaboutLeave(const osmscout::RouteDescription::RoundaboutLeaveDescriptionRef &roundaboutLeaveDesc,
                           const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
    {
      std::string exitStr = roundaboutLeaveDesc
          ? std::to_string(roundaboutLeaveDesc->GetExitCount()) : "?";
      std::string street = NameOrRef(nameDesc);

      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = "roundaboutLeave";
      instr.streetName = street;
      instr.description = "Take exit " + exitStr + (street.empty() ? "" : " onto " + street);
      instr.shortDescription = "Exit " + exitStr;
      result.push_back(instr);
    }

    void OnMotorwayEnter(const osmscout::RouteDescription::MotorwayEnterDescriptionRef &motorwayEnterDesc,
                         const osmscout::RouteDescription::CrossingWaysDescriptionRef & /*crossingWaysDesc*/) override
    {
      std::string motorway = motorwayEnterDesc
          ? NameOrRef(motorwayEnterDesc->GetToDescription()) : "";

      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = "motorwayEnter";
      instr.streetName = motorway;
      instr.description = "Enter " + (motorway.empty() ? "motorway" : motorway);
      instr.shortDescription = "Enter motorway";
      result.push_back(instr);
    }

    void OnMotorwayChange(const osmscout::RouteDescription::MotorwayChangeDescriptionRef &motorwayChangeDesc,
                          const osmscout::RouteDescription::MotorwayJunctionDescriptionRef & /*motorwayJunctionDesc*/,
                          const osmscout::RouteDescription::DirectionDescriptionRef &directionDesc,
                          const osmscout::RouteDescription::DestinationDescriptionRef & /*destDesc*/) override
    {
      auto move = directionDesc ? directionDesc->GetCurve()
                  : osmscout::RouteDescription::DirectionDescription::straightOn;
      std::string toMotorway = motorwayChangeDesc
          ? NameOrRef(motorwayChangeDesc->GetToDescription()) : "";

      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = MoveToTurnType(move);
      instr.streetName = toMotorway;
      instr.description = "Keep " + MoveToDescription(move) + " onto " + (toMotorway.empty() ? "motorway" : toMotorway);
      instr.shortDescription = "Keep " + MoveToTurnType(move);
      result.push_back(instr);
    }

    void OnMotorwayLeave(const osmscout::RouteDescription::MotorwayLeaveDescriptionRef & /*motorwayLeaveDesc*/,
                         const osmscout::RouteDescription::MotorwayJunctionDescriptionRef & /*motorwayJunctionDesc*/,
                         const osmscout::RouteDescription::DirectionDescriptionRef &directionDesc,
                         const osmscout::RouteDescription::NameDescriptionRef &nameDesc,
                         const osmscout::RouteDescription::DestinationDescriptionRef & /*destDesc*/) override
    {
      auto move = directionDesc ? directionDesc->GetCurve()
                  : osmscout::RouteDescription::DirectionDescription::straightOn;
      std::string street = NameOrRef(nameDesc);

      JavaRouteInstruction instr;
      instr.distanceTo = distance.AsMeter();
      instr.turnType = MoveToTurnType(move);
      instr.streetName = street;
      instr.description = MoveToDescription(move) + (street.empty() ? "" : " into " + street);
      instr.shortDescription = "Leave motorway";
      result.push_back(instr);
    }

    void OnPathNameChange(const osmscout::RouteDescription::NameChangedDescriptionRef &nameChangedDesc) override
    {
      currentStreet = nameChangedDesc
          ? NameOrRef(nameChangedDesc->GetTargetDescription()) : "";
    }
  };
};

class JavaNavigationController
{
public:
  JavaNavigationController(ClientData *clientData,
                           const osmscout::RouteDescriptionRef &routeDescription,
                           osmscout::Vehicle vehicle,
                           JavaVM *jvm,
                           jobject listenerGlobal,
                           const NavigationListenerMethods &methods)
    : data(clientData),
      routeDescription(routeDescription),
      vehicle(vehicle),
      jvm(jvm),
      listenerGlobal(listenerGlobal),
      methods(methods),
      engine{
        std::make_shared<osmscout::DataAgent<JavaNavigationController>>(*this),
        std::make_shared<osmscout::PositionAgent>(),
        std::make_shared<osmscout::BearingAgent>(),
        std::make_shared<osmscout::RouteStateAgent>(),
        std::make_shared<osmscout::ArrivalEstimateAgent>(),
        std::make_shared<osmscout::SpeedAgent>(),
        std::make_shared<osmscout::LaneAgent>(),
        std::make_shared<osmscout::VoiceInstructionAgent>(osmscout::DistanceUnitSystem::Metrics, std::make_shared<osmscout::NoOpTTSMessageGenerator>()),
        std::make_shared<osmscout::RouteInstructionAgent<JavaRouteInstruction, JavaRouteInstructionBuilder>>()
      }
  {
  }

  ClientData* GetClientData() const
  {
    return data;
  }

  ~JavaNavigationController()
  {
    Stop();
  }

  void Start()
  {
    std::scoped_lock lock(threadMutex);
    if (thread.joinable()) {
      return;
    }

    running = true;
    thread = std::thread(&JavaNavigationController::Run, this);
  }

  void Stop()
  {
    {
      std::scoped_lock lock(queueMutex);
      running = false;
    }
    queueCv.notify_all();

    std::scoped_lock lock(threadMutex);
    if (thread.joinable()) {
      thread.join();
    }

    if (jvm != nullptr && listenerGlobal != nullptr) {
      JNIEnv *env = nullptr;
      bool attachedByUs = false;
      if (jvm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) == JNI_EDETACHED ||
          env == nullptr) {
        if (jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr) == JNI_OK) {
          attachedByUs = true;
        } else {
          env = nullptr;
        }
      }
      if (env != nullptr) {
        env->DeleteGlobalRef(listenerGlobal);
        if (methods.positionClsGlobal != nullptr) {
          env->DeleteGlobalRef(methods.positionClsGlobal);
          methods.positionClsGlobal = nullptr;
        }
        if (methods.stateClsGlobal != nullptr) {
          env->DeleteGlobalRef(methods.stateClsGlobal);
          methods.stateClsGlobal = nullptr;
        }
        if (methods.instructionClsGlobal != nullptr) {
          env->DeleteGlobalRef(methods.instructionClsGlobal);
          methods.instructionClsGlobal = nullptr;
        }
        if (methods.turnTypeClsGlobal != nullptr) {
          env->DeleteGlobalRef(methods.turnTypeClsGlobal);
          methods.turnTypeClsGlobal = nullptr;
        }
        if (methods.laneTurnClsGlobal != nullptr) {
          env->DeleteGlobalRef(methods.laneTurnClsGlobal);
          methods.laneTurnClsGlobal = nullptr;
        }
        if (attachedByUs) {
          jvm->DetachCurrentThread();
        }
      }
      listenerGlobal = nullptr;
    }
  }

  void ProcessLocation(double lat, double lon, double speed, double accuracy,
                       const osmscout::Timestamp &timestamp)
  {
    osmscout::GeoCoord coord(lat, lon);
    if (!coord.IsValid()) {
      return;
    }

    auto msg = std::make_shared<osmscout::GPSUpdateMessage>(
        timestamp,
        coord,
        speed,
        accuracy >= 0 ? osmscout::Distance::Of<osmscout::Meter>(accuracy)
                      : osmscout::Distance::Of<osmscout::Meter>(-1));

    {
      std::scoped_lock lock(queueMutex);
      messageQueue.push(msg);
    }
    queueCv.notify_one();
  }

  bool loadRoutableObjects(const osmscout::GeoBox &box,
                           const osmscout::Vehicle &loadVehicle,
                           const std::map<std::string, osmscout::DatabaseId> &databaseMapping,
                           osmscout::RoutableObjectsRef &resultData)
  {
    if (data == nullptr || data->dbThread == nullptr) {
      return false;
    }

    resultData->bbox = box;

    data->dbThread->RunSynchronousJob(
      [&](const std::list<osmscout::DBInstanceRef> &databases) {
        osmscout::Magnification magnification(osmscout::Magnification::magClose);
        for (auto &db : databases) {
          auto database = db->GetDatabase();
          auto dbIdIt = databaseMapping.find(database->GetPath());
          if (dbIdIt == databaseMapping.end()) {
            continue;
          }
          osmscout::DatabaseId databaseId = dbIdIt->second;

          osmscout::MapService::TypeDefinition routableTypes;
          for (const auto &type : database->GetTypeConfig()->GetTypes()) {
            if (type->CanRoute(loadVehicle)) {
              if (type->CanBeArea()) {
                routableTypes.areaTypes.Set(type);
              }
              if (type->CanBeWay()) {
                routableTypes.wayTypes.Set(type);
              }
              if (type->CanBeNode()) {
                routableTypes.nodeTypes.Set(type);
              }
            }
          }

          std::list<osmscout::TileRef> tiles;
          auto mapService = db->GetMapService();
          mapService->LookupTiles(magnification, box, tiles);
          mapService->LoadMissingTileData(osmscout::AreaSearchParameter{},
                                          magnification,
                                          routableTypes,
                                          tiles);

          osmscout::RoutableDBObjects &objects = resultData->dbMap[databaseId];
          objects.typeConfig = database->GetTypeConfig();
          for (const auto &tile : tiles) {
            tile->GetWayData().CopyData([&](const osmscout::WayRef &way) {
              objects.ways[way->GetFileOffset()] = way;
            });
            tile->GetAreaData().CopyData([&](const osmscout::AreaRef &area) {
              objects.areas[area->GetFileOffset()] = area;
            });
          }
        }
      });

    return true;
  }

private:
  void Run()
  {
    JNIEnv *env;
    if (jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr) != JNI_OK) {
      osmscout::log.Error() << "NavigationController: failed to attach thread to JVM";
      return;
    }

    auto startTime = std::chrono::system_clock::now();
    ProcessEngineMessage(std::make_shared<osmscout::InitializeMessage>(startTime));
    ProcessEngineMessage(std::make_shared<osmscout::VoiceSetupMessage>(startTime, osmscout::NavigationVoiceType::VoiceOfMarble, ""));
    ProcessEngineMessage(std::make_shared<osmscout::RouteUpdateMessage>(startTime, routeDescription, vehicle));

    // Use the playback/GPS timeline for ticks. This keeps the engine's "now"
    // close to the last GPS timestamp, so a slow synchronous data load does not
    // mark the GPS fix as Outdated before the PositionAgent can use it.
    auto tickTime = startTime;

    while (running) {
      std::unique_lock lock(queueMutex);
      if (queueCv.wait_for(lock, std::chrono::seconds(1),
                           [&] { return !running || !messageQueue.empty(); })) {
        if (!running) {
          break;
        }
        while (!messageQueue.empty()) {
          auto msg = messageQueue.front();
          messageQueue.pop();
          lock.unlock();
          ProcessEngineMessage(msg);
          if (auto gpsMsg = dynamic_cast<osmscout::GPSUpdateMessage *>(msg.get());
              gpsMsg != nullptr) {
            tickTime = gpsMsg->timestamp;
          }
          lock.lock();
        }
      } else {
        // Timeout: send time tick, advancing one second in playback time
        lock.unlock();
        tickTime += std::chrono::seconds(1);
        auto tick = std::make_shared<osmscout::TimeTickMessage>(tickTime);
        ProcessEngineMessage(tick);
      }
    }

    jvm->DetachCurrentThread();
  }

  void ProcessEngineMessage(const osmscout::NavigationMessageRef &msg)
  {
    auto output = engine.Process(msg);
    for (const auto &message : output) {
      DispatchMessage(message);
    }
  }

  void DispatchMessage(const osmscout::NavigationMessageRef &message)
  {
    JNIEnv *env;
    if (jvm->AttachCurrentThread(reinterpret_cast<void **>(&env), nullptr) != JNI_OK) {
      return;
    }

    if (auto positionMessage = dynamic_cast<osmscout::PositionAgent::PositionMessage *>(message.get());
        positionMessage != nullptr) {
      DispatchPositionEstimate(env, positionMessage);
    }
    else if (auto bearingMessage = dynamic_cast<osmscout::BearingChangedMessage *>(message.get());
             bearingMessage != nullptr) {
      lastBearing = bearingMessage->bearing;
    }
    else if (auto targetMessage = dynamic_cast<osmscout::TargetReachedMessage *>(message.get());
             targetMessage != nullptr) {
      env->CallVoidMethod(listenerGlobal, methods.onTargetReached,
                          targetMessage->targetBearing.AsDegrees(),
                          targetMessage->targetDistance.AsMeter());
    }
    else if (auto rerouteMessage = dynamic_cast<osmscout::RerouteRequestMessage *>(message.get());
             rerouteMessage != nullptr) {
      env->CallVoidMethod(listenerGlobal, methods.onRerouteRequest,
                          rerouteMessage->from.GetLat(),
                          rerouteMessage->from.GetLon(),
                          rerouteMessage->initialBearing
                              ? rerouteMessage->initialBearing->AsDegrees()
                              : std::numeric_limits<double>::quiet_NaN(),
                          routeDescription->Nodes().back().GetLocation().GetLat(),
                          routeDescription->Nodes().back().GetLocation().GetLon());
    }
    else if (auto arrivalMessage = dynamic_cast<osmscout::ArrivalEstimateMessage *>(message.get());
             arrivalMessage != nullptr) {
      using namespace std::chrono;
      env->CallVoidMethod(listenerGlobal, methods.onArrivalEstimate,
                          duration_cast<milliseconds>(arrivalMessage->arrivalEstimate.time_since_epoch()).count(),
                          arrivalMessage->remainingDistance.AsMeter());
    }
    else if (auto currentSpeedMessage = dynamic_cast<osmscout::CurrentSpeedMessage *>(message.get());
             currentSpeedMessage != nullptr) {
      env->CallVoidMethod(listenerGlobal, methods.onCurrentSpeed, currentSpeedMessage->speed);
    }
    else if (auto maxSpeedMessage = dynamic_cast<osmscout::MaxAllowedSpeedMessage *>(message.get());
             maxSpeedMessage != nullptr) {
      env->CallVoidMethod(listenerGlobal, methods.onMaxAllowedSpeed,
                          maxSpeedMessage->defined ? maxSpeedMessage->maxAllowedSpeed : -1.0);
    }
    else if (auto laneMessage = dynamic_cast<osmscout::LaneAgent::LaneMessage *>(message.get());
             laneMessage != nullptr) {
      jstring turnStr = env->NewStringUTF(LaneTurnToString(laneMessage->lane.turn).c_str());

      // Build LaneTurn[] array from per-lane turns vector
      const auto &turns = laneMessage->lane.turns;
      jsize turnCount = static_cast<jsize>(turns.size());
      jobjectArray turnsArray = env->NewObjectArray(
          turnCount,
          static_cast<jclass>(methods.laneTurnClsGlobal),
          nullptr);
      for (jsize i = 0; i < turnCount; i++) {
        jobject turnObj = env->CallStaticObjectMethod(
            static_cast<jclass>(methods.laneTurnClsGlobal),
            methods.laneTurnFromId,
            static_cast<jint>(turns[i]));
        env->SetObjectArrayElement(turnsArray, i, turnObj);
        env->DeleteLocalRef(turnObj);
      }

      env->CallVoidMethod(listenerGlobal, methods.onLaneUpdate,
                          laneMessage->lane.oneway ? JNI_TRUE : JNI_FALSE,
                          static_cast<jint>(laneMessage->lane.count),
                          laneMessage->lane.suggested ? JNI_TRUE : JNI_FALSE,
                          static_cast<jint>(laneMessage->lane.suggestedFrom),
                          static_cast<jint>(laneMessage->lane.suggestedTo),
                          turnStr,
                          turnsArray);
      env->DeleteLocalRef(turnStr);
      env->DeleteLocalRef(turnsArray);
    }
    else if (auto voiceMessage = dynamic_cast<osmscout::SampleVoiceInstructionMessage *>(message.get());
             voiceMessage != nullptr) {
      jsize count = static_cast<jsize>(voiceMessage->message.size());
      jintArray samples = env->NewIntArray(count);
      if (samples != nullptr) {
        std::vector<jint> values(count);
        for (jsize i = 0; i < count; i++) {
          values[i] = static_cast<jint>(voiceMessage->message[i]);
        }
        env->SetIntArrayRegion(samples, 0, count, values.data());
        env->CallVoidMethod(listenerGlobal, methods.onVoiceInstruction, samples);
        env->DeleteLocalRef(samples);
      }
    }
    else if (auto routeInstrMsg = dynamic_cast<osmscout::RouteInstructionsMessage<JavaRouteInstruction> *>(message.get());
             routeInstrMsg != nullptr) {
      DispatchRouteInstructions(env, routeInstrMsg->instructions);
    }
    else if (auto nextInstrMsg = dynamic_cast<osmscout::NextRouteInstructionsMessage<JavaRouteInstruction> *>(message.get());
             nextInstrMsg != nullptr) {
      DispatchNextRouteInstruction(env, nextInstrMsg->nextRouteInstruction);
    }
  }

  void DispatchPositionEstimate(JNIEnv *env,
                                osmscout::PositionAgent::PositionMessage *positionMessage)
  {
    if (!methods.positionCtor || !methods.stateValueOf ||
        !methods.positionClsGlobal || !methods.stateClsGlobal) {
      return;
    }

    jstring stateName = env->NewStringUTF(PositionStateToString(positionMessage->position.state).c_str());
    jobject stateObj = env->CallStaticObjectMethod(
        static_cast<jclass>(methods.stateClsGlobal), methods.stateValueOf, stateName);
    env->DeleteLocalRef(stateName);

    double bearing = std::numeric_limits<double>::quiet_NaN();
    if (lastBearing.has_value()) {
      bearing = lastBearing->AsDegrees();
    }

    jobject positionObj = env->NewObject(
        static_cast<jclass>(methods.positionClsGlobal), methods.positionCtor,
        stateObj,
        positionMessage->position.coord.GetLat(),
        positionMessage->position.coord.GetLon(),
        bearing,
        -1.0);

    env->CallVoidMethod(listenerGlobal, methods.onPositionEstimate, positionObj);
    env->DeleteLocalRef(positionObj);
    env->DeleteLocalRef(stateObj);
  }

  jobject CreateJavaRouteInstruction(JNIEnv *env, const JavaRouteInstruction &instr)
  {
    if (!methods.instructionCtor || !methods.turnTypeValueOf ||
        !methods.instructionClsGlobal || !methods.turnTypeClsGlobal) {
      return nullptr;
    }

    // Convert turn type string to TurnType enum
    jstring turnTypeStr = env->NewStringUTF(instr.turnType.c_str());
    jobject turnTypeObj = env->CallStaticObjectMethod(
        static_cast<jclass>(methods.turnTypeClsGlobal), methods.turnTypeValueOf, turnTypeStr);
    env->DeleteLocalRef(turnTypeStr);

    jstring streetNameJ = env->NewStringUTF(instr.streetName.c_str());
    jstring descriptionJ = env->NewStringUTF(instr.description.c_str());
    jstring shortDescJ = env->NewStringUTF(instr.shortDescription.c_str());

    // "Next next" hint
    jstring nnTurnTypeStr = env->NewStringUTF(instr.nextNextTurnType.c_str());
    jobject nnTurnTypeObj = env->CallStaticObjectMethod(
        static_cast<jclass>(methods.turnTypeClsGlobal), methods.turnTypeValueOf, nnTurnTypeStr);
    env->DeleteLocalRef(nnTurnTypeStr);

    jstring nnDescJ = env->NewStringUTF(instr.nextNextDescription.c_str());
    jstring nnShortDescJ = env->NewStringUTF(instr.nextNextShortDescription.c_str());

    jobject instrObj = env->NewObject(
        static_cast<jclass>(methods.instructionClsGlobal), methods.instructionCtor,
        instr.distanceTo,
        turnTypeObj,
        streetNameJ,
        descriptionJ,
        shortDescJ,
        instr.nextNextDistanceTo,
        nnTurnTypeObj,
        nnDescJ,
        nnShortDescJ);

    env->DeleteLocalRef(turnTypeObj);
    env->DeleteLocalRef(streetNameJ);
    env->DeleteLocalRef(descriptionJ);
    env->DeleteLocalRef(shortDescJ);
    env->DeleteLocalRef(nnTurnTypeObj);
    env->DeleteLocalRef(nnDescJ);
    env->DeleteLocalRef(nnShortDescJ);

    return instrObj;
  }

  void DispatchRouteInstructions(JNIEnv *env,
                                 const std::list<JavaRouteInstruction> &instructions)
  {
    if (!methods.onRouteInstructions) return;

    jsize count = static_cast<jsize>(instructions.size());
    jobjectArray arr = env->NewObjectArray(
        count,
        static_cast<jclass>(methods.instructionClsGlobal),
        nullptr);

    jsize idx = 0;
    for (const auto &instr : instructions) {
      jobject instrObj = CreateJavaRouteInstruction(env, instr);
      if (instrObj) {
        env->SetObjectArrayElement(arr, idx++, instrObj);
        env->DeleteLocalRef(instrObj);
      }
    }

    env->CallVoidMethod(listenerGlobal, methods.onRouteInstructions, arr);
    env->DeleteLocalRef(arr);
  }

  void DispatchNextRouteInstruction(JNIEnv *env,
                                    const JavaRouteInstruction &instruction)
  {
    if (!methods.onNextRouteInstruction) return;

    jobject instrObj = CreateJavaRouteInstruction(env, instruction);
    if (instrObj) {
      env->CallVoidMethod(listenerGlobal, methods.onNextRouteInstruction, instrObj);
      env->DeleteLocalRef(instrObj);
    }
  }

private:
  ClientData *data;
  osmscout::RouteDescriptionRef routeDescription;
  osmscout::Vehicle vehicle;
  JavaVM *jvm;
  jobject listenerGlobal;
  NavigationListenerMethods methods;
  osmscout::NavigationEngine engine;

  std::thread thread;
  std::mutex threadMutex;

  std::mutex queueMutex;
  std::condition_variable queueCv;
  std::queue<osmscout::NavigationMessageRef> messageQueue;
  bool running{false};

  std::optional<osmscout::Bearing> lastBearing;
};

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_searchLocations(JNIEnv *env, jobject self,
                                                                    jstring queryJStr,
                                                                    jint limit,
                                                                    jstring defaultRegionJStr,
                                                                    jboolean cancel)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    return nullptr;
  }

  const char *queryCStr = env->GetStringUTFChars(queryJStr, nullptr);
  if (queryCStr == nullptr) {
    return nullptr;
  }
  std::string query(queryCStr);
  env->ReleaseStringUTFChars(queryJStr, queryCStr);

  if (query.empty()) {
    // Return empty array
    jclass entryCls = env->FindClass("com/framstag/libosmscout/client/LocationEntry");
    return env->NewObjectArray(0, entryCls, nullptr);
  }

  // A query that is a coordinate pair (e.g. "51.5, 7.4") yields a coordinate
  // result that ranks first, matching OSMScout2.
  double coordLat = 0.0;
  double coordLon = 0.0;
  const bool hasCoordinate = ParseCoordinate(query, coordLat, coordLon);

  // Optional admin region context (e.g. current map region)
  std::string defaultRegion;
  if (defaultRegionJStr != nullptr) {
    const char *regionCStr = env->GetStringUTFChars(defaultRegionJStr, nullptr);
    if (regionCStr != nullptr) {
      defaultRegion = regionCStr;
      env->ReleaseStringUTFChars(defaultRegionJStr, regionCStr);
    }
  }

  // A new query cancels the previously running search; cancel=true requests
  // explicit cancellation of the current search.
  {
    std::lock_guard<std::mutex> guard(g_searchMutex);
    if (g_currentBreaker) {
      g_currentBreaker->Break();
      g_currentBreaker = nullptr;
    }
    if (!cancel) {
      g_currentBreaker = std::make_shared<osmscout::ThreadedBreaker>();
    }
  }

  std::vector<osmscout::LocationSearchResult::Entry> results;
#ifdef OSMSCOUT_HAVE_LIB_MARISA
  std::vector<FreeTextEntry> freeTextEntries;
  std::set<osmscout::FileOffset> seenOffsets;
#endif
  bool limitReached = false;

  data->dbThread->RunSynchronousJob(
    [&](const std::list<osmscout::DBInstanceRef> &databases) {
      osmscout::BreakerRef breaker;
      {
        std::lock_guard<std::mutex> guard(g_searchMutex);
        breaker = g_currentBreaker;
      }

      const auto limitReachedTotal = [&]() {
#ifdef OSMSCOUT_HAVE_LIB_MARISA
        return results.size() + freeTextEntries.size() >= static_cast<size_t>(limit);
#else
        return results.size() >= static_cast<size_t>(limit);
#endif
      };

      for (const auto &db : databases) {
        if (breaker && breaker->IsAborted()) {
          break;
        }
        // The basemap is a low-zoom background map; it is not searched (its
        // index files may be absent or incomplete).
        if (IsBasemapDatabase(db)) {
          continue;
        }

        osmscout::LocationServiceRef locationService = db->GetLocationService();
        if (!locationService) {
          continue;
        }

        osmscout::LocationStringSearchParameter param(query);
        param.SetLimit(static_cast<size_t>(limit));
        param.SetStringMatcherFactory(
            std::make_shared<osmscout::StringMatcherTransliterateFactory>());
        if (breaker) {
          param.SetBreaker(breaker);
        }

        // Resolve the default admin region by name and scope the search to it
        if (!defaultRegion.empty()) {
          osmscout::LocationStringSearchParameter regionParam(defaultRegion);
          regionParam.SetLimit(1);
          regionParam.SetAdminRegionOnlyMatch(true);
          regionParam.SetStringMatcherFactory(
              std::make_shared<osmscout::StringMatcherTransliterateFactory>());
          osmscout::LocationSearchResult regionResult;
          if (locationService->SearchForLocationByString(regionParam, regionResult) &&
              !regionResult.results.empty() &&
              regionResult.results.front().adminRegion) {
            param.SetDefaultAdminRegion(regionResult.results.front().adminRegion);
          }
        }

        osmscout::LocationSearchResult searchResult;
        if (locationService->SearchForLocationByString(param, searchResult)) {
          for (const auto &entry : searchResult.results) {
            results.push_back(entry);
          }
          if (searchResult.limitReached) {
            limitReached = true;
          }
        }

#ifdef OSMSCOUT_HAVE_LIB_MARISA
        // Free-text search over the text index (optional; index may be missing)
        osmscout::TextSearchIndex textSearch;
        if (!textSearch.Load(db->path)) {
          if (IsBasemapDatabase(db)) {
            // just debug, basemap may omit text indexes to save space
            osmscout::log.Debug() << "Failed to load text index files (basemap) " << db->path;
          } else {
            osmscout::log.Warn() << "Failed to load text index files, search only for locations with db " << db->path;
          }
        } else {
          osmscout::TextSearchIndex::ResultsMap resultsTxt;
          textSearch.Search(query,
                            /*searchPOIs*/ true, /*searchLocations*/ true,
                            /*searchRegions*/ true, /*searchOther*/ true,
                            /*transliterate*/ true,
                            resultsTxt);
          for (const auto &e : resultsTxt) {
            if (limitReachedTotal()) {
              break;
            }
            for (const auto &fref : e.second) {
              if (limitReachedTotal()) {
                break;
              }
              if (seenOffsets.count(fref.GetFileOffset()) != 0) {
                continue;
              }
              seenOffsets.insert(fref.GetFileOffset());
              FreeTextEntry entry;
              if (BuildFreeTextEntry(db, fref, e.first, entry)) {
                freeTextEntries.push_back(entry);
              }
            }
          }
        }
#endif

        if (limitReachedTotal()) {
          break;
        }
      }
    }
  );

#ifdef OSMSCOUT_HAVE_LIB_MARISA
  // Record offsets of structured results so free-text hits of the same object
  // are not returned twice.
  for (const auto &entry : results) {
    osmscout::ObjectFileRef ref;
    if (entry.address) {
      ref = entry.address->object;
    } else if (entry.poi) {
      ref = entry.poi->object;
    } else if (entry.location && !entry.location->objects.empty()) {
      ref = entry.location->objects.front();
    } else if (entry.adminRegion) {
      ref = entry.adminRegion->object;
    }
    if (ref.Valid()) {
      seenOffsets.insert(ref.GetFileOffset());
    }
  }
  // Drop free-text hits that duplicate structured results.
  freeTextEntries.erase(
      std::remove_if(freeTextEntries.begin(), freeTextEntries.end(),
                     [&](const FreeTextEntry &e) {
                       return seenOffsets.count(static_cast<osmscout::FileOffset>(e.objectFileOffset)) != 0;
                     }),
      freeTextEntries.end());
#endif

  // Truncate to limit: structured results first, free-text fills the rest
  if (results.size() >= static_cast<size_t>(limit)) {
    results.resize(static_cast<size_t>(limit));
#ifdef OSMSCOUT_HAVE_LIB_MARISA
    freeTextEntries.clear();
#endif
  } else {
#ifdef OSMSCOUT_HAVE_LIB_MARISA
    freeTextEntries.resize(static_cast<size_t>(limit) - results.size());
#endif
  }

  // Build Java LocationEntry[]
  jclass entryCls = env->FindClass("com/framstag/libosmscout/client/LocationEntry");
  if (entryCls == nullptr) {
    return nullptr;
  }

  jmethodID entryCtor = env->GetMethodID(entryCls, "<init>", "()V");
  if (entryCtor == nullptr) {
    return nullptr;
  }

  jfieldID labelField = env->GetFieldID(entryCls, "label", "Ljava/lang/String;");
  jfieldID typeField = env->GetFieldID(entryCls, "type", "Ljava/lang/String;");
  jfieldID objectTypeField = env->GetFieldID(entryCls, "objectType", "Ljava/lang/String;");
  jfieldID latField = env->GetFieldID(entryCls, "lat", "D");
  jfieldID lonField = env->GetFieldID(entryCls, "lon", "D");
  jfieldID regionField = env->GetFieldID(entryCls, "region", "[Ljava/lang/String;");
  jfieldID postalAreaField = env->GetFieldID(entryCls, "postalArea", "Ljava/lang/String;");
  jfieldID adminRegionHierarchyField = env->GetFieldID(entryCls, "adminRegionHierarchy", "Ljava/lang/String;");
  jfieldID objectTypeNameField = env->GetFieldID(entryCls, "objectTypeName", "Ljava/lang/String;");
  jfieldID objectFileOffsetField = env->GetFieldID(entryCls, "objectFileOffset", "J");
  jfieldID matchQualityField = env->GetFieldID(entryCls, "matchQuality", "Ljava/lang/String;");
  jfieldID refTypeField = env->GetFieldID(entryCls, "refType", "Ljava/lang/String;");

  jsize count = static_cast<jsize>((hasCoordinate ? 1 : 0) + results.size()
#ifdef OSMSCOUT_HAVE_LIB_MARISA
                                  + freeTextEntries.size()
#endif
                                  );
  jobjectArray resultArray = env->NewObjectArray(count, entryCls, nullptr);
  if (resultArray == nullptr) {
    return nullptr;
  }

  jsize idx = 0;

  // Coordinate result first (ranks above all object results, matching OSMScout2)
  if (hasCoordinate) {
    jobject jEntry = env->NewObject(entryCls, entryCtor);
    env->SetObjectField(jEntry, labelField, env->NewStringUTF(query.c_str()));
    env->SetObjectField(jEntry, typeField, env->NewStringUTF("coordinate"));
    env->SetDoubleField(jEntry, latField, coordLat);
    env->SetDoubleField(jEntry, lonField, coordLon);
    env->SetObjectField(jEntry, matchQualityField, env->NewStringUTF("match"));
    env->SetObjectField(jEntry, regionField,
                        env->NewObjectArray(0, env->FindClass("java/lang/String"), nullptr));
    env->SetObjectArrayElement(resultArray, idx++, jEntry);
    env->DeleteLocalRef(jEntry);
  }

  // Serialize structured results; free-text hits follow (see loop below)
  const jsize structuredCount = static_cast<jsize>(results.size());
  for (jsize i = 0; i < structuredCount; i++) {
    const auto &entry = results[static_cast<size_t>(i)];
    jobject jEntry = env->NewObject(entryCls, entryCtor);

    // label
    std::string label;
    if (entry.adminRegion && entry.location && entry.address) {
      label = entry.location->name + " " + entry.address->name;
    } else if (entry.location) {
      label = entry.location->name;
    } else if (entry.poi) {
      label = entry.poi->name;
    } else if (entry.adminRegion) {
      label = entry.adminRegion->name;
    } else if (entry.address) {
      label = entry.address->name;
    }
    env->SetObjectField(jEntry, labelField, env->NewStringUTF(label.c_str()));

    // type
    std::string type = "object";
    env->SetObjectField(jEntry, typeField, env->NewStringUTF(type.c_str()));

    // objectType
    std::string objectType;
    if (entry.adminRegion) {
      objectType = "boundary_administrative";
    } else if (entry.poi) {
      objectType = "poi";
    } else if (entry.location) {
      objectType = "place";
    } else if (entry.address) {
      objectType = "address";
    }
    env->SetObjectField(jEntry, objectTypeField, env->NewStringUTF(objectType.c_str()));

    // lat, lon — extract from ObjectFileRef via database lookup
    double lat = 0.0, lon = 0.0;
    std::string objectTypeName;
    long long objectFileOffset = 0;
    osmscout::ObjectFileRef objRef;
    if (entry.address) {
      objRef = entry.address->object;
    } else if (entry.poi) {
      objRef = entry.poi->object;
    } else if (entry.location && !entry.location->objects.empty()) {
      objRef = entry.location->objects.front();
    } else if (entry.adminRegion) {
      objRef = entry.adminRegion->object;
    }

    if (objRef.Valid()) {
      objectFileOffset = static_cast<long long>(objRef.GetFileOffset());
      data->dbThread->RunSynchronousJob(
        [&](const std::list<osmscout::DBInstanceRef> &databases) {
          for (const auto &db : databases) {
            if (IsBasemapDatabase(db)) {
              continue;
            }
            auto database = db->GetDatabase();
            if (objRef.GetType() == osmscout::RefType::refNode) {
              osmscout::NodeRef node;
              if (database->GetNodeByOffset(objRef.GetFileOffset(), node)) {
                lat = node->GetCoords().GetLat();
                lon = node->GetCoords().GetLon();
                objectTypeName = node->GetType()->GetName();
              }
            } else if (objRef.GetType() == osmscout::RefType::refArea) {
              osmscout::AreaRef area;
              if (database->GetAreaByOffset(objRef.GetFileOffset(), area)) {
                lat = area->GetBoundingBox().GetCenter().GetLat();
                lon = area->GetBoundingBox().GetCenter().GetLon();
                objectTypeName = area->GetType()->GetName();
              }
            } else if (objRef.GetType() == osmscout::RefType::refWay) {
              osmscout::WayRef way;
              if (database->GetWayByOffset(objRef.GetFileOffset(), way)) {
                lat = way->GetBoundingBox().GetCenter().GetLat();
                lon = way->GetBoundingBox().GetCenter().GetLon();
                objectTypeName = way->GetType()->GetName();
              }
            }
          }
        }
      );
    }
    env->SetDoubleField(jEntry, latField, lat);
    env->SetDoubleField(jEntry, lonField, lon);
    env->SetObjectField(jEntry, objectTypeNameField, env->NewStringUTF(objectTypeName.c_str()));
    env->SetLongField(jEntry, objectFileOffsetField, objectFileOffset);

    // refType
    std::string refTypeStr;
    if (objRef.GetType() == osmscout::RefType::refNode) {
      refTypeStr = "node";
    } else if (objRef.GetType() == osmscout::RefType::refWay) {
      refTypeStr = "way";
    } else if (objRef.GetType() == osmscout::RefType::refArea) {
      refTypeStr = "area";
    }
    if (!refTypeStr.empty()) {
      env->SetObjectField(jEntry, refTypeField, env->NewStringUTF(refTypeStr.c_str()));
    }

    // match quality
    std::string matchQuality = "candidate";
    if ((entry.location && entry.locationMatchQuality == osmscout::LocationSearchResult::match) ||
        (entry.address && entry.addressMatchQuality == osmscout::LocationSearchResult::match) ||
        (entry.adminRegion && entry.adminRegionMatchQuality == osmscout::LocationSearchResult::match) ||
        (entry.poi && entry.poiMatchQuality == osmscout::LocationSearchResult::match)) {
      matchQuality = "match";
    }
    env->SetObjectField(jEntry, matchQualityField, env->NewStringUTF(matchQuality.c_str()));

    // region (admin region hierarchy) + postal area + admin region hierarchy path
    std::vector<std::string> regionParts;
    if (entry.adminRegion) {
      regionParts.push_back(entry.adminRegion->name);
    }
    if (entry.postalArea) {
      regionParts.push_back(entry.postalArea->name);
    }

    jobjectArray regionArray = env->NewObjectArray(
        static_cast<jsize>(regionParts.size()),
        env->FindClass("java/lang/String"),
        nullptr);
    for (jsize r = 0; r < static_cast<jsize>(regionParts.size()); r++) {
      env->SetObjectArrayElement(regionArray, r,
                                 env->NewStringUTF(regionParts[static_cast<size_t>(r)].c_str()));
    }
    env->SetObjectField(jEntry, regionField, regionArray);

    // postal area
    if (entry.postalArea) {
      env->SetObjectField(jEntry, postalAreaField, env->NewStringUTF(entry.postalArea->name.c_str()));
    }

    // admin region hierarchy (full path)
    std::string hierarchyPath;
    if (entry.adminRegion) {
      std::map<osmscout::FileOffset, osmscout::AdminRegionRef> adminRegionMap;
      data->dbThread->RunSynchronousJob(
        [&](const std::list<osmscout::DBInstanceRef> &databases) {
          for (const auto &db : databases) {
            if (IsBasemapDatabase(db)) {
              continue;
            }
            auto locationService = db->GetLocationService();
            if (locationService) {
              locationService->ResolveAdminRegionHierachie(entry.adminRegion, adminRegionMap);
            }
          }
        }
      );
      hierarchyPath = entry.adminRegion->name;
      osmscout::FileOffset parentOffset = entry.adminRegion->parentRegionOffset;
      while (parentOffset != 0) {
        auto it = adminRegionMap.find(parentOffset);
        if (it == adminRegionMap.end()) break;
        hierarchyPath += "/" + it->second->name;
        parentOffset = it->second->parentRegionOffset;
      }
    }
    env->SetObjectField(jEntry, adminRegionHierarchyField, env->NewStringUTF(hierarchyPath.c_str()));

    env->SetObjectArrayElement(resultArray, idx++, jEntry);
    env->DeleteLocalRef(jEntry);
  }

#ifdef OSMSCOUT_HAVE_LIB_MARISA
  // Free-text search hits (POIs, named objects via the text index)
  for (jsize i = 0; i < static_cast<jsize>(freeTextEntries.size()); i++) {
    const auto &entry = freeTextEntries[static_cast<size_t>(i)];
    jobject jEntry = env->NewObject(entryCls, entryCtor);

    env->SetObjectField(jEntry, labelField, env->NewStringUTF(entry.label.c_str()));
    env->SetObjectField(jEntry, typeField, env->NewStringUTF("object"));
    env->SetObjectField(jEntry, objectTypeField, env->NewStringUTF(entry.objectType.c_str()));
    env->SetDoubleField(jEntry, latField, entry.lat);
    env->SetDoubleField(jEntry, lonField, entry.lon);
    env->SetObjectField(jEntry, objectTypeNameField, env->NewStringUTF(entry.objectTypeName.c_str()));
    env->SetLongField(jEntry, objectFileOffsetField, entry.objectFileOffset);
    env->SetObjectField(jEntry, matchQualityField, env->NewStringUTF("match"));
    if (!entry.refType.empty()) {
      env->SetObjectField(jEntry, refTypeField, env->NewStringUTF(entry.refType.c_str()));
    }
    // No region/postal area information for free-text hits
    env->SetObjectField(jEntry, regionField,
                        env->NewObjectArray(0, env->FindClass("java/lang/String"), nullptr));

    env->SetObjectArrayElement(resultArray, idx++, jEntry);
    env->DeleteLocalRef(jEntry);
  }
#endif

  return resultArray;
}

// --------------------------------------------------------------------------
// OSMScoutClient::cancelSearch()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_cancelSearch(JNIEnv */*env*/, jobject /*self*/)
{
  std::lock_guard<std::mutex> guard(g_searchMutex);
  if (g_currentBreaker) {
    g_currentBreaker->Break();
    g_currentBreaker = nullptr;
  }
}

// --------------------------------------------------------------------------
// OSMScoutClient::getRegion(double lat, double lon)
// --------------------------------------------------------------------------
// Reverse lookup of the admin region containing the given coordinate. Used to
// scope location searches to the current map region (matching OSMScout2).

extern "C" JNIEXPORT jstring JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_getRegion(JNIEnv *env, jobject self,
                                                              jdouble lat, jdouble lon)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    return nullptr;
  }

  std::string regionName;
  data->dbThread->RunSynchronousJob(
    [&](const std::list<osmscout::DBInstanceRef> &databases) {
      osmscout::GeoCoord coord(lat, lon);
      for (const auto &db : databases) {
        if (IsBasemapDatabase(db)) {
          continue;
        }
        auto descriptionService = db->GetLocationDescriptionService();
        if (!descriptionService) {
          continue;
        }
        std::list<osmscout::LocationDescriptionService::ReverseLookupResult> result;
        if (descriptionService->ReverseLookupRegion(coord, result)) {
          for (const auto &entry : result) {
            if (entry.adminRegion) {
              regionName = entry.adminRegion->name;
              return;
            }
          }
        }
      }
    }
  );

  if (regionName.empty()) {
    return nullptr;
  }
  return env->NewStringUTF(regionName.c_str());
}

// --------------------------------------------------------------------------
// OSMScoutClient::getDescription(double lat, double lon) and
// OSMScoutClient::getDescriptionCandidates(double lat, double lon)
// --------------------------------------------------------------------------

namespace {

// Maximum number of candidate objects returned for a long-press lookup
const size_t MAX_DESCRIPTION_CANDIDATES = 10;

// One ranked candidate object found at a coordinate, with its description
// and identity (ref type, OSM type name, file offset).
struct RankedDescriptionCandidate {
  osmscout::ObjectDescription description;
  std::string refType;   // "area", "way" or "node"
  std::string typeName;  // e.g. "building", "highway_residential"
  int64_t fileOffset;    // file offset of the object in the database
};

// Create an empty Java ObjectDescription (zero entries, no identity)
jobject NewEmptyObjectDescription(JNIEnv *env)
{
  jclass descCls = env->FindClass("com/framstag/libosmscout/client/ObjectDescription");
  jmethodID descCtor = env->GetMethodID(descCls, "<init>", "(Ljava/util/List;)V");
  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  jobject emptyList = env->NewObject(arrayListCls, arrayListCtor);
  return env->NewObject(descCls, descCtor, emptyList);
}

// Create an empty Java list
jobject NewEmptyJavaList(JNIEnv *env)
{
  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  return env->NewObject(arrayListCls, arrayListCtor);
}

// Marshal an ObjectDescription with its identity into a Java ObjectDescription
jobject MarshalObjectDescription(JNIEnv *env,
                                 const osmscout::ObjectDescription &description,
                                 const std::string &refType,
                                 const std::string &typeName,
                                 int64_t fileOffset)
{
  jclass descCls = env->FindClass("com/framstag/libosmscout/client/ObjectDescription");
  jmethodID descCtor = env->GetMethodID(descCls, "<init>",
    "(Ljava/util/List;DDLjava/lang/String;Ljava/lang/String;J)V");

  jclass entryCls = env->FindClass("com/framstag/libosmscout/client/DescriptionEntry");
  jmethodID entryCtor = env->GetMethodID(entryCls, "<init>", "()V");

  jfieldID sectionKeyField = env->GetFieldID(entryCls, "sectionKey", "Ljava/lang/String;");
  jfieldID subsectionKeyField = env->GetFieldID(entryCls, "subsectionKey", "Ljava/lang/String;");
  jfieldID hasIndexField = env->GetFieldID(entryCls, "hasIndex", "Z");
  jfieldID indexField = env->GetFieldID(entryCls, "index", "I");
  jfieldID labelKeyField = env->GetFieldID(entryCls, "labelKey", "Ljava/lang/String;");
  jfieldID valueField = env->GetFieldID(entryCls, "value", "Ljava/lang/String;");

  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  jmethodID arrayListAdd = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");

  jobject entryList = env->NewObject(arrayListCls, arrayListCtor);

  for (const auto &entry : description.GetEntries()) {
    jobject jEntry = env->NewObject(entryCls, entryCtor);

    env->SetObjectField(jEntry, sectionKeyField,
                        env->NewStringUTF(entry.GetSectionKey().c_str()));

    env->SetObjectField(jEntry, subsectionKeyField,
                        env->NewStringUTF(entry.GetSubsectionKey().c_str()));

    env->SetBooleanField(jEntry, hasIndexField,
                         static_cast<jboolean>(entry.HasIndex()));

    env->SetIntField(jEntry, indexField,
                     static_cast<jint>(entry.GetIndex()));

    env->SetObjectField(jEntry, labelKeyField,
                        env->NewStringUTF(entry.GetLabelKey().c_str()));

    env->SetObjectField(jEntry, valueField,
                        env->NewStringUTF(entry.GetValue().c_str()));

    env->CallBooleanMethod(entryList, arrayListAdd, jEntry);
    env->DeleteLocalRef(jEntry);
  }

  double nan = std::numeric_limits<double>::quiet_NaN();
  return env->NewObject(descCls, descCtor, entryList,
                        static_cast<jdouble>(nan),
                        static_cast<jdouble>(nan),
                        env->NewStringUTF(refType.c_str()),
                        env->NewStringUTF(typeName.c_str()),
                        static_cast<jlong>(fileOffset));
}

// Collect all reasonable objects with description data at the given coordinate.
// Candidates are ranked by (1) has description data, (2) visible at the given
// magnification (areas smaller than ~1px on screen are not visible), (3) way/node
// very close to the coordinate, (4) small area containing the coordinate,
// (5) type rank (area < way < node), (6) proximity. When several databases cover
// the coordinate, candidates from a coarser database (whose bounding box contains
// a finer database's box, e.g. a world basemap) are dropped in favor of the
// finer map. The result is capped at MAX_DESCRIPTION_CANDIDATES and only
// contains candidates with description data.
std::vector<RankedDescriptionCandidate> CollectDescriptionCandidates(ClientData *data,
                                                                     double lat, double lon,
                                                                     int magnification)
{
  std::vector<RankedDescriptionCandidate> result;

  osmscout::GeoCoord coord(lat, lon);

  // Candidates per database, so overlapping databases (e.g. a world basemap
  // plus a regional map) can be merged with the finer map preferred.
  struct DbCandidates {
    osmscout::GeoBox box;
    std::vector<RankedDescriptionCandidate> items;
  };
  std::vector<DbCandidates> perDb;

  data->dbThread->RunSynchronousJob(
    [&](const std::list<osmscout::DBInstanceRef> &databases) {
      osmscout::log.Debug() << "[JNI] RunSynchronousJob callback: " << databases.size() << " databases";
      for (const auto &db : databases) {
        osmscout::GeoBox dbBox = db->GetDBGeoBox();
        osmscout::log.Debug() << "[JNI] checking db box: " << dbBox.GetDisplayText();
        if (!dbBox.Includes(coord)) {
          osmscout::log.Debug() << "[JNI] coord not in db box, skipping";
          continue;
        }

        auto database = db->GetDatabase();
        osmscout::TypeConfigRef typeConfig = database->GetTypeConfig();
        if (!typeConfig) {
          osmscout::log.Warn() << "[JNI] no typeConfig for database";
          continue;
        }

        // Use type-specific sets for each lookup
        osmscout::TypeInfoSet nodeTypes(typeConfig->GetNodeTypes());
        osmscout::TypeInfoSet wayTypes(typeConfig->GetWayTypes());
        osmscout::TypeInfoSet areaTypes(typeConfig->GetAreaTypes());
        osmscout::Distance radius = osmscout::Distance::Of<osmscout::Meter>(50);

        // Collect candidates with ranking metadata
        struct Candidate {
          osmscout::FeatureValueBuffer buffer;
          osmscout::Distance distance;
          int typeRank;     // 0=area, 1=way, 2=node (lower = better)
          bool contains;    // true if area contains the click point
          double areaSize;  // bounding box area in sq meters (0 for ways/nodes)
          osmscout::ObjectDescription description;
          bool hasData;             // description service produced entries
          bool veryClose;           // way/node within VERY_CLOSE meters
          bool effectiveContains;   // small area containing the point
          bool visible;             // object large enough to be visible at the zoom
          std::string refType;      // "area", "way" or "node"
          std::string typeName;     // OSM type name of the object
          int64_t fileOffset;       // file offset of the object in the database
        };
        std::vector<Candidate> candidates;

        // Query areas first (most important)
        try {
          auto areaResults = database->LoadAreasInRadius(coord, areaTypes, radius);
          for (const auto &entry : areaResults.GetAreaResults()) {
            const auto &area = entry.GetArea();
            osmscout::GeoBox box = area->GetBoundingBox();
            // Convert the bounding box from degrees to square meters at the
            // box center (degrees are not a uniform metric).
            const double metersPerDegLat = 111320.0;
            const double metersPerDegLon =
                metersPerDegLat * std::cos(box.GetCenter().GetLat() * M_PI / 180.0);
            double size = box.GetWidth() * metersPerDegLon
                          * box.GetHeight() * metersPerDegLat;
            candidates.push_back({area->GetFeatureValueBuffer(),
                                  entry.GetDistance(), 0, entry.IsInArea(), size,
                                  osmscout::ObjectDescription(), false, false, false, false,
                                  "area",
                                  area->GetFeatureValueBuffer().GetType()->GetName(),
                                  static_cast<int64_t>(area->GetFileOffset())});
          }
        } catch (const std::exception &e) {
          osmscout::log.Warn() << "[JNI] LoadAreasInRadius exception: " << e.what();
        }

        // Query ways
        try {
          auto wayResults = database->LoadWaysInRadius(coord, wayTypes, radius);
          for (const auto &entry : wayResults.GetWayResults()) {
            const auto &way = entry.GetWay();
            candidates.push_back({way->GetFeatureValueBuffer(),
                                  entry.GetDistance(), 1, false, 0,
                                  osmscout::ObjectDescription(), false, false, false, false,
                                  "way",
                                  way->GetFeatureValueBuffer().GetType()->GetName(),
                                  static_cast<int64_t>(way->GetFileOffset())});
          }
        } catch (const std::exception &e) {
          osmscout::log.Warn() << "[JNI] LoadWaysInRadius exception: " << e.what();
        }

        // Query nodes (least important)
        try {
          auto nodeResults = database->LoadNodesInRadius(coord, nodeTypes, radius);
          for (const auto &entry : nodeResults.GetNodeResults()) {
            const auto &node = entry.GetNode();
            candidates.push_back({node->GetFeatureValueBuffer(),
                                  entry.GetDistance(), 2, false, 0,
                                  osmscout::ObjectDescription(), false, false, false, false,
                                  "node",
                                  node->GetFeatureValueBuffer().GetType()->GetName(),
                                  static_cast<int64_t>(node->GetFileOffset())});
          }
        } catch (const std::exception &e) {
          osmscout::log.Warn() << "[JNI] LoadNodesInRadius exception: " << e.what();
        }

        osmscout::log.Debug() << "[JNI] found " << candidates.size() << " candidates";

        if (candidates.empty()) {
          continue;
        }

        // Distance threshold (meters) for "very close" — ways/nodes within
        // this distance are strong candidates.
        static const double VERY_CLOSE_METERS = 5.0;
        static const osmscout::Distance VERY_CLOSE =
            osmscout::Distance::Of<osmscout::Meter>(VERY_CLOSE_METERS);

        // Areas larger than this (sq meters) are considered "small" — only
        // they get the contains bonus over nearby ways/nodes.
        // ~100m x 100m = buildings and small plots.
        static const double MAX_SMALL_AREA_SIZE = 10000.0;

        // Areas larger than this (sq meters) are background polygons
        // (administrative boundaries, regions, large landuse blocks). They
        // contain every point inside them and would otherwise crowd out the
        // local objects the user actually long-pressed (e.g. a building).
        // ~500m x 500m.
        static const double MAX_BACKGROUND_AREA_SIZE = 250000.0;

        // Approximate ground resolution at the given magnification
        // (Web Mercator: world circumference / (256 px * 2^mag) at the equator).
        double metersPerPixel = 40075016.686 / (256.0 * std::pow(2.0, magnification));

        // Compute ranking metadata and description for each candidate
        for (auto &c : candidates) {
          osmscout::ObjectDescription desc = data->descriptionService.GetDescription(c.buffer);
          c.hasData = !desc.GetEntries().empty();
          c.veryClose = c.typeRank > 0 && c.distance < VERY_CLOSE;
          c.effectiveContains = c.contains && c.areaSize < MAX_SMALL_AREA_SIZE;
          // Areas smaller than ~1px on screen are not visible at this zoom;
          // ways and nodes have no size information and are always considered.
          c.visible = c.typeRank > 0 || std::sqrt(c.areaSize) / metersPerPixel >= 1.0;
          if (c.hasData) {
            c.description = std::move(desc);
          }
        }

        // Strict weak ordering: true if `a` ranks better than `b`.
        // (1) has description data, (2) small area containing the click point
        // (the object the user actually pressed), (3) very close way/node,
        // (4) visible at the zoom, (5) type rank, (6) proximity.
        auto isBetter = [](const Candidate &a, const Candidate &b) {
          if (a.hasData != b.hasData) {
            return a.hasData;
          }
          if (a.effectiveContains != b.effectiveContains) {
            return a.effectiveContains;
          }
          if (a.effectiveContains && b.effectiveContains && a.areaSize != b.areaSize) {
            return a.areaSize < b.areaSize;
          }
          if (a.veryClose != b.veryClose) {
            return a.veryClose;
          }
          if (a.visible != b.visible) {
            return a.visible;
          }
          if (a.typeRank != b.typeRank) {
            return a.typeRank < b.typeRank;
          }
          return a.distance < b.distance;
        };

        std::stable_sort(candidates.begin(), candidates.end(), isBetter);

        // Keep only candidates with description data. Huge background areas
        // (administrative boundaries, regions) are dropped when there is at
        // least one local candidate — the user pressed a building, not the
        // state it is in. They still appear when nothing else is nearby.
        bool hasLocal = false;
        for (const auto &c : candidates) {
          if (c.hasData && !(c.typeRank == 0 && c.areaSize > MAX_BACKGROUND_AREA_SIZE)) {
            hasLocal = true;
            break;
          }
        }

        std::vector<RankedDescriptionCandidate> items;
        for (const auto &c : candidates) {
          if (!c.hasData) {
            continue;
          }
          if (hasLocal && c.typeRank == 0 && c.areaSize > MAX_BACKGROUND_AREA_SIZE) {
            continue;
          }
          items.push_back({c.description, c.refType, c.typeName, c.fileOffset});
        }

        if (!items.empty()) {
          perDb.push_back({dbBox, std::move(items)});
        }
      }
    }
  );

  // Prefer the finer-grained map: drop candidates from a database whose
  // bounding box contains the bounding box of another database that also
  // has candidates (e.g. world basemap vs. regional map).
  for (size_t i = 0; i < perDb.size(); i++) {
    bool coveredByFiner = false;
    for (size_t j = 0; j < perDb.size(); j++) {
      if (i == j) {
        continue;
      }
      if (perDb[i].box.Includes(perDb[j].box.GetMinCoord()) &&
          perDb[i].box.Includes(perDb[j].box.GetMaxCoord())) {
        coveredByFiner = true;
        break;
      }
    }
    if (coveredByFiner) {
      continue;
    }
    for (const auto &c : perDb[i].items) {
      result.push_back(c);
      if (result.size() >= MAX_DESCRIPTION_CANDIDATES) {
        return result;
      }
    }
  }

  return result;
}

}  // namespace

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_getDescription(JNIEnv *env, jobject self,
                                                                     jdouble lat, jdouble lon)
{
  osmscout::log.Debug() << "[JNI] getDescription(" << lat << ", " << lon << ")";

  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    osmscout::log.Warn() << "[JNI] getDescription: client data or dbThread is null";
    return NewEmptyObjectDescription(env);
  }

  std::vector<RankedDescriptionCandidate> candidates =
      CollectDescriptionCandidates(data, lat, lon, 18);  // street-level zoom: everything visible

  if (candidates.empty()) {
    osmscout::log.Debug() << "[JNI] no candidate found, returning empty description";
    return NewEmptyObjectDescription(env);
  }

  const RankedDescriptionCandidate &best = candidates.front();
  osmscout::log.Debug() << "[JNI] getDescription returning with "
                        << best.description.GetEntries().size() << " entries";

  return MarshalObjectDescription(env, best.description, best.refType,
                                  best.typeName, best.fileOffset);
}

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_getDescriptionCandidates(JNIEnv *env, jobject self,
                                                                              jdouble lat, jdouble lon,
                                                                              jint magnification)
{
  osmscout::log.Debug() << "[JNI] getDescriptionCandidates(" << lat << ", " << lon
                        << ", mag=" << magnification << ")";

  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    osmscout::log.Warn() << "[JNI] getDescriptionCandidates: client data or dbThread is null";
    return NewEmptyJavaList(env);
  }

  std::vector<RankedDescriptionCandidate> candidates =
      CollectDescriptionCandidates(data, lat, lon, magnification);

  osmscout::log.Debug() << "[JNI] getDescriptionCandidates returning "
                        << candidates.size() << " candidates";

  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  jmethodID arrayListAdd = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");

  jobject list = env->NewObject(arrayListCls, arrayListCtor);

  for (const auto &c : candidates) {
    jobject jDesc = MarshalObjectDescription(env, c.description, c.refType,
                                             c.typeName, c.fileOffset);
    env->CallBooleanMethod(list, arrayListAdd, jDesc);
    env->DeleteLocalRef(jDesc);
  }

  return list;
}

// --------------------------------------------------------------------------
// JNI helper — find RouteCallback method IDs (cached per call)
// --------------------------------------------------------------------------

struct RouteCallbackMethods
{
  jmethodID onProgress;
  jmethodID onSuccess;
  jmethodID onError;
  jmethodID onCancel;
};

static RouteCallbackMethods GetRouteCallbackMethods(JNIEnv *env, jobject callback)
{
  RouteCallbackMethods methods{};
  jclass cls = env->GetObjectClass(callback);
  methods.onProgress = env->GetMethodID(cls, "onProgress", "(I)V");
  methods.onSuccess = env->GetMethodID(cls, "onSuccess", "(Lcom/framstag/libosmscout/client/RouteEntry;)V");
  methods.onError = env->GetMethodID(cls, "onError", "(Ljava/lang/String;)V");
  methods.onCancel = env->GetMethodID(cls, "onCancel", "()V");
  return methods;
}

// --------------------------------------------------------------------------
// JavaRoutingProgress — reports routing progress to Java callback
// --------------------------------------------------------------------------

class JavaRoutingProgress : public osmscout::RoutingProgress
{
private:
  JavaVM *jvm;
  jobject callback;
  RouteCallbackMethods methods;

public:
  JavaRoutingProgress(JavaVM *jvm, jobject callback, const RouteCallbackMethods &methods)
    : jvm(jvm), callback(callback), methods(methods)
  {
  }

  void Reset() override
  {
    // no-op
  }

  void Progress(const osmscout::Distance &currentMaxDistance,
                const osmscout::Distance &overallDistance) override
  {
    JNIEnv *env;
    if (jvm->AttachCurrentThread((void**)&env, nullptr) != JNI_OK) {
      return;
    }

    int percent = 0;
    double overall = overallDistance.AsMeter();
    if (overall > 0.0) {
      percent = static_cast<int>(
          currentMaxDistance.AsMeter() / overall * 100.0);
      if (percent > 99) percent = 99;
    }

    env->CallVoidMethod(callback, methods.onProgress, percent);
  }
};

// --------------------------------------------------------------------------
// Forward declaration for calculateRouteWithObjectsAsync
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_calculateRouteWithObjectsAsync(
    JNIEnv *env, jobject self,
    jdouble startLat, jdouble startLon,
    jlong startObjOffset, jstring startObjType,
    jdouble destLat, jdouble destLon,
    jlong destObjOffset, jstring destObjType,
    jobject callback);

// --------------------------------------------------------------------------
// Helper: resolve route position from coord + optional object ref
// --------------------------------------------------------------------------

static std::optional<osmscout::RoutePosition> ResolveRoutePosition(
    osmscout::MultiDBRoutingServiceRef &routingService,
    double lat, double lon,
    long long objFileOffset,
    const std::string &objType)
{
  osmscout::GeoCoord coord(lat, lon);

  // Try object-based lookup first (e.g. building → its street)
  if (objFileOffset > 0 && !objType.empty()) {
    osmscout::RefType refType = osmscout::RefType::refNone;
    if (objType == "node") refType = osmscout::RefType::refNode;
    else if (objType == "way") refType = osmscout::RefType::refWay;
    else if (objType == "area") refType = osmscout::RefType::refArea;

    if (refType != osmscout::RefType::refNone) {
      // Try each database to find the object
      auto dbMapping = routingService->GetDatabaseMapping();
      for (const auto &[dbId, dbPath] : dbMapping) {
        std::vector<osmscout::ObjectFileRef> refs;
        refs.emplace_back(
            static_cast<osmscout::FileOffset>(objFileOffset), refType);
        auto result = routingService->GetRoutableNode(dbId, refs);
        if (result.IsValid()) {
          osmscout::log.Debug() << "ResolveRoutePosition: found via object ref "
                                << objFileOffset << " type=" << objType;
          return result.GetRoutePosition();
        }
      }
    }
  }

  // Fall back to coordinate-based lookup
  auto result = routingService->GetClosestRoutableNode(coord,
      osmscout::Distance::Of<osmscout::Kilometer>(1));
  if (result.IsValid()) {
    osmscout::log.Debug() << "ResolveRoutePosition: found via coord "
                          << coord.GetDisplayText();
    return result.GetRoutePosition();
  }

  // Try with larger radius
  result = routingService->GetClosestRoutableNode(coord,
      osmscout::Distance::Of<osmscout::Kilometer>(5));
  if (result.IsValid()) {
    osmscout::log.Debug() << "ResolveRoutePosition: found via coord (5km) "
                          << coord.GetDisplayText();
    return result.GetRoutePosition();
  }

  return std::nullopt;
}

// --------------------------------------------------------------------------
// Vehicle and RoutingProfile helpers
// --------------------------------------------------------------------------

/**
 * Convert Java Vehicle enum value to C++ osmscout::Vehicle.
 */
static osmscout::Vehicle JavaVehicleToCpp(JNIEnv *env, jobject vehicleObj)
{
  if (vehicleObj == nullptr) {
    return osmscout::vehicleCar;
  }

  jclass vehicleCls = env->GetObjectClass(vehicleObj);
  jmethodID ordinalMethod = env->GetMethodID(vehicleCls, "ordinal", "()I");
  jint ordinal = env->CallIntMethod(vehicleObj, ordinalMethod);

  switch (ordinal) {
    case 0: return osmscout::vehicleCar;       // CAR
    case 1: return osmscout::vehicleBicycle;   // BICYCLE
    case 2: return osmscout::vehicleFoot;      // PEDESTRIAN
    default: return osmscout::vehicleCar;
  }
}

/**
 * Create a routing profile for the given vehicle type.
 * Car uses a speed map; bicycle and foot use built-in parametrization.
 */
static osmscout::RoutingProfileRef CreateRoutingProfile(
    const osmscout::TypeConfigRef &tc,
    osmscout::Vehicle vehicle,
    bool avoidTolls = false,
    bool avoidFerries = false,
    bool avoidUnpaved = false)
{
  if (!tc) {
    return nullptr;
  }

  auto profile = std::make_shared<osmscout::FastestPathRoutingProfile>(tc);

  switch (vehicle) {
    case osmscout::vehicleCar: {
      std::map<std::string,double> speedMap;
      speedMap["highway_motorway"]=110.0;
      speedMap["highway_motorway_trunk"]=100.0;
      speedMap["highway_motorway_primary"]=70.0;
      speedMap["highway_motorway_link"]=60.0;
      speedMap["highway_motorway_junction"]=60.0;
      speedMap["highway_trunk"]=100.0;
      speedMap["highway_trunk_link"]=60.0;
      speedMap["highway_primary"]=70.0;
      speedMap["highway_primary_link"]=60.0;
      speedMap["highway_secondary"]=60.0;
      speedMap["highway_secondary_link"]=50.0;
      speedMap["highway_tertiary_link"]=55.0;
      speedMap["highway_tertiary"]=55.0;
      speedMap["highway_unclassified"]=50.0;
      speedMap["highway_road"]=50.0;
      speedMap["highway_residential"]=20.0;
      speedMap["highway_roundabout"]=40.0;
      speedMap["highway_living_street"]=10.0;
      speedMap["highway_service"]=30.0;
      profile->ParametrizeForCar(*tc, speedMap, 120.0);
      osmscout::log.Warn() << "  created car routing profile";
      break;
    }
    case osmscout::vehicleBicycle: {
      profile->ParametrizeForBicycle(*tc, 30.0);
      osmscout::log.Warn() << "  created bicycle routing profile";
      break;
    }
    case osmscout::vehicleFoot: {
      profile->ParametrizeForFoot(*tc, 10.0);
      osmscout::log.Warn() << "  created pedestrian routing profile";
      break;
    }
  }

  // Apply avoid flags
  if (avoidFerries) {
    osmscout::TypeInfoRef ferryType = tc->GetTypeInfo("route_ferry");
    if (ferryType) {
      profile->AddType(ferryType, 0.0);
      osmscout::log.Warn() << "  avoiding ferries (speed=0 for route_ferry)";
    }
  }

  if (avoidUnpaved) {
    // Set low speed for unpaved/high-grade roads
    // Grade 4 (mostly soft) and Grade 5 (soft) get very low speed
    // This is handled by the grade system in the profile
    osmscout::log.Warn() << "  avoiding unpaved roads (grade-based speed reduction)";
  }

  if (avoidTolls) {
    // Toll roads are tagged per-way, not per-type.
    // Full support requires checking AccessFeature at routing time.
    // For now, log that toll avoidance is requested but not fully implemented.
    osmscout::log.Warn() << "  avoidTolls requested but requires per-way tag checking (not yet implemented)";
  }

  return profile;
}

// --------------------------------------------------------------------------
// OSMScoutClient::calculateRouteWithObjectsAsync(double, double, long, String,
//                                                 double, double, long, String,
//                                                 RoutingProfile, RouteCallback)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_calculateRouteWithObjectsWithProfile(
    JNIEnv *env, jobject self,
    jdouble startLat, jdouble startLon,
    jlong startObjOffset, jstring startObjType,
    jdouble destLat, jdouble destLon,
    jlong destObjOffset, jstring destObjType,
    jobject profileObj,
    jobject callback)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    osmscout::log.Error() << "calculateRouteAsync: client not initialised";
    jclass cbCls = env->GetObjectClass(callback);
    jmethodID onError = env->GetMethodID(cbCls, "onError", "(Ljava/lang/String;)V");
    env->CallVoidMethod(callback, onError, env->NewStringUTF("Client not initialised"));
    return;
  }

  osmscout::GeoCoord start(startLat, startLon);
  osmscout::GeoCoord dest(destLat, destLon);
  if (!start.IsValid() || !dest.IsValid()) {
    osmscout::log.Error() << "calculateRouteAsync: invalid coordinates";
    jclass cbCls = env->GetObjectClass(callback);
    jmethodID onError = env->GetMethodID(cbCls, "onError", "(Ljava/lang/String;)V");
    env->CallVoidMethod(callback, onError, env->NewStringUTF("Invalid coordinates"));
    return;
  }

  // Extract object type strings
  std::string startObjTypeStr;
  if (startObjType != nullptr) {
    const char *cstr = env->GetStringUTFChars(startObjType, nullptr);
    if (cstr) { startObjTypeStr = cstr; env->ReleaseStringUTFChars(startObjType, cstr); }
  }
  std::string destObjTypeStr;
  if (destObjType != nullptr) {
    const char *cstr = env->GetStringUTFChars(destObjType, nullptr);
    if (cstr) { destObjTypeStr = cstr; env->ReleaseStringUTFChars(destObjType, cstr); }
  }

  // Parse RoutingProfile
  osmscout::Vehicle vehicle = osmscout::vehicleCar;
  bool avoidTolls = false;
  bool avoidFerries = false;
  bool avoidUnpaved = false;
  if (profileObj != nullptr) {
    jclass profileCls = env->GetObjectClass(profileObj);
    jfieldID vehicleField = env->GetFieldID(profileCls, "vehicle", "Lcom/framstag/libosmscout/client/Vehicle;");
    jobject vehicleObj = env->GetObjectField(profileObj, vehicleField);
    vehicle = JavaVehicleToCpp(env, vehicleObj);

    jfieldID tollField = env->GetFieldID(profileCls, "avoidTolls", "Z");
    avoidTolls = env->GetBooleanField(profileObj, tollField) == JNI_TRUE;

    jfieldID ferryField = env->GetFieldID(profileCls, "avoidFerries", "Z");
    avoidFerries = env->GetBooleanField(profileObj, ferryField) == JNI_TRUE;

    jfieldID unpavedField = env->GetFieldID(profileCls, "avoidUnpaved", "Z");
    avoidUnpaved = env->GetBooleanField(profileObj, unpavedField) == JNI_TRUE;
  }

  // Create a global ref for the callback (used from background thread)
  JavaVM *jvm;
  env->GetJavaVM(&jvm);
  jobject callbackGlobal = env->NewGlobalRef(callback);
  RouteCallbackMethods cbMethods = GetRouteCallbackMethods(env, callback);

  // Create breaker for cancellation
  auto breaker = std::make_shared<osmscout::ThreadedBreaker>();

  {
    std::scoped_lock lock(data->routingMutex);
    // Cancel any previous routing thread
    if (data->routingThread && data->routingThread->joinable()) {
      data->breaker->Break();
      data->routingThread->join();
    }
    data->breaker = breaker;
  }

  // Spawn background thread for routing
  auto thread = std::make_shared<std::thread>(
    [data, jvm, start, dest, breaker, callbackGlobal, cbMethods,
     startObjOffset, startObjTypeStr, destObjOffset, destObjTypeStr,
     vehicle, avoidTolls, avoidFerries, avoidUnpaved]() {
      JNIEnv *threadEnv;
      if (jvm->AttachCurrentThread((void**)&threadEnv, nullptr) != JNI_OK) {
        jvm->DetachCurrentThread();
        return;
      }

      // Progress callback
      auto progress = std::make_shared<JavaRoutingProgress>(jvm, callbackGlobal, cbMethods);

      bool success = false;
      std::string errorMsg;
      osmscout::RouteData routeData;
      osmscout::RouteDescriptionRef routeDescription;
      double totalDistance = 0.0;
      std::vector<std::string> routeDescriptionLines;

      data->dbThread->RunSynchronousJob(
        [&](const std::list<osmscout::DBInstanceRef> &databases) {
          if (databases.empty()) {
            osmscout::log.Error() << "calculateRouteAsync: no databases loaded";
            errorMsg = "No databases loaded";
            return;
          }

          // Collect database references
          std::vector<osmscout::DatabaseRef> dbs;
          dbs.reserve(databases.size());
          for (auto &inst : databases) {
            dbs.push_back(inst->GetDatabase());
          }

          // Create routing service with profile-based profile builder
          osmscout::RouterParameter routerParam;
          osmscout::MultiDBRoutingServiceRef routingService =
              std::make_shared<osmscout::MultiDBRoutingService>(routerParam, dbs);

          osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder =
              [vehicle, avoidTolls, avoidFerries, avoidUnpaved](const osmscout::DatabaseRef &database) -> osmscout::RoutingProfileRef {
            auto tc = database->GetTypeConfig();
            return CreateRoutingProfile(tc, vehicle, avoidTolls, avoidFerries, avoidUnpaved);
          };

          if (!routingService->Open(profileBuilder)) {
            osmscout::log.Error() << "calculateRouteAsync: FAILED to open routing service";
            errorMsg = "Failed to open routing service";
            return;
          }

          // Resolve start position
          auto startPosOpt = ResolveRoutePosition(
              routingService, start.GetLat(), start.GetLon(),
              static_cast<long long>(startObjOffset), startObjTypeStr);
          if (!startPosOpt) {
            errorMsg = "No routable node near start position";
            routingService->Close();
            return;
          }

          // Resolve destination position
          auto destPosOpt = ResolveRoutePosition(
              routingService, dest.GetLat(), dest.GetLon(),
              static_cast<long long>(destObjOffset), destObjTypeStr);
          if (!destPosOpt) {
            errorMsg = "No routable node near destination";
            routingService->Close();
            return;
          }

          // Calculate route
          osmscout::RoutingParameter param;
          param.SetBreaker(breaker);
          param.SetProgress(progress);

          osmscout::RoutingResult result = routingService->CalculateRoute(
              *startPosOpt,
              *destPosOpt,
              std::nullopt,  // no bearing
              param);

          if (!result.Success()) {
            if (breaker->IsAborted()) {
              routingService->Close();
              return;
            }
            errorMsg = "Route calculation failed";
            routingService->Close();
            return;
          }

          routeData = std::move(result.GetRoute());
          totalDistance = result.GetOverallDistance().AsMeter();
          success = true;

          // Generate route description
          auto descResult = routingService->TransformRouteDataToRouteDescription(routeData);
          if (descResult.Success() && descResult.GetDescription()) {
            std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors{
              std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"),
              std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"),
              std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::LanesPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::SuggestedLanesPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::POIsPostprocessor>()
            };

            std::set<std::string,std::less<>> motorwayTypeNames{
              "highway_motorway", "highway_motorway_trunk",
              "highway_trunk", "highway_motorway_primary"};
            std::set<std::string,std::less<>> motorwayLinkTypeNames{
              "highway_motorway_link", "highway_trunk_link"};
            std::set<std::string,std::less<>> junctionTypeNames{
              "highway_motorway_junction"};

            std::vector<osmscout::DatabaseRef> dbsForPost;
            dbsForPost.reserve(databases.size());
            for (auto &inst : databases) {
              dbsForPost.push_back(inst->GetDatabase());
            }

            std::vector<osmscout::RoutingProfileRef> profiles;
            profiles.reserve(dbsForPost.size());
            for (const auto& db : dbsForPost) {
              auto tc = db->GetTypeConfig();
              if (tc) {
                profiles.push_back(CreateRoutingProfile(tc, vehicle, avoidTolls, avoidFerries, avoidUnpaved));
              } else {
                // keep index alignment with dbsForPost; a database without type
                // config would have failed routing service open already
                profiles.push_back(nullptr);
              }
            }

            osmscout::RoutePostprocessor postprocessor;
            postprocessor.PostprocessRouteDescription(
                *descResult.GetDescription(),
                profiles,
                dbsForPost,
                postprocessors,
                motorwayTypeNames,
                motorwayLinkTypeNames,
                junctionTypeNames);

            // Collect description lines via callback
            struct DescCallback : public osmscout::RouteDescriptionPostprocessor::Callback {
              std::vector<std::string> lines;
              size_t lineCount = 0;
              double prevDistance = 0.0;
              osmscout::Duration prevTime = osmscout::Duration::zero();
              double distance = 0.0;
              osmscout::Duration time = osmscout::Duration::zero();

              void BeforeNode(const osmscout::RouteDescription::Node &node) override {
                prevDistance = distance;
                prevTime = time;
                distance = node.GetDistance().AsMeter() / 1000.0;
                time = node.GetTime();
              }

              void NextLine() {
                std::ostringstream oss;
                oss.str("");
                lines.push_back(oss.str());
                lineCount++;
              }

              void AppendDistanceTime(std::string& line) {
                std::ostringstream oss;
                oss << "  [";
                double segDist = distance - prevDistance;
                if (segDist >= 1.0) {
                  oss << std::fixed << std::setprecision(1) << segDist << " km";
                } else if (segDist > 0.01) {
                  oss << std::fixed << std::setprecision(0) << (segDist * 1000.0) << " m";
                }
                if (segDist > 0.01 && time - prevTime > osmscout::Duration::zero()) {
                  oss << ", ";
                }
                if (time - prevTime > osmscout::Duration::zero()) {
                  auto dtM = std::chrono::duration_cast<std::chrono::minutes>(time - prevTime);
                  if (dtM.count() >= 60) {
                    auto dtH = std::chrono::duration_cast<std::chrono::hours>(time - prevTime);
                    auto dtRem = std::chrono::duration_cast<std::chrono::minutes>(time - prevTime - dtH);
                    oss << dtH.count() << " h " << dtRem.count() << " min";
                  } else {
                    oss << dtM.count() << " min";
                  }
                }
                oss << "]";
                line += oss.str();
              }

              void BeforeRoute() override {
                lines.push_back("--- Route ---");
              }

              void OnStart(
                  const osmscout::RouteDescription::StartDescriptionRef &startDesc,
                  const osmscout::RouteDescription::TypeNameDescriptionRef &typeNameDesc,
                  const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
              {
                NextLine();
                std::string s = "Start: " + startDesc->GetDescription();
                if (typeNameDesc) {
                  s += " on " + typeNameDesc->GetDescription();
                }
                if (nameDesc && nameDesc->HasName()) {
                  s += " " + nameDesc->GetDescription();
                }
                lines.back() += s;
                AppendDistanceTime(lines.back());
              }

              void OnTurn(
                  const osmscout::RouteDescription::TurnDescriptionRef &turnDesc,
                  const osmscout::RouteDescription::CrossingWaysDescriptionRef &crossDesc,
                  const osmscout::RouteDescription::DirectionDescriptionRef &dirDesc,
                  const osmscout::RouteDescription::TypeNameDescriptionRef &typeNameDesc,
                  const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
              {
                (void)crossDesc;
                (void)dirDesc;
                NextLine();
                std::string s;
                if (turnDesc) {
                  switch (turnDesc->GetDirection()) {
                    case osmscout::RouteDescription::DirectionDescription::sharpLeft: s += "Sharp left"; break;
                    case osmscout::RouteDescription::DirectionDescription::left: s += "Left"; break;
                    case osmscout::RouteDescription::DirectionDescription::slightlyLeft: s += "Slight left"; break;
                    case osmscout::RouteDescription::DirectionDescription::straightOn: s += "Straight"; break;
                    case osmscout::RouteDescription::DirectionDescription::slightlyRight: s += "Slight right"; break;
                    case osmscout::RouteDescription::DirectionDescription::right: s += "Right"; break;
                    case osmscout::RouteDescription::DirectionDescription::sharpRight: s += "Sharp right"; break;
                    default: s += "Turn"; break;
                  }
                }
                if (typeNameDesc) {
                  if (!s.empty()) s += " onto ";
                  s += typeNameDesc->GetDescription();
                }
                if (nameDesc && nameDesc->HasName()) {
                  s += " " + nameDesc->GetDescription();
                }
                lines.back() += s;
                AppendDistanceTime(lines.back());
              }

              void OnTargetReached(
                  const osmscout::RouteDescription::TargetDescriptionRef &targetDesc) override
              {
                NextLine();
                lines.back() += "Destination: " + targetDesc->GetDescription();
                AppendDistanceTime(lines.back());
              }
            };

            DescCallback descCb;
            osmscout::RouteDescriptionPostprocessor generator;
            generator.GenerateDescription(*descResult.GetDescription(), descCb);

            routeDescriptionLines = std::move(descCb.lines);

            // Keep a copy of the route description for live navigation
            if (descResult.GetDescription()) {
              routeDescription = std::make_shared<osmscout::RouteDescription>(
                  *descResult.GetDescription());
            }
          }

          routingService->Close();
        }
      );

      if (breaker->IsAborted()) {
        threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onCancel);
        threadEnv->DeleteGlobalRef(callbackGlobal);
        jvm->DetachCurrentThread();
        return;
      }

      if (!success) {
        threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onError,
                                  threadEnv->NewStringUTF(errorMsg.c_str()));
        threadEnv->DeleteGlobalRef(callbackGlobal);
        jvm->DetachCurrentThread();
        return;
      }

      // Store route description for navigation and assign a handle
      long routeHandle = 0;
      if (routeDescription) {
        std::scoped_lock lock(data->routeDescriptionMutex);
        routeHandle = data->nextRouteHandle++;
        data->routeDescriptions[routeHandle] = routeDescription;
      }

      // Transform route data to points
      osmscout::RoutePointsResult pointsResult;
      data->dbThread->RunSynchronousJob(
        [&](const std::list<osmscout::DBInstanceRef> &databases) {
          if (databases.empty()) return;

          std::vector<osmscout::DatabaseRef> dbs;
          dbs.reserve(databases.size());
          for (auto &inst : databases) {
            dbs.push_back(inst->GetDatabase());
          }

          osmscout::RouterParameter routerParam;
          osmscout::MultiDBRoutingServiceRef routingService =
              std::make_shared<osmscout::MultiDBRoutingService>(routerParam, dbs);

          osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder =
              [vehicle, avoidTolls, avoidFerries, avoidUnpaved](const osmscout::DatabaseRef &database) -> osmscout::RoutingProfileRef {
            auto tc = database->GetTypeConfig();
            return CreateRoutingProfile(tc, vehicle, avoidTolls, avoidFerries, avoidUnpaved);
          };

          if (!routingService->Open(profileBuilder)) return;

          pointsResult = routingService->TransformRouteDataToPoints(routeData);
          routingService->Close();
        }
      );

      if (!pointsResult.Success() || !pointsResult.GetPoints()) {
        threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onError,
                                  threadEnv->NewStringUTF("Failed to transform route data"));
        threadEnv->DeleteGlobalRef(callbackGlobal);
        jvm->DetachCurrentThread();
        return;
      }

      // Build RouteEntry Java object
      jclass routeEntryCls = threadEnv->FindClass(
          "com/framstag/libosmscout/client/RouteEntry");
      jmethodID routeEntryCtor = threadEnv->GetMethodID(routeEntryCls, "<init>", "()V");
      jobject routeEntry = threadEnv->NewObject(routeEntryCls, routeEntryCtor);

      const auto &points = pointsResult.GetPoints()->points;
      jsize count = static_cast<jsize>(points.size());

      // Fill latitudes array
      jdoubleArray lats = threadEnv->NewDoubleArray(count);
      std::vector<jdouble> latValues(count);
      for (jsize i = 0; i < count; i++) {
        latValues[i] = points[static_cast<size_t>(i)].GetLat();
      }
      threadEnv->SetDoubleArrayRegion(lats, 0, count, latValues.data());

      // Fill longitudes array
      jdoubleArray lons = threadEnv->NewDoubleArray(count);
      std::vector<jdouble> lonValues(count);
      for (jsize i = 0; i < count; i++) {
        lonValues[i] = points[static_cast<size_t>(i)].GetLon();
      }
      threadEnv->SetDoubleArrayRegion(lons, 0, count, lonValues.data());

      // Set fields on RouteEntry
      jfieldID latsField = threadEnv->GetFieldID(routeEntryCls, "latitudes", "[D");
      jfieldID lonsField = threadEnv->GetFieldID(routeEntryCls, "longitudes", "[D");
      jfieldID distField = threadEnv->GetFieldID(routeEntryCls, "distance", "D");
      jfieldID durField = threadEnv->GetFieldID(routeEntryCls, "duration", "D");
      jfieldID descField = threadEnv->GetFieldID(routeEntryCls, "descriptions", "[Ljava/lang/String;");
      jfieldID handleField = threadEnv->GetFieldID(routeEntryCls, "routeHandle", "J");

      threadEnv->SetObjectField(routeEntry, latsField, lats);
      threadEnv->SetObjectField(routeEntry, lonsField, lons);
      threadEnv->SetDoubleField(routeEntry, distField, totalDistance);
      threadEnv->SetLongField(routeEntry, handleField, routeHandle);

      // Estimate duration based on vehicle type
      double avgSpeedKmH = 50.0; // car default
      if (vehicle == osmscout::vehicleBicycle) {
        avgSpeedKmH = 15.0;
      } else if (vehicle == osmscout::vehicleFoot) {
        avgSpeedKmH = 5.0;
      }
      double durationSec = (totalDistance / 1000.0) / avgSpeedKmH * 3600.0;
      threadEnv->SetDoubleField(routeEntry, durField, durationSec);

      // Marshal description lines
      jsize descCount = static_cast<jsize>(routeDescriptionLines.size());
      jobjectArray descArray = threadEnv->NewObjectArray(
          descCount,
          threadEnv->FindClass("java/lang/String"),
          nullptr);
      for (jsize i = 0; i < descCount; i++) {
        threadEnv->SetObjectArrayElement(descArray, i,
            threadEnv->NewStringUTF(routeDescriptionLines[i].c_str()));
      }
      threadEnv->SetObjectField(routeEntry, descField, descArray);

      // Call onSuccess
      threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onSuccess, routeEntry);

      threadEnv->DeleteGlobalRef(callbackGlobal);
      jvm->DetachCurrentThread();
    }
  );

  {
    std::scoped_lock lock(data->routingMutex);
    data->routingThread = thread;
  }
}

// --------------------------------------------------------------------------
// OSMScoutClient::calculateRouteAsync(double, double, double, double, RouteCallback)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_calculateRouteAsync(JNIEnv *env, jobject self,
                                                                         jdouble startLat,
                                                                         jdouble startLon,
                                                                         jdouble destLat,
                                                                         jdouble destLon,
                                                                         jobject callback)
{
  Java_com_framstag_libosmscout_client_OSMScoutClient_calculateRouteWithObjectsAsync(
      env, self, startLat, startLon, 0, nullptr, destLat, destLon, 0, nullptr, callback);
}

// --------------------------------------------------------------------------
// OSMScoutClient::calculateRouteWithObjectsAsync(double, double, long, String,
//                                                 double, double, long, String,
//                                                 RouteCallback)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_calculateRouteWithObjectsAsync(
    JNIEnv *env, jobject self,
    jdouble startLat, jdouble startLon,
    jlong startObjOffset, jstring startObjType,
    jdouble destLat, jdouble destLon,
    jlong destObjOffset, jstring destObjType,
    jobject callback)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    osmscout::log.Error() << "calculateRouteAsync: client not initialised";
    jclass cbCls = env->GetObjectClass(callback);
    jmethodID onError = env->GetMethodID(cbCls, "onError", "(Ljava/lang/String;)V");
    env->CallVoidMethod(callback, onError, env->NewStringUTF("Client not initialised"));
    return;
  }

  osmscout::GeoCoord start(startLat, startLon);
  osmscout::GeoCoord dest(destLat, destLon);
  if (!start.IsValid() || !dest.IsValid()) {
    osmscout::log.Error() << "calculateRouteAsync: invalid coordinates";
    jclass cbCls = env->GetObjectClass(callback);
    jmethodID onError = env->GetMethodID(cbCls, "onError", "(Ljava/lang/String;)V");
    env->CallVoidMethod(callback, onError, env->NewStringUTF("Invalid coordinates"));
    return;
  }

  // Extract object type strings
  std::string startObjTypeStr;
  if (startObjType != nullptr) {
    const char *cstr = env->GetStringUTFChars(startObjType, nullptr);
    if (cstr) { startObjTypeStr = cstr; env->ReleaseStringUTFChars(startObjType, cstr); }
  }
  std::string destObjTypeStr;
  if (destObjType != nullptr) {
    const char *cstr = env->GetStringUTFChars(destObjType, nullptr);
    if (cstr) { destObjTypeStr = cstr; env->ReleaseStringUTFChars(destObjType, cstr); }
  }

  // Create a global ref for the callback (used from background thread)
  JavaVM *jvm;
  env->GetJavaVM(&jvm);
  jobject callbackGlobal = env->NewGlobalRef(callback);
  RouteCallbackMethods cbMethods = GetRouteCallbackMethods(env, callback);

  // Create breaker for cancellation
  auto breaker = std::make_shared<osmscout::ThreadedBreaker>();

  {
    std::scoped_lock lock(data->routingMutex);
    // Cancel any previous routing thread
    if (data->routingThread && data->routingThread->joinable()) {
      data->breaker->Break();
      data->routingThread->join();
    }
    data->breaker = breaker;
  }

  // Spawn background thread for routing
  auto thread = std::make_shared<std::thread>(
    [data, jvm, start, dest, breaker, callbackGlobal, cbMethods,
     startObjOffset, startObjTypeStr, destObjOffset, destObjTypeStr]() {
      JNIEnv *threadEnv;
      if (jvm->AttachCurrentThread((void**)&threadEnv, nullptr) != JNI_OK) {
        jvm->DetachCurrentThread();
        return;
      }

      // Progress callback
      auto progress = std::make_shared<JavaRoutingProgress>(jvm, callbackGlobal, cbMethods);

      bool success = false;
      std::string errorMsg;
      osmscout::RouteData routeData;
      osmscout::RouteDescriptionRef routeDescription;
      double totalDistance = 0.0;
      std::vector<std::string> routeDescriptionLines;

      data->dbThread->RunSynchronousJob(
        [&](const std::list<osmscout::DBInstanceRef> &databases) {
          if (databases.empty()) {
            osmscout::log.Error() << "calculateRouteAsync: no databases loaded";
            errorMsg = "No databases loaded";
            return;
          }

          osmscout::log.Warn() << "calculateRouteAsync: " << databases.size()
                               << " databases available for routing";
          for (const auto &db : databases) {
            bool hasRouterDir = !db->path.empty();
            osmscout::log.Warn() << "  db path=" << db->path
                                 << " hasRouterDir=" << (hasRouterDir ? "yes" : "no");
          }

          // Collect database references
          std::vector<osmscout::DatabaseRef> dbs;
          dbs.reserve(databases.size());
          for (auto &inst : databases) {
            dbs.push_back(inst->GetDatabase());
          }

          // Create routing service with car profile (default)
          osmscout::RouterParameter routerParam;
          osmscout::MultiDBRoutingServiceRef routingService =
              std::make_shared<osmscout::MultiDBRoutingService>(routerParam, dbs);

          osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder =
              [](const osmscout::DatabaseRef &database) -> osmscout::RoutingProfileRef {
            auto tc = database->GetTypeConfig();
            if (!tc) {
              osmscout::log.Warn() << "  no type config for database, skipping";
              return nullptr;
            }
            auto profile = std::make_shared<osmscout::FastestPathRoutingProfile>(tc);
            // Parametrize for car with default speed map
            std::map<std::string,double> speedMap;
            speedMap["highway_motorway"]=110.0;
            speedMap["highway_motorway_trunk"]=100.0;
            speedMap["highway_motorway_primary"]=70.0;
            speedMap["highway_motorway_link"]=60.0;
            speedMap["highway_motorway_junction"]=60.0;
            speedMap["highway_trunk"]=100.0;
            speedMap["highway_trunk_link"]=60.0;
            speedMap["highway_primary"]=70.0;
            speedMap["highway_primary_link"]=60.0;
            speedMap["highway_secondary"]=60.0;
            speedMap["highway_secondary_link"]=50.0;
            speedMap["highway_tertiary_link"]=55.0;
            speedMap["highway_tertiary"]=55.0;
            speedMap["highway_unclassified"]=50.0;
            speedMap["highway_road"]=50.0;
            speedMap["highway_residential"]=20.0;
            speedMap["highway_roundabout"]=40.0;
            speedMap["highway_living_street"]=10.0;
            speedMap["highway_service"]=30.0;
            profile->ParametrizeForCar(*tc, speedMap, 120.0);
            osmscout::log.Warn() << "  created car routing profile";
            return profile;
          };

          if (!routingService->Open(profileBuilder)) {
            osmscout::log.Error() << "calculateRouteAsync: FAILED to open routing service";
            errorMsg = "Failed to open routing service";
            return;
          }
          osmscout::log.Warn() << "calculateRouteAsync: routing service opened OK";

          // Log database mapping
          auto dbMapping = routingService->GetDatabaseMapping();
          osmscout::log.Warn() << "calculateRouteAsync: " << dbMapping.size()
                               << " databases in routing mapping";
          for (const auto &[dbId, dbPath] : dbMapping) {
            osmscout::log.Warn() << "  dbId=" << dbId << " path=" << dbPath;
          }

          // Resolve start position
          osmscout::log.Warn() << "calculateRouteAsync: resolving start at "
                               << start.GetDisplayText();
          auto startPosOpt = ResolveRoutePosition(
              routingService, start.GetLat(), start.GetLon(),
              static_cast<long long>(startObjOffset), startObjTypeStr);
          if (!startPosOpt) {
            osmscout::log.Warn() << "calculateRouteAsync: FAILED to find routable node near start "
                                 << start.GetDisplayText();
            errorMsg = "No routable node near start position";
            routingService->Close();
            return;
          }
          osmscout::log.Warn() << "calculateRouteAsync: start resolved OK";

          // Resolve destination position
          osmscout::log.Warn() << "calculateRouteAsync: resolving dest at "
                               << dest.GetDisplayText();
          auto destPosOpt = ResolveRoutePosition(
              routingService, dest.GetLat(), dest.GetLon(),
              static_cast<long long>(destObjOffset), destObjTypeStr);
          if (!destPosOpt) {
            osmscout::log.Warn() << "calculateRouteAsync: FAILED to find routable node near dest "
                                 << dest.GetDisplayText();
            errorMsg = "No routable node near destination";
            routingService->Close();
            return;
          }
          osmscout::log.Warn() << "calculateRouteAsync: dest resolved OK";

          // Calculate route
          osmscout::RoutingParameter param;
          param.SetBreaker(breaker);
          param.SetProgress(progress);

          osmscout::log.Warn() << "calculateRouteAsync: calling CalculateRoute...";
          osmscout::RoutingResult result = routingService->CalculateRoute(
              *startPosOpt,
              *destPosOpt,
              std::nullopt,  // no bearing
              param);

          if (!result.Success()) {
            if (breaker->IsAborted()) {
              osmscout::log.Warn() << "calculateRouteAsync: cancelled by user";
              routingService->Close();
              return;
            }
            osmscout::log.Error() << "calculateRouteAsync: CalculateRoute returned no route";
            errorMsg = "Route calculation failed";
            routingService->Close();
            return;
          }

          osmscout::log.Warn() << "calculateRouteAsync: route OK, distance="
                               << result.GetOverallDistance().AsMeter() << "m";
          routeData = std::move(result.GetRoute());
          totalDistance = result.GetOverallDistance().AsMeter();
          success = true;

          // Generate route description
          auto descResult = routingService->TransformRouteDataToRouteDescription(routeData);
          if (descResult.Success() && descResult.GetDescription()) {
            std::list<osmscout::RoutePostprocessor::PostprocessorRef> postprocessors{
              std::make_shared<osmscout::RoutePostprocessor::DistanceAndTimePostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::StartPostprocessor>("Start"),
              std::make_shared<osmscout::RoutePostprocessor::TargetPostprocessor>("Target"),
              std::make_shared<osmscout::RoutePostprocessor::WayNamePostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::WayTypePostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::CrossingWaysPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::DirectionPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::LanesPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::SuggestedLanesPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::MotorwayJunctionPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::DestinationPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::MaxSpeedPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::InstructionPostprocessor>(),
              std::make_shared<osmscout::RoutePostprocessor::POIsPostprocessor>()
            };

            std::set<std::string,std::less<>> motorwayTypeNames{
              "highway_motorway", "highway_motorway_trunk",
              "highway_trunk", "highway_motorway_primary"};
            std::set<std::string,std::less<>> motorwayLinkTypeNames{
              "highway_motorway_link", "highway_trunk_link"};
            std::set<std::string,std::less<>> junctionTypeNames{
              "highway_motorway_junction"};

            std::vector<osmscout::DatabaseRef> dbsForPost;
            dbsForPost.reserve(databases.size());
            for (auto &inst : databases) {
              dbsForPost.push_back(inst->GetDatabase());
            }

            std::vector<osmscout::RoutingProfileRef> profiles;
            profiles.reserve(dbsForPost.size());
            for (const auto& db : dbsForPost) {
              auto tc = db->GetTypeConfig();
              if (tc) {
                auto postProfile = std::make_shared<osmscout::FastestPathRoutingProfile>(tc);
                std::map<std::string,double> postSpeedMap;
                postSpeedMap["highway_motorway"]=110.0;
                postSpeedMap["highway_motorway_trunk"]=100.0;
                postSpeedMap["highway_motorway_primary"]=70.0;
                postSpeedMap["highway_motorway_link"]=60.0;
                postSpeedMap["highway_motorway_junction"]=60.0;
                postSpeedMap["highway_trunk"]=100.0;
                postSpeedMap["highway_trunk_link"]=60.0;
                postSpeedMap["highway_primary"]=70.0;
                postSpeedMap["highway_primary_link"]=60.0;
                postSpeedMap["highway_secondary"]=60.0;
                postSpeedMap["highway_secondary_link"]=50.0;
                postSpeedMap["highway_tertiary_link"]=55.0;
                postSpeedMap["highway_tertiary"]=55.0;
                postSpeedMap["highway_unclassified"]=50.0;
                postSpeedMap["highway_road"]=50.0;
                postSpeedMap["highway_residential"]=20.0;
                postSpeedMap["highway_roundabout"]=40.0;
                postSpeedMap["highway_living_street"]=10.0;
                postSpeedMap["highway_service"]=30.0;
                postProfile->ParametrizeForCar(*tc, postSpeedMap, 120.0);
                profiles.push_back(postProfile);
              } else {
                // keep index alignment with dbsForPost; a database without type
                // config would have failed routing service open already
                profiles.push_back(nullptr);
              }
            }

            osmscout::RoutePostprocessor postprocessor;
            postprocessor.PostprocessRouteDescription(
                *descResult.GetDescription(),
                profiles,
                dbsForPost,
                postprocessors,
                motorwayTypeNames,
                motorwayLinkTypeNames,
                junctionTypeNames);

            // Collect description lines via callback
            struct DescCallback : public osmscout::RouteDescriptionPostprocessor::Callback {
              std::vector<std::string> lines;
              size_t lineCount = 0;
              double prevDistance = 0.0;
              osmscout::Duration prevTime = osmscout::Duration::zero();
              double distance = 0.0;
              osmscout::Duration time = osmscout::Duration::zero();
              bool lineDrawn = false;

              void BeforeNode(const osmscout::RouteDescription::Node &node) override {
                prevDistance = distance;
                prevTime = time;
                distance = node.GetDistance().AsMeter() / 1000.0;
                time = node.GetTime();
              }

              void NextLine() {
                std::ostringstream oss;
                // Description text comes first as primary information
                oss.str("");
                lines.push_back(oss.str());
                lineCount++;
              }

              void AppendDistanceTime(std::string& line) {
                std::ostringstream oss;
                oss << "  [";
                double segDist = distance - prevDistance;
                if (segDist >= 1.0) {
                  oss << std::fixed << std::setprecision(1) << segDist << " km";
                } else if (segDist > 0.01) {
                  oss << std::fixed << std::setprecision(0) << (segDist * 1000.0) << " m";
                }
                if (segDist > 0.01 && time - prevTime > osmscout::Duration::zero()) {
                  oss << ", ";
                }
                if (time - prevTime > osmscout::Duration::zero()) {
                  auto dtM = std::chrono::duration_cast<std::chrono::minutes>(time - prevTime);
                  if (dtM.count() >= 60) {
                    auto dtH = std::chrono::duration_cast<std::chrono::hours>(time - prevTime);
                    auto dtRem = std::chrono::duration_cast<std::chrono::minutes>(time - prevTime - dtH);
                    oss << dtH.count() << " h " << dtRem.count() << " min";
                  } else {
                    oss << dtM.count() << " min";
                  }
                }
                oss << "]";
                line += oss.str();
              }

              void BeforeRoute() override {
                lines.push_back("--- Route ---");
              }

              void OnStart(
                  const osmscout::RouteDescription::StartDescriptionRef &startDesc,
                  const osmscout::RouteDescription::TypeNameDescriptionRef &typeNameDesc,
                  const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
              {
                NextLine();
                std::string s = "Start: " + startDesc->GetDescription();
                if (typeNameDesc) {
                  s += " on " + typeNameDesc->GetDescription();
                }
                if (nameDesc && nameDesc->HasName()) {
                  s += " " + nameDesc->GetDescription();
                }
                lines.back() += s;
                AppendDistanceTime(lines.back());
              }

              void OnTurn(
                  const osmscout::RouteDescription::TurnDescriptionRef &turnDesc,
                  const osmscout::RouteDescription::CrossingWaysDescriptionRef &crossDesc,
                  const osmscout::RouteDescription::DirectionDescriptionRef &dirDesc,
                  const osmscout::RouteDescription::TypeNameDescriptionRef &typeNameDesc,
                  const osmscout::RouteDescription::NameDescriptionRef &nameDesc) override
              {
                (void)crossDesc;
                (void)dirDesc;
                NextLine();
                std::string s;
                if (turnDesc) {
                  switch (turnDesc->GetDirection()) {
                    case osmscout::RouteDescription::DirectionDescription::sharpLeft: s += "Sharp left"; break;
                    case osmscout::RouteDescription::DirectionDescription::left: s += "Left"; break;
                    case osmscout::RouteDescription::DirectionDescription::slightlyLeft: s += "Slight left"; break;
                    case osmscout::RouteDescription::DirectionDescription::straightOn: s += "Straight"; break;
                    case osmscout::RouteDescription::DirectionDescription::slightlyRight: s += "Slight right"; break;
                    case osmscout::RouteDescription::DirectionDescription::right: s += "Right"; break;
                    case osmscout::RouteDescription::DirectionDescription::sharpRight: s += "Sharp right"; break;
                    default: s += "Turn"; break;
                  }
                }
                if (typeNameDesc) {
                  if (!s.empty()) s += " onto ";
                  s += typeNameDesc->GetDescription();
                }
                if (nameDesc && nameDesc->HasName()) {
                  s += " " + nameDesc->GetDescription();
                }
                lines.back() += s;
                AppendDistanceTime(lines.back());
              }

              void OnTargetReached(
                  const osmscout::RouteDescription::TargetDescriptionRef &targetDesc) override
              {
                NextLine();
                lines.back() += "Destination: " + targetDesc->GetDescription();
                AppendDistanceTime(lines.back());
              }
            };

            DescCallback descCb;
            osmscout::RouteDescriptionPostprocessor generator;
            generator.GenerateDescription(*descResult.GetDescription(), descCb);

            // Store description lines for marshalling
            routeDescriptionLines = std::move(descCb.lines);
            osmscout::log.Warn() << "calculateRouteAsync: generated "
                                 << routeDescriptionLines.size() << " description lines";

            // Keep a copy of the route description for live navigation
            if (descResult.GetDescription()) {
              routeDescription = std::make_shared<osmscout::RouteDescription>(
                  *descResult.GetDescription());
            }
          } else {
            osmscout::log.Warn() << "calculateRouteAsync: no route description generated";
          }

          routingService->Close();
        }
      );

      if (breaker->IsAborted()) {
        threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onCancel);
        threadEnv->DeleteGlobalRef(callbackGlobal);
        jvm->DetachCurrentThread();
        return;
      }

      if (!success) {
        threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onError,
                                  threadEnv->NewStringUTF(errorMsg.c_str()));
        threadEnv->DeleteGlobalRef(callbackGlobal);
        jvm->DetachCurrentThread();
        return;
      }

      // Store route description for navigation and assign a handle
      long routeHandle = 0;
      if (routeDescription) {
        std::scoped_lock lock(data->routeDescriptionMutex);
        routeHandle = data->nextRouteHandle++;
        data->routeDescriptions[routeHandle] = routeDescription;
      }

      // Transform route data to points
      osmscout::RoutePointsResult pointsResult;
      data->dbThread->RunSynchronousJob(
        [&](const std::list<osmscout::DBInstanceRef> &databases) {
          if (databases.empty()) return;

          std::vector<osmscout::DatabaseRef> dbs;
          dbs.reserve(databases.size());
          for (auto &inst : databases) {
            dbs.push_back(inst->GetDatabase());
          }

          osmscout::RouterParameter routerParam;
          osmscout::MultiDBRoutingServiceRef routingService =
              std::make_shared<osmscout::MultiDBRoutingService>(routerParam, dbs);

          osmscout::MultiDBRoutingService::RoutingProfileBuilder profileBuilder =
              [](const osmscout::DatabaseRef &database) -> osmscout::RoutingProfileRef {
            return std::make_shared<osmscout::FastestPathRoutingProfile>(
                database->GetTypeConfig());
          };

          if (!routingService->Open(profileBuilder)) return;

          pointsResult = routingService->TransformRouteDataToPoints(routeData);
          routingService->Close();
        }
      );

      if (!pointsResult.Success() || !pointsResult.GetPoints()) {
        osmscout::log.Error() << "calculateRouteAsync: failed to transform route data to points";
        threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onError,
                                  threadEnv->NewStringUTF("Failed to transform route data"));
        threadEnv->DeleteGlobalRef(callbackGlobal);
        jvm->DetachCurrentThread();
        return;
      }

      // Build RouteEntry Java object
      jclass routeEntryCls = threadEnv->FindClass(
          "com/framstag/libosmscout/client/RouteEntry");
      jmethodID routeEntryCtor = threadEnv->GetMethodID(routeEntryCls, "<init>", "()V");
      jobject routeEntry = threadEnv->NewObject(routeEntryCls, routeEntryCtor);

      const auto &points = pointsResult.GetPoints()->points;
      jsize count = static_cast<jsize>(points.size());

      // Fill latitudes array
      jdoubleArray lats = threadEnv->NewDoubleArray(count);
      std::vector<jdouble> latValues(count);
      for (jsize i = 0; i < count; i++) {
        latValues[i] = points[static_cast<size_t>(i)].GetLat();
      }
      threadEnv->SetDoubleArrayRegion(lats, 0, count, latValues.data());

      // Fill longitudes array
      jdoubleArray lons = threadEnv->NewDoubleArray(count);
      std::vector<jdouble> lonValues(count);
      for (jsize i = 0; i < count; i++) {
        lonValues[i] = points[static_cast<size_t>(i)].GetLon();
      }
      threadEnv->SetDoubleArrayRegion(lons, 0, count, lonValues.data());

      // Set fields on RouteEntry
      jfieldID latsField = threadEnv->GetFieldID(routeEntryCls, "latitudes", "[D");
      jfieldID lonsField = threadEnv->GetFieldID(routeEntryCls, "longitudes", "[D");
      jfieldID distField = threadEnv->GetFieldID(routeEntryCls, "distance", "D");
      jfieldID durField = threadEnv->GetFieldID(routeEntryCls, "duration", "D");
      jfieldID descField = threadEnv->GetFieldID(routeEntryCls, "descriptions", "[Ljava/lang/String;");
      jfieldID handleField = threadEnv->GetFieldID(routeEntryCls, "routeHandle", "J");

      threadEnv->SetObjectField(routeEntry, latsField, lats);
      threadEnv->SetObjectField(routeEntry, lonsField, lons);
      threadEnv->SetDoubleField(routeEntry, distField, totalDistance);
      threadEnv->SetLongField(routeEntry, handleField, routeHandle);

      // Estimate duration: 50 km/h average speed (default car)
      double durationSec = (totalDistance / 1000.0) / 50.0 * 3600.0;
      threadEnv->SetDoubleField(routeEntry, durField, durationSec);

      // Marshal description lines
      jsize descCount = static_cast<jsize>(routeDescriptionLines.size());
      jobjectArray descArray = threadEnv->NewObjectArray(
          descCount,
          threadEnv->FindClass("java/lang/String"),
          nullptr);
      for (jsize i = 0; i < descCount; i++) {
        threadEnv->SetObjectArrayElement(descArray, i,
            threadEnv->NewStringUTF(routeDescriptionLines[i].c_str()));
      }
      threadEnv->SetObjectField(routeEntry, descField, descArray);

      // Call onSuccess
      threadEnv->CallVoidMethod(callbackGlobal, cbMethods.onSuccess, routeEntry);

      threadEnv->DeleteGlobalRef(callbackGlobal);
      jvm->DetachCurrentThread();
    }
  );

  {
    std::scoped_lock lock(data->routingMutex);
    data->routingThread = thread;
  }
}

// --------------------------------------------------------------------------
// OSMScoutClient::cancelRoute()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_cancelRoute(JNIEnv *env, jobject self)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr) {
    return;
  }

  std::scoped_lock lock(data->routingMutex);
  if (data->breaker) {
    data->breaker->Break();
  }
}

// --------------------------------------------------------------------------
// Favorite Location JNI methods
// --------------------------------------------------------------------------

static jobject toJavaFavLocation(JNIEnv *env, const osmscout::FavLocation &fav)
{
  jclass cls = env->FindClass("com/framstag/libosmscout/client/FavoriteLocation");
  jmethodID ctor = env->GetMethodID(cls, "<init>", "()V");
  jobject obj = env->NewObject(cls, ctor);

  jfieldID nameField = env->GetFieldID(cls, "name", "Ljava/lang/String;");
  jfieldID latField = env->GetFieldID(cls, "lat", "D");
  jfieldID lonField = env->GetFieldID(cls, "lon", "D");

  env->SetObjectField(obj, nameField, env->NewStringUTF(fav.name.c_str()));
  env->SetDoubleField(obj, latField, fav.lat);
  env->SetDoubleField(obj, lonField, fav.lon);

  return obj;
}

static jobject toJavaFavGroup(JNIEnv *env, const osmscout::FavLocationGroup &group)
{
  jclass cls = env->FindClass("com/framstag/libosmscout/client/FavoriteLocationGroup");
  jmethodID ctor = env->GetMethodID(cls, "<init>", "()V");
  jobject obj = env->NewObject(cls, ctor);

  jfieldID nameField = env->GetFieldID(cls, "name", "Ljava/lang/String;");
  jfieldID favsField = env->GetFieldID(cls, "favorites", "Ljava/util/List;");

  env->SetObjectField(obj, nameField, env->NewStringUTF(group.name.c_str()));

  // Create Java ArrayList
  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID listCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  jmethodID addMethod = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");
  jobject list = env->NewObject(arrayListCls, listCtor);

  for (const auto &fav : group.favorites) {
    jobject favObj = toJavaFavLocation(env, fav);
    env->CallBooleanMethod(list, addMethod, favObj);
    env->DeleteLocalRef(favObj);
  }

  env->SetObjectField(obj, favsField, list);
  env->DeleteLocalRef(list);

  return obj;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_loadFavoriteLocations(JNIEnv *env, jobject self, jstring filePath)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr) {
    return JNI_FALSE;
  }

  const char *pathCStr = env->GetStringUTFChars(filePath, nullptr);
  if (pathCStr == nullptr) {
    return JNI_FALSE;
  }

  // Create or recreate the service with the given path
  delete data->favService;
  data->favService = new osmscout::FavoriteLocationService(pathCStr);

  env->ReleaseStringUTFChars(filePath, pathCStr);
  return JNI_TRUE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_saveFavoriteLocations(JNIEnv *env, jobject self, jstring filePath, jobjectArray groupsArray)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *pathCStr = env->GetStringUTFChars(filePath, nullptr);
  if (pathCStr == nullptr) {
    return JNI_FALSE;
  }

  // Convert Java groups to C++ groups
  jclass groupCls = env->FindClass("com/framstag/libosmscout/client/FavoriteLocationGroup");
  jfieldID groupNameField = env->GetFieldID(groupCls, "name", "Ljava/lang/String;");
  jfieldID groupFavsField = env->GetFieldID(groupCls, "favorites", "Ljava/util/List;");

  jclass favCls = env->FindClass("com/framstag/libosmscout/client/FavoriteLocation");
  jfieldID favNameField = env->GetFieldID(favCls, "name", "Ljava/lang/String;");
  jfieldID favLatField = env->GetFieldID(favCls, "lat", "D");
  jfieldID favLonField = env->GetFieldID(favCls, "lon", "D");

  jclass listCls = env->FindClass("java/util/List");
  jmethodID listSizeMethod = env->GetMethodID(listCls, "size", "()I");
  jmethodID listGetMethod = env->GetMethodID(listCls, "get", "(I)Ljava/lang/Object;");

  jsize len = groupsArray ? env->GetArrayLength(groupsArray) : 0;

  // Rebuild service with new data
  delete data->favService;
  auto *service = new osmscout::FavoriteLocationService(pathCStr);

  for (jsize i = 0; i < len; i++) {
    jobject groupObj = env->GetObjectArrayElement(groupsArray, i);
    if (groupObj == nullptr) continue;

    jstring groupNameJStr = (jstring)env->GetObjectField(groupObj, groupNameField);
    const char *groupNameCStr = env->GetStringUTFChars(groupNameJStr, nullptr);
    std::string groupName(groupNameCStr);
    env->ReleaseStringUTFChars(groupNameJStr, groupNameCStr);

    service->AddGroup(groupName);

    jobject favList = env->GetObjectField(groupObj, groupFavsField);
    if (favList != nullptr) {
      jint favCount = env->CallIntMethod(favList, listSizeMethod);
      for (jint j = 0; j < favCount; j++) {
        jobject favObj = env->CallObjectMethod(favList, listGetMethod, j);
        if (favObj == nullptr) continue;

        jstring favNameJStr = (jstring)env->GetObjectField(favObj, favNameField);
        jdouble lat = env->GetDoubleField(favObj, favLatField);
        jdouble lon = env->GetDoubleField(favObj, favLonField);

        const char *favNameCStr = env->GetStringUTFChars(favNameJStr, nullptr);
        osmscout::FavLocation fav;
        fav.name = favNameCStr;
        fav.lat = lat;
        fav.lon = lon;
        env->ReleaseStringUTFChars(favNameJStr, favNameCStr);

        service->AddFavorite(groupName, fav);

        env->DeleteLocalRef(favObj);
      }
      env->DeleteLocalRef(favList);
    }

    env->DeleteLocalRef(groupObj);
  }

  bool ok = service->Save();
  data->favService = service;

  env->ReleaseStringUTFChars(filePath, pathCStr);
  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_getFavoriteGroups(JNIEnv *env, jobject self)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return nullptr;
  }

  auto groups = data->favService->GetGroups();

  jclass groupCls = env->FindClass("com/framstag/libosmscout/client/FavoriteLocationGroup");
  jobjectArray result = env->NewObjectArray((jsize)groups.size(), groupCls, nullptr);

  for (size_t i = 0; i < groups.size(); i++) {
    jobject groupObj = toJavaFavGroup(env, groups[i]);
    env->SetObjectArrayElement(result, (jsize)i, groupObj);
    env->DeleteLocalRef(groupObj);
  }

  return result;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_addGroup(JNIEnv *env, jobject self, jstring name)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *nameCStr = env->GetStringUTFChars(name, nullptr);
  bool ok = data->favService->AddGroup(nameCStr);
  env->ReleaseStringUTFChars(name, nameCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_deleteGroup(JNIEnv *env, jobject self, jstring name)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *nameCStr = env->GetStringUTFChars(name, nullptr);
  bool ok = data->favService->DeleteGroup(nameCStr);
  env->ReleaseStringUTFChars(name, nameCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_addFavorite(JNIEnv *env, jobject self,
                                                                 jstring groupName, jstring favName,
                                                                 jdouble lat, jdouble lon)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);
  const char *favCStr = env->GetStringUTFChars(favName, nullptr);

  osmscout::FavLocation fav;
  fav.name = favCStr;
  fav.lat = lat;
  fav.lon = lon;

  bool ok = data->favService->AddFavorite(groupCStr, fav);

  env->ReleaseStringUTFChars(groupName, groupCStr);
  env->ReleaseStringUTFChars(favName, favCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_deleteFavorite(JNIEnv *env, jobject self,
                                                                    jstring groupName, jstring favName)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);
  const char *favCStr = env->GetStringUTFChars(favName, nullptr);

  bool ok = data->favService->DeleteFavorite(groupCStr, favCStr);

  env->ReleaseStringUTFChars(groupName, groupCStr);
  env->ReleaseStringUTFChars(favName, favCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_renameFavorite(JNIEnv *env, jobject self,
                                                                    jstring groupName, jstring oldName,
                                                                    jstring newName)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);
  const char *oldCStr = env->GetStringUTFChars(oldName, nullptr);
  const char *newCStr = env->GetStringUTFChars(newName, nullptr);

  bool ok = data->favService->RenameFavorite(groupCStr, oldCStr, newCStr);

  env->ReleaseStringUTFChars(groupName, groupCStr);
  env->ReleaseStringUTFChars(oldName, oldCStr);
  env->ReleaseStringUTFChars(newName, newCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_setStarred(JNIEnv *env, jobject self,
                                                                jstring groupName, jstring favName,
                                                                jboolean starred)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);
  const char *favCStr = env->GetStringUTFChars(favName, nullptr);

  bool ok = data->favService->SetStarred(groupCStr, favCStr, starred == JNI_TRUE);

  env->ReleaseStringUTFChars(groupName, groupCStr);
  env->ReleaseStringUTFChars(favName, favCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_isStarred(JNIEnv *env, jobject self,
                                                               jstring groupName, jstring favName)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);
  const char *favCStr = env->GetStringUTFChars(favName, nullptr);

  bool starred = data->favService->IsStarred(groupCStr, favCStr);

  env->ReleaseStringUTFChars(groupName, groupCStr);
  env->ReleaseStringUTFChars(favName, favCStr);

  return starred ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_setGroupColor(JNIEnv *env, jobject self,
                                                                    jstring groupName, jstring color)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return JNI_FALSE;
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);
  const char *colorCStr = env->GetStringUTFChars(color, nullptr);

  bool ok = data->favService->SetGroupColor(groupCStr, colorCStr);

  env->ReleaseStringUTFChars(groupName, groupCStr);
  env->ReleaseStringUTFChars(color, colorCStr);

  return ok ? JNI_TRUE : JNI_FALSE;
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_getGroupColor(JNIEnv *env, jobject self,
                                                                   jstring groupName)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->favService == nullptr) {
    return env->NewStringUTF("");
  }

  const char *groupCStr = env->GetStringUTFChars(groupName, nullptr);

  std::string color = data->favService->GetGroupColor(groupCStr);

  env->ReleaseStringUTFChars(groupName, groupCStr);

  return env->NewStringUTF(color.c_str());
}

// --------------------------------------------------------------------------
// OSMScoutClient::startNavigation(long routeHandle, NavigationListener)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_startNavigation(JNIEnv *env,
                                                                    jobject self,
                                                                    jlong routeHandle,
                                                                    jobject listener)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    osmscout::log.Error() << "startNavigation: client not initialised";
    return nullptr;
  }

  if (routeHandle == 0 || listener == nullptr) {
    osmscout::log.Error() << "startNavigation: invalid route handle or listener";
    return nullptr;
  }

  osmscout::RouteDescriptionRef routeDescription;
  {
    std::scoped_lock lock(data->routeDescriptionMutex);
    auto it = data->routeDescriptions.find(static_cast<long>(routeHandle));
    if (it == data->routeDescriptions.end()) {
      osmscout::log.Error() << "startNavigation: route handle " << routeHandle << " not found";
      return nullptr;
    }
    routeDescription = it->second;
  }

  if (!routeDescription) {
    osmscout::log.Error() << "startNavigation: route description is null";
    return nullptr;
  }

  JavaVM *jvm = nullptr;
  if (env->GetJavaVM(&jvm) != JNI_OK) {
    osmscout::log.Error() << "startNavigation: failed to get JavaVM";
    return nullptr;
  }

  jobject listenerGlobal = env->NewGlobalRef(listener);
  if (listenerGlobal == nullptr) {
    osmscout::log.Error() << "startNavigation: failed to create global ref";
    return nullptr;
  }

  NavigationListenerMethods methods;
  if (!GetNavigationListenerMethods(env, listenerGlobal, methods)) {
    osmscout::log.Error() << "startNavigation: failed to resolve listener methods";
    env->DeleteGlobalRef(listenerGlobal);
    return nullptr;
  }

  auto controller = std::make_shared<JavaNavigationController>(
      data, routeDescription, osmscout::vehicleCar, jvm, listenerGlobal, methods);

  jclass controllerCls = env->FindClass("com/framstag/libosmscout/client/NavigationController");
  if (controllerCls == nullptr) {
    osmscout::log.Error() << "startNavigation: NavigationController class not found";
    return nullptr;
  }
  jmethodID controllerCtor = env->GetMethodID(controllerCls, "<init>", "()V");
  if (controllerCtor == nullptr) {
    osmscout::log.Error() << "startNavigation: NavigationController constructor not found";
    return nullptr;
  }
  jobject controllerObj = env->NewObject(controllerCls, controllerCtor);
  if (controllerObj == nullptr) {
    osmscout::log.Error() << "startNavigation: failed to create NavigationController object";
    return nullptr;
  }

  // Store the C++ controller pointer in the Java object's nativeHandle field
  jfieldID handleField = env->GetFieldID(controllerCls, "nativeHandle", "J");
  env->SetLongField(controllerObj, handleField,
                    static_cast<jlong>(reinterpret_cast<intptr_t>(controller.get())));

  // Keep controller alive in ClientData and start its thread
  {
    std::scoped_lock lock(data->routeDescriptionMutex);
    data->navigationControllers[controller.get()] = controller;
  }
  controller->Start();

  return controllerObj;
}

// --------------------------------------------------------------------------
// OSMScoutClient::startNavigation(long routeHandle, Vehicle, NavigationListener)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_startNavigationWithVehicle(
    JNIEnv *env,
    jobject self,
    jlong routeHandle,
    jobject vehicleObj,
    jobject listener)
{
  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    osmscout::log.Error() << "startNavigation: client not initialised";
    return nullptr;
  }

  if (routeHandle == 0 || listener == nullptr) {
    osmscout::log.Error() << "startNavigation: invalid route handle or listener";
    return nullptr;
  }

  osmscout::RouteDescriptionRef routeDescription;
  {
    std::scoped_lock lock(data->routeDescriptionMutex);
    auto it = data->routeDescriptions.find(static_cast<long>(routeHandle));
    if (it == data->routeDescriptions.end()) {
      osmscout::log.Error() << "startNavigation: route handle " << routeHandle << " not found";
      return nullptr;
    }
    routeDescription = it->second;
  }

  if (!routeDescription) {
    osmscout::log.Error() << "startNavigation: route description is null";
    return nullptr;
  }

  // Convert Java Vehicle to C++ vehicle
  osmscout::Vehicle vehicle = JavaVehicleToCpp(env, vehicleObj);

  JavaVM *jvm = nullptr;
  if (env->GetJavaVM(&jvm) != JNI_OK) {
    osmscout::log.Error() << "startNavigation: failed to get JavaVM";
    return nullptr;
  }

  jobject listenerGlobal = env->NewGlobalRef(listener);
  if (listenerGlobal == nullptr) {
    osmscout::log.Error() << "startNavigation: failed to create global ref";
    return nullptr;
  }

  NavigationListenerMethods methods;
  if (!GetNavigationListenerMethods(env, listenerGlobal, methods)) {
    osmscout::log.Error() << "startNavigation: failed to resolve listener methods";
    env->DeleteGlobalRef(listenerGlobal);
    return nullptr;
  }

  auto controller = std::make_shared<JavaNavigationController>(
      data, routeDescription, vehicle, jvm, listenerGlobal, methods);

  jclass controllerCls = env->FindClass("com/framstag/libosmscout/client/NavigationController");
  if (controllerCls == nullptr) {
    osmscout::log.Error() << "startNavigation: NavigationController class not found";
    return nullptr;
  }
  jmethodID controllerCtor = env->GetMethodID(controllerCls, "<init>", "()V");
  if (controllerCtor == nullptr) {
    osmscout::log.Error() << "startNavigation: NavigationController constructor not found";
    return nullptr;
  }
  jobject controllerObj = env->NewObject(controllerCls, controllerCtor);
  if (controllerObj == nullptr) {
    osmscout::log.Error() << "startNavigation: failed to create NavigationController object";
    return nullptr;
  }

  // Store the C++ controller pointer in the Java object's nativeHandle field
  jfieldID handleField = env->GetFieldID(controllerCls, "nativeHandle", "J");
  env->SetLongField(controllerObj, handleField,
                    static_cast<jlong>(reinterpret_cast<intptr_t>(controller.get())));

  // Keep controller alive in ClientData and start its thread
  {
    std::scoped_lock lock(data->routeDescriptionMutex);
    data->navigationControllers[controller.get()] = controller;
  }
  controller->Start();

  return controllerObj;
}

// --------------------------------------------------------------------------
// NavigationController::stop()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_NavigationController_stop(JNIEnv *env,
                                                               jobject self)
{
  jfieldID handleField = env->GetFieldID(
      env->GetObjectClass(self), "nativeHandle", "J");
  jlong handle = env->GetLongField(self, handleField);
  if (handle == 0) {
    return;
  }

  auto *controller = reinterpret_cast<JavaNavigationController *>(static_cast<intptr_t>(handle));
  if (controller == nullptr) {
    return;
  }

  controller->Stop();
  env->SetLongField(self, handleField, 0);

  ClientData *data = controller->GetClientData();
  if (data != nullptr) {
    std::scoped_lock lock(data->routeDescriptionMutex);
    data->navigationControllers.erase(controller);
  }
}

// --------------------------------------------------------------------------
// NavigationController::processLocation(...)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_NavigationController_processLocation(JNIEnv *env,
                                                                          jobject self,
                                                                          jdouble lat,
                                                                          jdouble lon,
                                                                          jdouble speed,
                                                                          jdouble accuracy,
                                                                          jlong timestamp)
{
  jfieldID handleField = env->GetFieldID(
      env->GetObjectClass(self), "nativeHandle", "J");
  jlong handle = env->GetLongField(self, handleField);
  if (handle == 0) {
    return;
  }

  auto *controller = reinterpret_cast<JavaNavigationController *>(static_cast<intptr_t>(handle));
  if (controller == nullptr) {
    return;
  }

  using namespace std::chrono;
  auto tp = system_clock::time_point(milliseconds(static_cast<long long>(timestamp)));
  controller->ProcessLocation(lat, lon, speed, accuracy, tp);
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativeParseMapList(String, MapProvider)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativeParseMapList(
    JNIEnv *env, jobject self, jstring jsonJStr, jobject providerObj)
{
  (void)self;

  const char *jsonCStr = jsonJStr ? env->GetStringUTFChars(jsonJStr, nullptr) : "";
  if (!jsonCStr || strlen(jsonCStr) == 0) {
    if (jsonJStr) env->ReleaseStringUTFChars(jsonJStr, jsonCStr);
    // Return empty list
    jclass arrayListCls = env->FindClass("java/util/ArrayList");
    jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
    return env->NewObject(arrayListCls, arrayListCtor);
  }

  // Read provider fields
  jclass providerCls = env->GetObjectClass(providerObj);
  jfieldID nameField = env->GetFieldID(providerCls, "name", "Ljava/lang/String;");
  jfieldID uriField = env->GetFieldID(providerCls, "uri", "Ljava/lang/String;");
  jfieldID listUriField = env->GetFieldID(providerCls, "listUri", "Ljava/lang/String;");

  jstring nameJStr = (jstring)env->GetObjectField(providerObj, nameField);
  jstring uriJStr = (jstring)env->GetObjectField(providerObj, uriField);
  jstring listUriJStr = (jstring)env->GetObjectField(providerObj, listUriField);

  const char *nameCStr = nameJStr ? env->GetStringUTFChars(nameJStr, nullptr) : "";
  const char *uriCStr = uriJStr ? env->GetStringUTFChars(uriJStr, nullptr) : "";
  const char *listUriCStr = listUriJStr ? env->GetStringUTFChars(listUriJStr, nullptr) : "";

  osmscout::MapProvider provider(nameCStr, uriCStr, listUriCStr);

  if (nameJStr) env->ReleaseStringUTFChars(nameJStr, nameCStr);
  if (uriJStr) env->ReleaseStringUTFChars(uriJStr, uriCStr);
  if (listUriJStr) env->ReleaseStringUTFChars(listUriJStr, listUriCStr);

  // Parse JSON
  std::string jsonStr(jsonCStr);
  env->ReleaseStringUTFChars(jsonJStr, jsonCStr);

  std::vector<osmscout::AvailableMapEntry> entries;
  try {
    auto json = nlohmann::json::parse(jsonStr);
    entries = osmscout::AvailableMapEntry::FromJsonArray(json, provider);
  } catch (const nlohmann::json::exception &e) {
    osmscout::log.Error() << "nativeParseMapList: JSON parse error: " << e.what();
    jclass arrayListCls = env->FindClass("java/util/ArrayList");
    jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
    return env->NewObject(arrayListCls, arrayListCtor);
  }

  // Convert to Java List<AvailableMapEntry>
  jclass entryCls = env->FindClass("com/framstag/libosmscout/client/AvailableMapEntry");
  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  jmethodID addMethod = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");

  jobject resultList = env->NewObject(arrayListCls, arrayListCtor);

  for (const auto &entry : entries) {
    jmethodID entryCtor;
    jobject entryObj;

    jstring entryName = env->NewStringUTF(entry.GetName().c_str());
    jstring entryDesc = env->NewStringUTF(entry.GetDescription().c_str());

    // Build path list
    jobject pathList = env->NewObject(arrayListCls, arrayListCtor);
    for (const auto &seg : entry.GetPath()) {
      jstring segStr = env->NewStringUTF(seg.c_str());
      env->CallBooleanMethod(pathList, addMethod, segStr);
      env->DeleteLocalRef(segStr);
    }

    if (entry.IsDirectory()) {
      entryCtor = env->GetMethodID(entryCls, "<init>",
                                    "(Ljava/lang/String;Ljava/util/List;Ljava/lang/String;)V");
      entryObj = env->NewObject(entryCls, entryCtor, entryName, pathList, entryDesc);
    } else {
      // Build provider object
      jclass provCls = env->FindClass("com/framstag/libosmscout/client/MapProvider");
      jmethodID provCtor = env->GetMethodID(provCls, "<init>",
                                              "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)V");
      jstring provName = env->NewStringUTF(entry.GetProvider().getName().c_str());
      jstring provUri = env->NewStringUTF(entry.GetProvider().getUri().c_str());
      jstring provListUri = env->NewStringUTF(entry.GetProvider().getListUri(0, 0).c_str());
      jobject provObj = env->NewObject(provCls, provCtor, provName, provUri, provListUri);
      env->DeleteLocalRef(provName);
      env->DeleteLocalRef(provUri);
      env->DeleteLocalRef(provListUri);

      jstring serverDir = env->NewStringUTF(entry.GetServerDirectory().c_str());

      entryCtor = env->GetMethodID(entryCls, "<init>",
                                    "(Ljava/lang/String;Ljava/util/List;Ljava/lang/String;"
                                    "Lcom/framstag/libosmscout/client/MapProvider;J"
                                    "Ljava/lang/String;JI)V");
      entryObj = env->NewObject(entryCls, entryCtor,
                                 entryName, pathList, entryDesc,
                                 provObj,
                                 static_cast<jlong>(entry.GetSize()),
                                 serverDir,
                                 static_cast<jlong>(std::chrono::duration_cast<std::chrono::seconds>(
                                   entry.GetCreation().time_since_epoch()).count()),
                                 static_cast<jint>(entry.GetVersion()));
      env->DeleteLocalRef(provObj);
      env->DeleteLocalRef(serverDir);
    }

    env->DeleteLocalRef(entryName);
    env->DeleteLocalRef(entryDesc);
    env->DeleteLocalRef(pathList);

    env->CallBooleanMethod(resultList, addMethod, entryObj);
    env->DeleteLocalRef(entryObj);
  }

  return resultList;
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativeGetMapFileNames()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativeGetMapFileNames(
    JNIEnv *env, jobject self)
{
  (void)self;

  std::vector<std::string> files = osmscout::MapDownloadService::MapFiles();

  jclass stringClass = env->FindClass("java/lang/String");
  jobjectArray result = env->NewObjectArray(static_cast<jsize>(files.size()), stringClass, nullptr);

  for (size_t i = 0; i < files.size(); ++i) {
    jstring str = env->NewStringUTF(files[i].c_str());
    env->SetObjectArrayElement(result, static_cast<jsize>(i), str);
    env->DeleteLocalRef(str);
  }

  env->DeleteLocalRef(stringClass);
  return result;
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativePrepareMapDirectory(AvailableMapEntry, String)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativePrepareMapDirectory(
    JNIEnv *env, jobject self, jobject entryObj, jstring targetDirJStr)
{
  (void)self;

  if (!entryObj || !targetDirJStr) {
    return JNI_FALSE;
  }

  jclass entryCls = env->GetObjectClass(entryObj);
  jfieldID nameField = env->GetFieldID(entryCls, "name", "Ljava/lang/String;");
  jfieldID sizeField = env->GetFieldID(entryCls, "size", "J");
  jfieldID serverDirField = env->GetFieldID(entryCls, "serverDirectory", "Ljava/lang/String;");
  jfieldID versionField = env->GetFieldID(entryCls, "version", "I");
  jfieldID creationField = env->GetFieldID(entryCls, "creationTimestamp", "J");

  jstring nameJStr = (jstring)env->GetObjectField(entryObj, nameField);
  jstring serverDirJStr = (jstring)env->GetObjectField(entryObj, serverDirField);
  jlong size = env->GetLongField(entryObj, sizeField);
  jint version = env->GetIntField(entryObj, versionField);
  jlong creationSecs = env->GetLongField(entryObj, creationField);

  const char *nameCStr = nameJStr ? env->GetStringUTFChars(nameJStr, nullptr) : "";
  const char *serverDirCStr = serverDirJStr ? env->GetStringUTFChars(serverDirJStr, nullptr) : "";
  const char *targetDirCStr = env->GetStringUTFChars(targetDirJStr, nullptr);

  osmscout::AvailableMapEntry entry(
    nameCStr,
    {},
    "",
    osmscout::MapProvider(),
    static_cast<uint64_t>(size),
    serverDirCStr ? serverDirCStr : "",
    osmscout::Timestamp(std::chrono::seconds(static_cast<uint64_t>(creationSecs))),
    static_cast<int>(version)
  );

  if (nameJStr) env->ReleaseStringUTFChars(nameJStr, nameCStr);
  if (serverDirJStr) env->ReleaseStringUTFChars(serverDirJStr, serverDirCStr);

  std::string targetDir = targetDirCStr ? targetDirCStr : "";
  env->ReleaseStringUTFChars(targetDirJStr, targetDirCStr);

  env->DeleteLocalRef(entryCls);

  bool ok = osmscout::MapDownloadService::PrepareMapDirectory(
      entry, std::filesystem::path(targetDir));

  return ok ? JNI_TRUE : JNI_FALSE;
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativeRegisterMapDirectory(String)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativeRegisterMapDirectory(
    JNIEnv *env, jobject self, jstring targetDirJStr)
{
  (void)env;
  (void)self;

  ClientData *data = activeClient;
  if (!data || !data->mapManager || !targetDirJStr) {
    return JNI_FALSE;
  }

  const char *targetDirCStr = env->GetStringUTFChars(targetDirJStr, nullptr);
  std::string targetDir = targetDirCStr ? targetDirCStr : "";
  env->ReleaseStringUTFChars(targetDirJStr, targetDirCStr);

  bool ok = osmscout::MapDownloadService::RegisterMapDirectory(
      std::filesystem::path(targetDir), data->mapManager);

  return ok ? JNI_TRUE : JNI_FALSE;
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativeCancelDownload(String)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT void JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativeCancelDownload(
    JNIEnv *env, jobject self, jstring handleJStr)
{
  (void)env;
  (void)self;
  (void)handleJStr;
  // Cancellation is handled on the Java side; the download worker no longer
  // uses the native MapDownloadService/AsyncWorker path.
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativeGetInstalledMaps()
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jobject JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativeGetInstalledMaps(
    JNIEnv *env, jobject self)
{
  (void)self;
  ClientData *data = activeClient;
  if (!data || !data->mapManager) {
    return nullptr;
  }

  jclass arrayListCls = env->FindClass("java/util/ArrayList");
  jmethodID arrayListCtor = env->GetMethodID(arrayListCls, "<init>", "()V");
  jmethodID addMethod = env->GetMethodID(arrayListCls, "add", "(Ljava/lang/Object;)Z");

  jobject resultList = env->NewObject(arrayListCls, arrayListCtor);

  auto dirs = data->mapManager->GetDatabaseDirectories();
  for (const auto &dir : dirs) {
    jstring dirStr = env->NewStringUTF(dir.GetDirStr().c_str());
    env->CallBooleanMethod(resultList, addMethod, dirStr);
    env->DeleteLocalRef(dirStr);
  }

  return resultList;
}

// --------------------------------------------------------------------------
// MapDownloadManager::nativeDeleteMap(String)
// --------------------------------------------------------------------------

extern "C" JNIEXPORT jboolean JNICALL
Java_com_framstag_libosmscout_client_MapDownloadManager_nativeDeleteMap(
    JNIEnv *env, jobject self, jstring pathJStr)
{
  (void)self;
  ClientData *data = activeClient;
  if (!data) {
    return JNI_FALSE;
  }

  const char *pathCStr = env->GetStringUTFChars(pathJStr, nullptr);
  if (!pathCStr) {
    return JNI_FALSE;
  }

  osmscout::MapDirectory mapDir{std::filesystem::path(pathCStr)};
  bool result = mapDir.DeleteDatabase();

  env->ReleaseStringUTFChars(pathJStr, pathCStr);

  if (result && data->mapManager) {
    data->mapManager->LookupDatabases();
  }

  return result ? JNI_TRUE : JNI_FALSE;
}

// --------------------------------------------------------------------------
// OSMScoutClient::searchPOIsByTypes(String[] typeNames, double lat, double lon,
//                                   double radiusMeters, int limit)
// --------------------------------------------------------------------------

namespace {
  // A single POI search result ready to be serialized into a Java PoiEntry.
  struct PoiEntry {
    std::string label;
    std::string objectType;
    double      lat{0.0};
    double      lon{0.0};
    double      distance{0.0};
  };

  // Fill a PoiEntry from a node/way/area object. The label falls back from
  // the name feature to the operator and ref features (same as POILookupModule).
  template<class T>
  bool BuildPoiEntry(const T& obj, const osmscout::GeoCoord& center, PoiEntry& entry)
  {
    if (!obj) {
      return false;
    }

    entry.objectType = obj->GetType()->GetName();

    const osmscout::FeatureValueBuffer& features = obj->GetFeatureValueBuffer();
    if (const auto* name = features.findValue<osmscout::NameFeatureValue>(); name != nullptr) {
      entry.label = name->GetLabel(osmscout::Locale(), 0);
    } else if (const auto* op = features.findValue<osmscout::OperatorFeatureValue>(); op != nullptr) {
      entry.label = op->GetLabel(osmscout::Locale(), 0);
    } else if (const auto* ref = features.findValue<osmscout::RefFeatureValue>(); ref != nullptr) {
      entry.label = ref->GetLabel(osmscout::Locale(), 0);
    }

    osmscout::GeoCoord coord;
    if constexpr (std::is_same_v<T, osmscout::NodeRef>) {
      coord = obj->GetCoords();
    } else {
      coord = obj->GetBoundingBox().GetCenter();
    }

    entry.lat = coord.GetLat();
    entry.lon = coord.GetLon();
    entry.distance = center.GetDistance(coord).AsMeter();
    return true;
  }
}

extern "C" JNIEXPORT jobjectArray JNICALL
Java_com_framstag_libosmscout_client_OSMScoutClient_searchPOIsByTypes(JNIEnv *env, jobject self,
                                                                      jobjectArray typeNamesJArray,
                                                                      jdouble lat, jdouble lon,
                                                                      jdouble radiusMeters, jint limit)
{
  jclass entryCls = env->FindClass("com/framstag/libosmscout/client/PoiEntry");
  if (entryCls == nullptr) {
    return nullptr;
  }

  auto emptyResult = [&]() -> jobjectArray {
    return env->NewObjectArray(0, entryCls, nullptr);
  };

  ClientData *data = getClientData(env, self);
  if (data == nullptr || data->dbThread == nullptr) {
    return emptyResult();
  }

  if (typeNamesJArray == nullptr || radiusMeters <= 0.0 || limit <= 0) {
    return emptyResult();
  }

  std::vector<std::string> typeNames;
  jsize typeCount = env->GetArrayLength(typeNamesJArray);
  for (jsize i = 0; i < typeCount; i++) {
    jstring typeJStr = (jstring)env->GetObjectArrayElement(typeNamesJArray, i);
    if (typeJStr == nullptr) {
      continue;
    }
    const char *typeCStr = env->GetStringUTFChars(typeJStr, nullptr);
    if (typeCStr != nullptr) {
      typeNames.emplace_back(typeCStr);
      env->ReleaseStringUTFChars(typeJStr, typeCStr);
    }
    env->DeleteLocalRef(typeJStr);
  }

  if (typeNames.empty()) {
    return emptyResult();
  }

  // A new search cancels the previously running search (same as searchLocations)
  {
    std::lock_guard<std::mutex> guard(g_searchMutex);
    if (g_currentBreaker) {
      g_currentBreaker->Break();
      g_currentBreaker = nullptr;
    }
    g_currentBreaker = std::make_shared<osmscout::ThreadedBreaker>();
  }

  std::vector<PoiEntry> entries;
  const osmscout::GeoCoord center(lat, lon);

  data->dbThread->RunSynchronousJob(
    [&](const std::list<osmscout::DBInstanceRef>& databases) {
      osmscout::BreakerRef breaker;
      {
        std::lock_guard<std::mutex> guard(g_searchMutex);
        breaker = g_currentBreaker;
      }

      for (const auto& db : databases) {
        if (breaker && breaker->IsAborted()) {
          break;
        }
        // The basemap is a low-zoom background map; it is not searched.
        if (IsBasemapDatabase(db)) {
          continue;
        }

        auto database = db->GetDatabase();
        if (!database) {
          continue;
        }
        auto typeConfig = database->GetTypeConfig();
        if (!typeConfig) {
          continue;
        }

        osmscout::TypeInfoSet nodeTypes;
        osmscout::TypeInfoSet wayTypes;
        osmscout::TypeInfoSet areaTypes;

        for (const auto& typeName : typeNames) {
          osmscout::TypeInfoRef typeInfo = typeConfig->GetTypeInfo(typeName);
          if (!typeInfo) {
            osmscout::log.Warn() << "There is no type " << typeName
                                 << " in database " << db->path;
            continue;
          }
          if (typeInfo->CanBeArea()) {
            areaTypes.Set(typeInfo);
          }
          if (typeInfo->CanBeWay()) {
            wayTypes.Set(typeInfo);
          }
          if (typeInfo->CanBeNode()) {
            nodeTypes.Set(typeInfo);
          }
        }

        if (nodeTypes.Empty() && wayTypes.Empty() && areaTypes.Empty()) {
          continue;
        }

        std::vector<osmscout::NodeRef> nodes;
        std::vector<osmscout::WayRef> ways;
        std::vector<osmscout::AreaRef> areas;

        try {
          osmscout::POIService poiService(database);
          poiService.GetPOIsInRadius(center,
                                     osmscout::Distance::Of<osmscout::Meter>(radiusMeters),
                                     nodeTypes, nodes,
                                     wayTypes, ways,
                                     areaTypes, areas);
        } catch (const std::exception& e) {
          osmscout::log.Error() << "Failed to load POIs in radius: " << e.what();
          continue;
        }

        for (const auto& area : areas) {
          PoiEntry entry;
          if (BuildPoiEntry(area, center, entry)) {
            entries.push_back(std::move(entry));
          }
        }
        for (const auto& way : ways) {
          PoiEntry entry;
          if (BuildPoiEntry(way, center, entry)) {
            entries.push_back(std::move(entry));
          }
        }
        for (const auto& node : nodes) {
          PoiEntry entry;
          if (BuildPoiEntry(node, center, entry)) {
            entries.push_back(std::move(entry));
          }
        }

        if (static_cast<int>(entries.size()) >= limit) {
          break;
        }
      }
    });

  // Nearest first
  std::sort(entries.begin(), entries.end(),
            [](const PoiEntry& a, const PoiEntry& b) { return a.distance < b.distance; });

  if (entries.size() > static_cast<size_t>(limit)) {
    entries.resize(static_cast<size_t>(limit));
  }

  jmethodID entryCtor = env->GetMethodID(entryCls, "<init>", "()V");
  if (entryCtor == nullptr) {
    return nullptr;
  }
  jfieldID labelField = env->GetFieldID(entryCls, "label", "Ljava/lang/String;");
  jfieldID objectTypeField = env->GetFieldID(entryCls, "objectType", "Ljava/lang/String;");
  jfieldID latField = env->GetFieldID(entryCls, "lat", "D");
  jfieldID lonField = env->GetFieldID(entryCls, "lon", "D");
  jfieldID distanceField = env->GetFieldID(entryCls, "distance", "D");

  jobjectArray resultArray = env->NewObjectArray(static_cast<jsize>(entries.size()), entryCls, nullptr);
  if (resultArray == nullptr) {
    return nullptr;
  }

  for (jsize i = 0; i < static_cast<jsize>(entries.size()); i++) {
    const PoiEntry& entry = entries[static_cast<size_t>(i)];
    jobject jEntry = env->NewObject(entryCls, entryCtor);
    env->SetObjectField(jEntry, labelField, env->NewStringUTF(entry.label.c_str()));
    env->SetObjectField(jEntry, objectTypeField, env->NewStringUTF(entry.objectType.c_str()));
    env->SetDoubleField(jEntry, latField, entry.lat);
    env->SetDoubleField(jEntry, lonField, entry.lon);
    env->SetDoubleField(jEntry, distanceField, entry.distance);
    env->SetObjectArrayElement(resultArray, i, jEntry);
    env->DeleteLocalRef(jEntry);
  }

  return resultArray;
}