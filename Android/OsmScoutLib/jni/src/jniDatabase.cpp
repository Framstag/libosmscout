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

#include "../include/jniObjectTypeSets.h"

#define DEBUG_TAG "OsmScoutJni:Database"

#ifdef __cplusplus
extern "C" {
#endif

osmscout::Database                    *gDatabase;

extern osmscout::StyleConfig          *gStyleConfig;
extern osmscout::MapData              *gMapData;
extern osmscout::MercatorProjection   *gMercatorProjection;
extern osmscout::ObjectTypeSets       *gObjectTypeSets;

void Java_osm_scout_Database_jniConstructor(JNIEnv *env, jobject object)
{
  osmscout::DatabaseParameter databaseParameter;

  gDatabase=new osmscout::Database(databaseParameter);
}

void Java_osm_scout_Database_jniDestructor(JNIEnv *env, jobject object)
{
  delete gDatabase;
}

jboolean Java_osm_scout_Database_jniOpen(JNIEnv *env, jobject object, jstring path)
{
  jboolean isCopy;

  const char *szPath=env->GetStringUTFChars(path, &isCopy);

  __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "jniOpen (%s)", szPath);

  if (gDatabase==NULL)
    return JNI_FALSE;

  bool result;

  if (gDatabase->Open(szPath))
    result=JNI_TRUE;
  else
    result=JNI_FALSE;

  env->ReleaseStringUTFChars(path, szPath);

  return result;
}

jboolean Java_osm_scout_Database_jniIsOpen(JNIEnv *env, jobject object)
{
  if (gDatabase==NULL)
    return JNI_FALSE;

  return gDatabase->IsOpen();
}

jobject Java_osm_scout_Database_jniGetBoundingBox(JNIEnv *env, jobject object)
{
  if (gDatabase==NULL)
    return NULL;

  double minLat, maxLat;
  double minLon, maxLon;
    
  if (!gDatabase->GetBoundingBox(minLat, minLon, maxLat, maxLon))
    return NULL;

  jclass javaClass = env->FindClass("osm/scout/GeoBox");
  jmethodID methodId = env->GetMethodID(javaClass,"<init>","(DDDD)V");
  jobject geoBox=env->NewObject(javaClass, methodId, minLon, maxLon, minLat, maxLat);

  return geoBox;
}

jobjectArray Java_osm_scout_Database_jniGetMatchingAdminRegions(JNIEnv *env, jobject object,
                      jstring name, int limit, jobject limitReached, jboolean startWith)
{
  if (gDatabase==NULL)
    return NULL;

  jboolean isCopy;

  const char *szName=env->GetStringUTFChars(name, &isCopy);

  std::list<osmscout::AdminRegion> regionList;
  
  bool nativeLimitReached;

  jobjectArray regions;

  if (gDatabase->GetMatchingAdminRegions(szName,
                                         regionList,
                                         limit,
                                         nativeLimitReached,
                                         startWith))
  {
    jclass javaClass=env->FindClass("osm/scout/AdminRegion");

    regions=env->NewObjectArray(regionList.size(), javaClass, NULL);

    jmethodID methodId = env->GetMethodID(javaClass, "<init>", "(Ljava/lang/String;IJ)V");

    int pos=0;

    for(std::list<osmscout::AdminRegion>::iterator iter=regionList.begin(); 
                                            iter!=regionList.end(); iter++)
    {
      jint type=iter->reference.type;
      jlong id=iter->reference.id;

      jobject region=env->NewObject(javaClass, methodId,
                                    env->NewStringUTF(iter->name.c_str()), type, id);

      env->SetObjectArrayElement(regions, pos, region);

      pos++;
    }

    // Set the <limitReached> value
    javaClass=env->FindClass("osm/scout/Bool");
    methodId=env->GetMethodID(javaClass, "set", "(Z)V");
    env->CallVoidMethod(limitReached, methodId, nativeLimitReached);
  }
  else
  {
    regions=NULL;
  }

  env->ReleaseStringUTFChars(name, szName);

  return regions;
}

jobject Java_osm_scout_Database_jniGetNode(JNIEnv *env, jobject object, long id)
{
  if (gDatabase==NULL)
    return NULL;

  osmscout::NodeRef nodeRef;

  gDatabase->GetNode(id, nodeRef);

  osmscout::Node* nativeNode=nodeRef.Get();

  jclass javaClass=env->FindClass("osm/scout/Node");
  
  jmethodID methodId=env->GetMethodID(javaClass,"<init>","(DD)V");
  
  jobject node=env->NewObject(javaClass, methodId, nativeNode->GetLon(), nativeNode->GetLat());

  return node;			
}

jobject Java_osm_scout_Database_jniGetObjects(JNIEnv *env, jobject object)
{
  if (gStyleConfig==NULL)
    return NULL;

  osmscout::AreaSearchParameter searchParameter;

  gDatabase->GetObjects(gObjectTypeSets->nodeTypes,
                        gObjectTypeSets->wayTypes,
                        gObjectTypeSets->areaTypes,
                        gMercatorProjection->GetLonMin(),
                        gMercatorProjection->GetLatMin(),
                        gMercatorProjection->GetLonMax(),
                        gMercatorProjection->GetLatMax(),
                        gMercatorProjection->GetMagnification(),
                        searchParameter,
                        gMapData->nodes,
                        gMapData->ways,
                        gMapData->areas,
                        gMapData->relationWays,
                        gMapData->relationAreas);

  return NULL;
}

#ifdef __cplusplus
}
#endif

