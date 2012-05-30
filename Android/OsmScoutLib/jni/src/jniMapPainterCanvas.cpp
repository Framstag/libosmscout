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

#include <jniMapPainterCanvas.h>
#include <jniObjectArray.h>

#define DEBUG_TAG "OsmScoutJni:MapPainterCanvas"

using namespace osmscout;

extern JniObjectArray<MapData>              *gMapDataArray;
extern JniObjectArray<MapPainterCanvas>     *gMapPainterArray;
extern JniObjectArray<MercatorProjection>   *gProjectionArray;
extern JniObjectArray<StyleConfig>          *gStyleConfigArray;

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
                   const LabelData& label)
  {
    // TODO
  }

  void MapPainterCanvas::DrawPlateLabel(const Projection& projection,
                        const MapParameter& parameter,
                        const LabelData& label)
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
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  CapStyle startCap,
                  CapStyle endCap,
                  size_t transStart, size_t transEnd)
  {
    jint javaColor=GetColorInt(color);
    
    jmethodID methodId=mJniEnv->GetMethodID(mJavaClass, "drawPath",
                                            "(IF[FZZ[F[F)V");
    
    if (!methodId)
      return;

    jfloatArray javaDash=NULL;
    float *dashArray=NULL;

    if (!dash.empty()) {

      javaDash=mJniEnv->NewFloatArray(dash.size());

      dashArray=new float[dash.size()];

      for(int i=0; i<dash.size(); i++)
      {
        dashArray[i]=dash[i]*width;
      }

      mJniEnv->SetFloatArrayRegion(javaDash, 0, dash.size(), dashArray);
    }

    jboolean roundedStartCap=JNI_FALSE;
    jboolean roundedEndCap=JNI_FALSE;

    if (dash.empty()) {

      if (startCap==capRound)
        roundedStartCap=JNI_TRUE;

      if (endCap==capRound)
        roundedEndCap=JNI_TRUE;
    }
    
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
        
    mJniEnv->CallVoidMethod(mObject, methodId, javaColor, (jfloat)width,
                            javaDash, roundedStartCap, roundedEndCap,
                            jArrayX, jArrayY);

    delete x;
    delete y;

    mJniEnv->DeleteLocalRef(jArrayX);
    mJniEnv->DeleteLocalRef(jArrayY);

    if (javaDash) {

      delete dashArray;
      mJniEnv->DeleteLocalRef(javaDash);
    }
  }

  void MapPainterCanvas::DrawArea(const Projection& projection,
                  const MapParameter& parameter,
                  const AreaData& area)
  {
    /*
     * TODO: Handle patterns
     * TODO: Handle clipping regions
     */
     
    jint fillColor=GetColorInt(area.fillStyle->GetFillColor());

    jint borderColor=GetColorInt(area.fillStyle->GetBorderColor());

    jfloat borderWidth=ConvertWidthToPixel(parameter,
                                           area.fillStyle->GetBorderWidth());

    if (borderWidth<parameter.GetLineMinWidthPixel())
    {
      // Set an invalid border width so no border will be drawn
      borderWidth=-1.0;
    }

    jmethodID methodId=mJniEnv->GetMethodID(mJavaClass,"drawArea","(IIF[F[F)V");
    
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
    
    mJniEnv->CallVoidMethod(mObject, methodId, fillColor, borderColor,
                            borderWidth, jArrayX, jArrayY);

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
    jint color=GetColorInt(style.GetFillColor());
    
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
  
  int MapPainterCanvas::GetColorInt(double r, double g, double  b, double a)
  {
    int colorA=(int)floor(255*a+0.5);
    int colorR=(int)floor(255*r+0.5);
    int colorG=(int)floor(255*g+0.5);
    int colorB=(int)floor(255*b+0.5);

    int color=((colorA<<24) | (colorR<<16) | (colorG<<8) | (colorB));

    return color;
  }

  int MapPainterCanvas::GetColorInt(Color color)
  {
    return GetColorInt(color.GetR(),
                       color.GetG(),
                       color.GetB(),
                       color.GetA());
  }

}

#ifdef __cplusplus
extern "C" {
#endif

jint Java_osm_scout_MapPainterCanvas_jniConstructor(JNIEnv *env, jobject object)
{
  MapPainterCanvas *nativeMapPainter=new MapPainterCanvas();

  return gMapPainterArray->Add(nativeMapPainter);
}

void Java_osm_scout_MapPainterCanvas_jniDestructor(JNIEnv *env, jobject object,
                                                   int mapPainterIndex)
{
  MapPainterCanvas *nativeMapPainter=
                    gMapPainterArray->GetAndRemove(mapPainterIndex);

  if (!nativeMapPainter)
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDestructor(): NULL object");
  else
    delete nativeMapPainter;
}

jboolean Java_osm_scout_MapPainterCanvas_jniDrawMap(JNIEnv *env, jobject object,
                    int mapPainterIndex, int styleConfigIndex,
                    int projectionIndex, int mapDataIndex)
{
  MapPainterCanvas *nativeMapPainter=gMapPainterArray->Get(mapPainterIndex);

  if (!nativeMapPainter)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): NULL MapPainter object");
    return JNI_FALSE;
  }

  StyleConfig *nativeStyleConfig=gStyleConfigArray->Get(styleConfigIndex);

  if (!nativeStyleConfig)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): NULL StyleConfig pointer");
    return JNI_FALSE;
  }

  MercatorProjection *nativeProjection=gProjectionArray->Get(projectionIndex);

  if (!nativeProjection)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): NULL Projection pointer");
    return JNI_FALSE;
  }

  MapData *nativeMapData=gMapDataArray->Get(mapDataIndex);

  if (!nativeMapData)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): NULL MapData pointer");
    return JNI_FALSE;
  }
	
  osmscout::MapParameter mapParameter;
  
  return nativeMapPainter->DrawMap(*nativeStyleConfig, *nativeProjection,
                              mapParameter, *nativeMapData, env, object);
}

#ifdef __cplusplus
}
#endif

