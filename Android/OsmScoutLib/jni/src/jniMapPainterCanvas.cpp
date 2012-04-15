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
#include <math.h>

#include <osmscout/AdminRegion.h>
#include <osmscout/Database.h>
#include <osmscout/Node.h>
#include <osmscout/MapPainter.h>

#include "../include/jniMapPainterCanvas.h"

#define DEBUG_TAG "OsmScoutJni:MapPainterCanvas"

#ifdef __cplusplus
extern "C" {
#endif

osmscout::MapPainterCanvas           *gMapPainter;

extern osmscout::StyleConfig         *gStyleConfig;
extern osmscout::MapData             *gMapData;
extern osmscout::MercatorProjection  *gMercatorProjection;

namespace osmscout {

  MapPainterCanvas::MapPainterCanvas()
  {
  }

  MapPainterCanvas::~MapPainterCanvas()
  {
  }
  
  bool MapPainterCanvas::HasIcon(const StyleConfig& styleConfig,
                 const MapParameter& parameter,
                 IconStyle& style)
  {
    // TODO
    return false;
  }
  
  bool MapPainterCanvas::HasPattern(const MapParameter& parameter,
                    const FillStyle& style)
  {
    // TODO
    return false;
  }

  void MapPainterCanvas::GetTextDimension(const MapParameter& parameter,
                          double fontSize,
                          const std::string& text,
                          double& xOff,
                          double& yOff,
                          double& width,
                          double& height)
  {
    // TODO
  }
                          
  void MapPainterCanvas::DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const Label& label)
  {
    // TODO
  }

  void MapPainterCanvas::DrawPlateLabel(const Projection& projection,
                        const MapParameter& parameter,
                        const Label& label)
  {
    // TODO
  }
                        
  void MapPainterCanvas::DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const LabelStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd)
  {
    // TODO
  }

  void MapPainterCanvas::DrawSymbol(const SymbolStyle* style,
                    double x, double y)
  {
    // TODO
  }

  void MapPainterCanvas::DrawIcon(const IconStyle* style,
                  double x, double y)
  {
    // TODO
  }

  void MapPainterCanvas::DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  double r, double g, double b, double a,
                  double width,
                  const std::vector<double>& dash,
                  CapStyle startCap,
                  CapStyle endCap,
                  size_t transStart, size_t transEnd)
  {
    jint color=GetColor(r, g, b, a);
    
    jmethodID methodId=mJniEnv->GetMethodID(mJavaClass, "drawPath",
                                            "(IF[F[F)V");
    
    if (!methodId)
      return;
    
    int numPoints=transEnd-transStart+1;
    
    float *x=new float[numPoints];
    float *y=new float[numPoints];
    
    for(int i=0; i<numPoints; i++)
    {
    	x[i]=(float)transBuffer.buffer[transStart+i].x;
    	y[i]=(float)transBuffer.buffer[transStart+i].y;
    }
    
    jfloatArray jArrayX=mJniEnv->NewFloatArray(numPoints);
    jfloatArray jArrayY=mJniEnv->NewFloatArray(numPoints);
    
    mJniEnv->SetFloatArrayRegion(jArrayX, 0, numPoints, x);
    mJniEnv->SetFloatArrayRegion(jArrayY, 0, numPoints, y);
        
    mJniEnv->CallVoidMethod(mObject, methodId, (jint)color, (jfloat)width,
                            jArrayX, jArrayY);

    delete x;
    delete y;

    mJniEnv->DeleteLocalRef(jArrayX);
    mJniEnv->DeleteLocalRef(jArrayY);
  }

  void MapPainterCanvas::DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area)
  {
    /*
     * TODO: Handle patterns
     * TODO: Handle clipping regions
     */
     
    jint color=GetColor(area.fillStyle->GetFillR(), area.fillStyle->GetFillG(),
                        area.fillStyle->GetFillB(), area.fillStyle->GetFillA());
    
    jmethodID methodId=mJniEnv->GetMethodID(mJavaClass,"drawArea","(I[F[F)V");
    
    if (!methodId)
      return;
    
    int numPoints=area.transEnd-area.transStart+1;
    
    float *x=new float[numPoints];
    float *y=new float[numPoints];
    
    for(int i=0; i<numPoints; i++)
    {
    	x[i]=(float)transBuffer.buffer[area.transStart+i].x;
    	y[i]=(float)transBuffer.buffer[area.transStart+i].y;
    }
    
    jfloatArray jArrayX=mJniEnv->NewFloatArray(numPoints);
    jfloatArray jArrayY=mJniEnv->NewFloatArray(numPoints);
    
    mJniEnv->SetFloatArrayRegion(jArrayX, 0, numPoints, x);
    mJniEnv->SetFloatArrayRegion(jArrayY, 0, numPoints, y);
    
    mJniEnv->CallVoidMethod(mObject, methodId, color, jArrayX, jArrayY);
    
    delete x;
    delete y;
    
    mJniEnv->DeleteLocalRef(jArrayX);
    mJniEnv->DeleteLocalRef(jArrayY);
  }

  void MapPainterCanvas::DrawArea(const FillStyle& style,
                  const MapParameter& parameter,
                  double x,
                  double y,
                  double width,
                  double height)
  {
    jint color=GetColor(style.GetFillR(), style.GetFillG(),
                        style.GetFillB(), style.GetFillA());
    
    jmethodID methodId=mJniEnv->GetMethodID(mJavaClass,"drawArea","(IFFFF)V");
    
    if (!methodId)
      return;
    
    mJniEnv->CallVoidMethod(mObject, methodId, color, (jfloat)x, (jfloat)y,
    						(jfloat)width, (jfloat)height);	
  }
  
  bool MapPainterCanvas::DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 JNIEnv *env,
                 jobject object)
  {
    mJniEnv=env;
    mJavaClass=env->FindClass("osm/scout/MapPainterCanvas");
    mObject=object;

    return Draw(styleConfig, projection, parameter, data);
  }
  
  int MapPainterCanvas::GetColor(double r, double g, double  b, double a)
  {
    int colorA=(int)floor(255*a+0.5);
    int colorR=(int)floor(255*r+0.5);
    int colorG=(int)floor(255*g+0.5);
    int colorB=(int)floor(255*b+0.5);

    int color=((colorA<<24) | (colorR<<16) | (colorG<<8) | (colorB));

    return color;
  }
}

void Java_osm_scout_MapPainterCanvas_jniConstructor(JNIEnv *env, jobject object)
{
  gMapPainter=new osmscout::MapPainterCanvas();
}

void Java_osm_scout_MapPainterCanvas_jniDestructor(JNIEnv *env, jobject object)
{
  delete gMapPainter;
}

jboolean Java_osm_scout_MapPainterCanvas_jniDrawMap(JNIEnv *env, jobject object)
{
  if (gMapPainter==NULL)
    return JNI_FALSE;
	
  osmscout::MapParameter mapParameter;
  
  __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG, "jniDrawMap");
  
  return gMapPainter->DrawMap(*gStyleConfig, *gMercatorProjection,
                              mapParameter, *gMapData, env, object);
}

#ifdef __cplusplus
}
#endif

