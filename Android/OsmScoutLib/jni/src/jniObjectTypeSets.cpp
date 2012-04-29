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

#include <jniObjectArray.h>
#include <jniObjectTypeSets.h>

#define DEBUG_TAG "OsmScoutJni:ObjectTypeSets"

using namespace osmscout;

extern JniObjectArray<ObjectTypeSets>       *gObjectTypeSetsArray;

#ifdef __cplusplus
extern "C" {
#endif

jint Java_osm_scout_ObjectTypeSets_jniConstructor(JNIEnv *env, jobject object)
{
  ObjectTypeSets *nativeObjectTypeSets=new ObjectTypeSets;

  return gObjectTypeSetsArray->Add(nativeObjectTypeSets);
}

void Java_osm_scout_ObjectTypeSets_jniDestructor(JNIEnv *env, jobject object,
                                              int objectTypeSetsIndex)
{
  ObjectTypeSets *nativeObjectTypeSets=
                     gObjectTypeSetsArray->GetAndRemove(objectTypeSetsIndex);

  if (!nativeObjectTypeSets)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL ObjectTypeSets object");
  }
  else
    delete nativeObjectTypeSets;
}

#ifdef __cplusplus
}
#endif

