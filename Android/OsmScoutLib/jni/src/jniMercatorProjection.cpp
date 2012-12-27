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

#include <osmscout/util/Projection.h>

#include <jniObjectArray.h>

#define DEBUG_TAG "OsmScoutJni:MercatorProjection"

using namespace osmscout;

extern JniObjectArray<MercatorProjection>   *gProjectionArray;

#ifdef __cplusplus
extern "C" {
#endif

jint Java_osm_scout_MercatorProjection_jniConstructor(JNIEnv *env, jobject object)
{
  MercatorProjection *nativeProjection=new MercatorProjection();

  return gProjectionArray->Add(nativeProjection);
}

void Java_osm_scout_MercatorProjection_jniDestructor(JNIEnv *env, jobject object,
                               int projectionIndex)
{
  MercatorProjection *nativeProjection=
                        gProjectionArray->GetAndRemove(projectionIndex);

  if (!nativeProjection)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL Projection object");
  }
  else
    delete nativeProjection;
}

jboolean Java_osm_scout_MercatorProjection_jniSet(JNIEnv *env, jobject object,
		int projectionIndex, jdouble lon, jdouble lat,
                jdouble magnification, jint width, jint height)
{
  MercatorProjection *nativeProjection=gProjectionArray->Get(projectionIndex);

  if (!nativeProjection)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniSet(): NULL Projection object");
    return JNI_FALSE;
  }

  return nativeProjection->Set(lon, lat, magnification, width, height);
}

jobject Java_osm_scout_MercatorProjection_jniGetBoundaries(JNIEnv *env,
                      jobject object, int projectionIndex)
{
  MercatorProjection *nativeProjection=gProjectionArray->Get(projectionIndex);

  if (!nativeProjection)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGetBoundaries(): NULL Projection object");
    return NULL;
  }

  jclass javaClass = env->FindClass("osm/scout/GeoBox");
  jmethodID methodId = env->GetMethodID(javaClass,"<init>","(DDDD)V");
  jobject geoBox=env->NewObject(javaClass, methodId,
                                nativeProjection->GetLonMin(),
                                nativeProjection->GetLonMax(),
                                nativeProjection->GetLatMin(),
                                nativeProjection->GetLatMax());

  return geoBox;
}

jobject Java_osm_scout_MercatorProjection_jniPixelToGeo(JNIEnv *env,
                    jobject object, int projectionIndex, double x, double y)
{
  MercatorProjection *nativeProjection=gProjectionArray->Get(projectionIndex);

  if (!nativeProjection)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniPixelToGeo(): NULL Projection object");
    return NULL;
  }

  double lon, lat;
  
  if (!nativeProjection->PixelToGeo(x, y, lon, lat))
    return NULL;

  jclass javaClass = env->FindClass("osm/scout/GeoPos");
  jmethodID methodId = env->GetMethodID(javaClass,"<init>","(DD)V");
  jobject geoPos=env->NewObject(javaClass, methodId, lon, lat);
  
  return geoPos;	
}

jobject Java_osm_scout_MercatorProjection_jniGeoToPixel(JNIEnv *env,
                  jobject object, int projectionIndex, double lon, double lat)
{
  MercatorProjection *nativeProjection=gProjectionArray->Get(projectionIndex);

  if (!nativeProjection)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniGeoToPixel(): NULL Projection object");
    return NULL;
  }

  double x, y;
  
  if (!nativeProjection->GeoToPixel(lon, lat, x, y))
    return NULL;
    
  jclass javaClass = env->FindClass("android/graphics/PointF");
  jmethodID methodId = env->GetMethodID(javaClass,"<init>","(FF)V");
  jobject point=env->NewObject(javaClass, methodId, (float)x, (float)y);
  
  return point;	
}

#ifdef __cplusplus
}
#endif

