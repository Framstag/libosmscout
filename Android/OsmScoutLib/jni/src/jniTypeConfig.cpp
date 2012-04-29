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

#include <osmscout/Database.h>
#include <osmscout/StyleConfig.h>
#include <osmscout/StyleConfigLoader.h>

#include <jniObjectArray.h>

#define DEBUG_TAG "OsmScoutJni:TypeConfig"

using namespace osmscout;

extern JniObjectArray<TypeConfig>           *gTypeConfigArray;

#ifdef __cplusplus
extern "C" {
#endif

void Java_osm_scout_TypeConfig_jniDestructor(JNIEnv *env, jobject object,
                                              int typeConfigIndex)
{
  TypeConfig *nativeTypeConfig=gTypeConfigArray->GetAndRemove(typeConfigIndex);

  if (!nativeTypeConfig)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL TypeConfig object");
  }

  // Do not delete native TypeConfig. Ii internally belongs to the matching
  // database
  //delete nativeTypeConfig;
}

#ifdef __cplusplus
}
#endif

