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
#include <osmscout/projection/Projection.h>
#include <osmscout/StyleConfig.h>

#include <jniMapPainterCanvas.h>
#include <jniObjectArray.h>
#include <jniObjectTypeSets.h>

using namespace osmscout;

// Global Object Arrays
JniObjectArray<Database>             *gDatabaseArray;
JniObjectArray<MapData>              *gMapDataArray;
JniObjectArray<MapPainterCanvas>     *gMapPainterArray;
JniObjectArray<MapParameter>         *gMapParameterArray;
JniObjectArray<MercatorProjection>   *gProjectionArray;
JniObjectArray<ObjectTypeSets>       *gObjectTypeSetsArray;
JniObjectArray<StyleConfig>          *gStyleConfigArray;
JniObjectArray<TypeConfig>           *gTypeConfigArray;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved)
{
  gDatabaseArray=new JniObjectArray<Database>();
  gMapDataArray=new JniObjectArray<MapData>();
  gMapPainterArray=new JniObjectArray<MapPainterCanvas>();
  gMapParameterArray=new JniObjectArray<MapParameter>();
  gProjectionArray=new JniObjectArray<MercatorProjection>();
  gObjectTypeSetsArray=new JniObjectArray<ObjectTypeSets>();
  gStyleConfigArray=new JniObjectArray<StyleConfig>();
  gTypeConfigArray=new JniObjectArray<TypeConfig>();

  return JNI_VERSION_1_6;
}

