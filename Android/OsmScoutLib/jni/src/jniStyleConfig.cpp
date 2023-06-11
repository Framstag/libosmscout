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

#include "osmscout/database/Database.h"
#include <osmscout/StyleConfig.h>
#include <osmscout/StyleConfigLoader.h>

#include <jniObjectTypeSets.h>
#include <jniObjectArray.h>

#define DEBUG_TAG "OsmScoutJni:StyleConfig"

using namespace osmscout;

extern JniObjectArray<ObjectTypeSets>       *gObjectTypeSetsArray;
extern JniObjectArray<StyleConfig>          *gStyleConfigArray;
extern JniObjectArray<TypeConfig>           *gTypeConfigArray;

#ifdef __cplusplus
extern "C" {
#endif

jint Java_osm_scout_StyleConfig_jniConstructor(JNIEnv *env, jobject object,
                                               int typeConfigIndex)
{
  TypeConfig *nativeTypeConfig=gTypeConfigArray->Get(typeConfigIndex);

  if (!nativeTypeConfig)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniConstructor(): NULL TypeConfig object");

    return -1;
  }

  StyleConfig *nativeStyleConfig=new StyleConfig(nativeTypeConfig);

  return gStyleConfigArray->Add(nativeStyleConfig);
}

void Java_osm_scout_StyleConfig_jniDestructor(JNIEnv *env, jobject object,
                                              int styleConfigIndex)
{
  StyleConfig *nativeStyleConfig=gStyleConfigArray->GetAndRemove(styleConfigIndex);

  if (!nativeStyleConfig)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL object");
  }
  else
    delete nativeStyleConfig;
}

jboolean Java_osm_scout_StyleConfig_jniLoadStyleConfig(JNIEnv *env,
                    jobject object, int styleConfigIndex, jstring javaFileName)
{
  StyleConfig *nativeStyleConfig=gStyleConfigArray->Get(styleConfigIndex);

  if (!nativeStyleConfig)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                                    "jniDestructor(): NULL object");
    return JNI_FALSE;
  }

  jboolean isCopy;

  const char *nativeFileName=env->GetStringUTFChars(javaFileName, &isCopy);

  jboolean result=osmscout::LoadStyleConfig(nativeFileName, *nativeStyleConfig);

  env->ReleaseStringUTFChars(javaFileName, nativeFileName);

  return result;
}

jobject Java_osm_scout_StyleConfig_jniGetObjectTypesWithMaxMag(JNIEnv *env,
               jobject object, int styleConfigIndex, jdouble magnification)
{
  StyleConfig *nativeStyleConfig=gStyleConfigArray->Get(styleConfigIndex);

  if (!nativeStyleConfig)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                     "jniGetObjectTypesWithMaxMag(): NULL StyleConfig object");
    return NULL;
  }

  ObjectTypeSets *nativeObjectTypeSets=new ObjectTypeSets;

  int objectTypeSetsIndex=gObjectTypeSetsArray->Add(nativeObjectTypeSets);

  nativeStyleConfig->GetNodeTypesWithMaxMag(magnification,
                                            nativeObjectTypeSets->nodeTypes);

  nativeStyleConfig->GetWayTypesByPrioWithMaxMag(magnification,
                                            nativeObjectTypeSets->wayTypes);

  nativeStyleConfig->GetAreaTypesWithMaxMag(magnification,
                                            nativeObjectTypeSets->areaTypes);

  jclass javaClass=env->FindClass("osm/scout/ObjectTypeSets");
  
  jmethodID methodId=env->GetMethodID(javaClass,"<init>","(I)V");
  
  jobject javaObjectTypeSets=env->NewObject(javaClass, methodId, objectTypeSetsIndex);

  return javaObjectTypeSets;
}

#ifdef __cplusplus
}
#endif

