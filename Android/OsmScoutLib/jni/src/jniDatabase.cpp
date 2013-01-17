/*
  This source is part of the libosmscout library
  Copyright (C) 2010  Tim Teulings

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

#include <jni.h>
#include <string.h>
#include <android/log.h>

#include <osmscout/AdminRegion.h>
#include <osmscout/Database.h>
#include <osmscout/Node.h>
#include <osmscout/MapPainter.h>

#include <jniObjectArray.h>
#include <jniObjectTypeSets.h>

#define DEBUG_TAG "OsmScoutJni:Database"

using namespace osmscout;

extern JniObjectArray<Database>             *gDatabaseArray;
extern JniObjectArray<MapData>              *gMapDataArray;
extern JniObjectArray<ObjectTypeSets>       *gObjectTypeSetsArray;
extern JniObjectArray<TypeConfig>           *gTypeConfigArray;

#ifdef __cplusplus
extern "C" {
#endif

jint Java_osm_scout_Database_jniConstructor(JNIEnv *env, jobject object)
{
  DatabaseParameter databaseParameter;

  Database *nativeDatabase=new Database(databaseParameter);

  return gDatabaseArray->Add(nativeDatabase);
}

void Java_osm_scout_Database_jniDestructor(JNIEnv *env, jobject object,
                                           int databaseIndex)
{
  Database *nativeDatabase=gDatabaseArray->GetAndRemove(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL Database object");
  }
  else
    delete nativeDatabase;
}

jboolean Java_osm_scout_Database_jniOpen(JNIEnv *env, jobject object,
                                         int databaseIndex,  jstring javaPath)
{
  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniOpen(): NULL Database object");
    return JNI_FALSE;
  }

  jboolean isCopy;

  const char *nativePath=env->GetStringUTFChars(javaPath, &isCopy);

  bool result=nativeDatabase->Open(nativePath);

  env->ReleaseStringUTFChars(javaPath, nativePath);

  return result;
}

jboolean Java_osm_scout_Database_jniIsOpen(JNIEnv *env, jobject object,
                                           int databaseIndex)
{
  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniIsOpen(): NULL Database object");
    return JNI_FALSE;
  }

  return nativeDatabase->IsOpen();
}

jobject Java_osm_scout_Database_jniGetBoundingBox(JNIEnv *env, jobject object,
                                                  int databaseIndex)
{
  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetBoundingBox(): NULL Database object");
    return NULL;
  }

  double minLat, maxLat;
  double minLon, maxLon;
    
  if (!nativeDatabase->GetBoundingBox(minLat, minLon, maxLat, maxLon))
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetBoundingBox(): GetBoundingBox() returned FALSE");
    return NULL;
  }

  jclass javaClass = env->FindClass("osm/scout/GeoBox");
  jmethodID methodId = env->GetMethodID(javaClass,"<init>","(DDDD)V");
  jobject geoBox=env->NewObject(javaClass, methodId,
                                minLon, maxLon, minLat, maxLat);

  return geoBox;
}

jobjectArray Java_osm_scout_Database_jniGetMatchingAdminRegions(JNIEnv *env,
                      jobject object, int databaseIndex, jstring javaName,
                      int limit, jobject javaLimitReached, jboolean startWith)
{
  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetMatchingAdminRegions(): NULL Database object");
    return NULL;
  }

  jboolean isCopy;

  const char *nativeName=env->GetStringUTFChars(javaName, &isCopy);

  std::list<AdminRegion> nativeRegions;
  
  bool nativeLimitReached;

  jobjectArray javaRegions;

  if (nativeDatabase->GetMatchingAdminRegions(nativeName,
                                         nativeRegions,
                                         limit,
                                         nativeLimitReached,
                                         startWith))
  {
    jclass javaClass=env->FindClass("osm/scout/AdminRegion");

    javaRegions=env->NewObjectArray(nativeRegions.size(), javaClass, NULL);

    jmethodID methodId = env->GetMethodID(javaClass, "<init>",
                                          "(Ljava/lang/String;IJ)V");

    int pos=0;

    for(std::list<osmscout::AdminRegion>::iterator iter=nativeRegions.begin(); 
                                            iter!=nativeRegions.end(); iter++)
    {
      jint type=iter->reference.type;
      jlong fileOffset=iter->reference.GetFileOffset();

      jobject javaRegion=env->NewObject(javaClass, methodId,
                                    env->NewStringUTF(iter->name.c_str()),
                                    type, fileOffset);

      env->SetObjectArrayElement(javaRegions, pos, javaRegion);

      pos++;
    }

    // Set the <limitReached> value
    javaClass=env->FindClass("osm/scout/Bool");
    methodId=env->GetMethodID(javaClass, "set", "(Z)V");
    env->CallVoidMethod(javaLimitReached, methodId, nativeLimitReached);
  }
  else
  {
    // GetMatchingAdminRegions returned FALSE
    javaRegions=NULL;
  }

  env->ReleaseStringUTFChars(javaName, nativeName);

  return javaRegions;
}

/*
jobject Java_osm_scout_Database_jniGetNode(JNIEnv *env, jobject object,
                                           int databaseIndex,  long id)
{
  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetNode(): NULL Database object");
    return NULL;
  }

  osmscout::NodeRef nodeRef;

  nativeDatabase->GetNode(id, nodeRef);

  osmscout::Node* nativeNode=nodeRef.Get();

  jclass javaClass=env->FindClass("osm/scout/Node");
  
  jmethodID methodId=env->GetMethodID(javaClass,"<init>","(DD)V");
  
  jobject node=env->NewObject(javaClass, methodId,
                              nativeNode->GetLon(), nativeNode->GetLat());

  return node;			
}
*/

