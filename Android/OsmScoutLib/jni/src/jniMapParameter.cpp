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

#include <osmscout/MapPainter.h>

#include <jniObjectArray.h>

#define DEBUG_TAG "OsmScoutJni:MapParameter"

using namespace osmscout;

extern JniObjectArray<MapParameter>       *gMapParameterArray;

#ifdef __cplusplus
extern "C" {
#endif

jint Java_osm_scout_MapParameter_jniConstructor(JNIEnv *env, jobject object)
{
  MapParameter *nativeMapParameter=new MapParameter();

  return gMapParameterArray->Add(nativeMapParameter);
}

void Java_osm_scout_MapParameter_jniDestructor(JNIEnv *env, jobject object,
                                          int mapParameterIndex)
{
  MapParameter *nativeMapParameter=gMapParameterArray->GetAndRemove(mapParameterIndex);

  if (!nativeMapParameter)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL object");
  }
  else
    delete nativeMapParameter;
}

void Java_osm_scout_MapParameter_jniSetIconPaths(JNIEnv *env, jobject object,
                          int mapParameterIndex, jobjectArray pathArray)
{
  MapParameter *nativeMapParameter=gMapParameterArray->Get(mapParameterIndex);

  if (!nativeMapParameter)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniSetIconPaths(): NULL object");
    return;
  }

  int pathCount=env->GetArrayLength(pathArray);

  std::list<std::string> nativePaths;

  for (int i=0; i<pathCount; i++)
  {
    jstring path=(jstring)env->GetObjectArrayElement(pathArray, i);

    const char *c_string=env->GetStringUTFChars(path, 0);

    std::string std_string(c_string);

    nativePaths.push_back(std_string);

    env->ReleaseStringUTFChars(path, c_string);
  }

  nativeMapParameter->SetIconPaths(nativePaths);
}

void Java_osm_scout_MapParameter_jniSetPatternPaths(JNIEnv *env, jobject object,
                          int mapParameterIndex, jobjectArray pathArray)
{
  MapParameter *nativeMapParameter=gMapParameterArray->Get(mapParameterIndex);

  if (!nativeMapParameter)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniSetPatternPaths(): NULL object");
    return;
  }

  int pathCount=env->GetArrayLength(pathArray);

  std::list<std::string> nativePaths;

  for (int i=0; i<pathCount; i++)
  {
    jstring path=(jstring)env->GetObjectArrayElement(pathArray, i);

    const char *c_string=env->GetStringUTFChars(path, 0);

    std::string std_string(c_string);

    nativePaths.push_back(std_string);

    env->ReleaseStringUTFChars(path, c_string);
  }

  nativeMapParameter->SetPatternPaths(nativePaths);
}

#ifdef __cplusplus
}
#endif