jobject Java_osm_scout_Database_jniGetObjects(JNIEnv *env, jobject object,
                                int databaseIndex, int objectTypeSetsIndex,
                                double lonMin, double latMin, double lonMax,
                                double latMax, double magnification)
{
  __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetObjects(): GetObjects() starts...");

  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetObjects(): NULL Database object");
    return NULL;
  }

  ObjectTypeSets *nativeObjectTypeSets=
                             gObjectTypeSetsArray->Get(objectTypeSetsIndex);

  if (!nativeObjectTypeSets)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetObjects(): NULL ObjectTypeSets object");
    return NULL;
  }

  AreaSearchParameter searchParameter;

  MapData *nativeMapData=new MapData();

  jobject javaMapData;

  if (!nativeDatabase->GetObjects(nativeObjectTypeSets->nodeTypes,
                             nativeObjectTypeSets->wayTypes,
                             nativeObjectTypeSets->areaTypes,
                             lonMin, latMin, lonMax, latMax,
                             magnification,
                             searchParameter,
                             nativeMapData->nodes,
                             nativeMapData->ways,
                             nativeMapData->areas,
                             nativeMapData->relationWays,
                             nativeMapData->relationAreas))
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetObjects(): GetObjects() Failed!");

    // GetObjects failed. Return null MapData object
    javaMapData=NULL;

    delete nativeMapData;
  }
  else
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetObjects(): GetObjects() Ok!");

    int mapDataIndex=gMapDataArray->Add(nativeMapData);

    jclass javaClass=env->FindClass("osm/scout/MapData");
  
    jmethodID methodId=env->GetMethodID(javaClass,"<init>","(I)V");
  
    javaMapData=env->NewObject(javaClass, methodId, mapDataIndex);
  }

  return javaMapData;
}

jboolean Java_osm_scout_Database_jniGetGroundTiles(JNIEnv *env, jobject object,
                                int databaseIndex, int mapDataIndex,
                                double lonMin, double latMin, double lonMax,
                                double latMax, double magnification)
{
  __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetGroundTiles(): GetGroundTiles() starts...");

  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetGroundTiles(): NULL Database object");
    return JNI_FALSE;
  }

  MapData *nativeMapData=gMapDataArray->Get(mapDataIndex);

  if (!nativeMapData)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetGroundTiles(): NULL MapData object");
    return JNI_FALSE;
  }

  return nativeDatabase->GetGroundTiles(
                             lonMin, latMin, lonMax, latMax,
                             magnification, nativeMapData->groundTiles);
}

jobject Java_osm_scout_Database_jniGetTypeConfig(JNIEnv *env, jobject object,
                                int databaseIndex)
{
  Database *nativeDatabase=gDatabaseArray->Get(databaseIndex);

  if (!nativeDatabase)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetTypeConfig(): NULL Database object");
    return NULL;
  }

  TypeConfig *nativeTypeConfig=nativeDatabase->GetTypeConfig();

  int typeConfigIndex=gTypeConfigArray->Add(nativeTypeConfig);

  jclass javaClass=env->FindClass("osm/scout/TypeConfig");
  
  jmethodID methodId=env->GetMethodID(javaClass,"<init>","(I)V");
  
  jobject javaTypeConfig=env->NewObject(javaClass, methodId, typeConfigIndex);

  return javaTypeConfig;
}

#ifdef __cplusplus
}
#endif


