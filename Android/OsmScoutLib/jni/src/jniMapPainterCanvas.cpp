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
extern JniObjectArray<MapParameter>         *gMapParameterArray;
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
    if (style.GetId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    if (style.GetId()!=0) {
      return true;
    }

    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "loadIconPNG",
                                            "(Ljava/lang/String;)I");

    if (!methodId)
      return false;

    for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
         path!=parameter.GetIconPaths().end(); ++path)
    {
      std::string filename=*path+"/"+style.GetIconName()+".png";

      jstring iconName=mJniEnv->NewStringUTF(filename.c_str());

      int id=mJniEnv->CallIntMethod(mPainterObject, methodId, iconName);

      mJniEnv->DeleteLocalRef(iconName);

      if (id>0)
      {
        style.SetId(id);    
        return true;
      }
    }

    // Error loading icon file
    style.SetId(std::numeric_limits<size_t>::max());
    return false;
  }
  
  bool MapPainterCanvas::HasPattern(const MapParameter& parameter,
                    const FillStyle& style)
  {
    // Was not able to load pattern
    if (style.GetPatternId()==std::numeric_limits<size_t>::max()) {
      return false;
    }

    // Pattern already loaded
    if (style.GetPatternId()!=0) {
      return true;
    }

    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "loadPatternPNG",
                                            "(Ljava/lang/String;)I");

    if (!methodId)
      return false;

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+"/"+style.GetPatternName()+".png";

      jstring iconName=mJniEnv->NewStringUTF(filename.c_str());

      int id=mJniEnv->CallIntMethod(mPainterObject, methodId, iconName);

      mJniEnv->DeleteLocalRef(iconName);

      if (id>0)
      {
        style.SetPatternId(id);
        return true;
      }
    }

    // Error loading icon file
    style.SetPatternId(std::numeric_limits<size_t>::max());
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
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "getTextDimension",
                               "(Ljava/lang/String;F)Landroid/graphics/Rect;");

    if (!methodId)
      return;

    jfloat javaFontSize=fontSize;

    jstring javaText=mJniEnv->NewStringUTF(text.c_str());
    
    jobject javaRect=mJniEnv->CallObjectMethod(mPainterObject, methodId,
                                               javaText, javaFontSize);

    mJniEnv->DeleteLocalRef(javaText);

    xOff=yOff=0.0;

    jclass rectClass=mJniEnv->FindClass("android/graphics/Rect");

    methodId=mJniEnv->GetMethodID(rectClass, "width", "()I");
    width=(double)mJniEnv->CallIntMethod(javaRect, methodId);

    methodId=mJniEnv->GetMethodID(rectClass, "height", "()I");
    height=(double)mJniEnv->CallIntMethod(javaRect, methodId);

    mJniEnv->DeleteLocalRef(javaRect);
    mJniEnv->DeleteLocalRef(rectClass);
  }
                          
  void MapPainterCanvas::DrawLabel(const Projection& projection,
                   const MapParameter& parameter,
                   const LabelData& label)
  {
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawLabel",
                               "(Ljava/lang/String;FFFII)V");

    if (!methodId)
      return;

    jint textColor=GetColorInt(
                               label.style->GetTextColor().GetR(),
                               label.style->GetTextColor().GetG(),
                               label.style->GetTextColor().GetB(),
                               label.alpha);

    jstring javaText=mJniEnv->NewStringUTF(label.text.c_str());

    jint labelStyle=label.style->GetStyle();

    mJniEnv->CallVoidMethod(mPainterObject, methodId, javaText, label.fontSize,
                            (jfloat)label.x, (jfloat)label.y, textColor,
                            labelStyle);

    mJniEnv->DeleteLocalRef(javaText);
  }

  void MapPainterCanvas::DrawPlateLabel(const Projection& projection,
                        const MapParameter& parameter,
                        const LabelData& label)
  {
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawPlateLabel",
                        "(Ljava/lang/String;FLandroid/graphics/RectF;III)V");

    if (!methodId)
      return;

    jclass rectClass=mJniEnv->FindClass("android/graphics/RectF");
    jmethodID rectMethodId=mJniEnv->GetMethodID(rectClass,"<init>","(FFFF)V");
    jobject box=mJniEnv->NewObject(rectClass, rectMethodId, (jfloat)label.bx1,
                   (jfloat)label.by1, (jfloat)label.bx2+1, (jfloat)label.by2+1);

    jint textColor=GetColorInt(label.style->GetTextColor());
    jint bgColor=GetColorInt(label.style->GetBgColor());
    jint borderColor=GetColorInt(label.style->GetBorderColor());

    jstring javaText=mJniEnv->NewStringUTF(label.text.c_str());

    mJniEnv->CallVoidMethod(mPainterObject, methodId, javaText,
                           (jfloat)label.fontSize, box, textColor,
                           bgColor, borderColor);

    mJniEnv->DeleteLocalRef(javaText);
  }
                        
  void MapPainterCanvas::DrawContourLabel(const Projection& projection,
                          const MapParameter& parameter,
                          const LabelStyle& style,
                          const std::string& text,
                          size_t transStart, size_t transEnd)
  {
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawContourLabel",
                     "(Ljava/lang/String;IFF[F[F)V");
    
    if (!methodId)
      return;

    jstring javaText=mJniEnv->NewStringUTF(text.c_str());

    jint textColor=GetColorInt(style.GetTextColor());

    jfloat pathLenght=0.0;

    int numPoints=transEnd-transStart+1;
    
    float *x=new float[numPoints];
    float *y=new float[numPoints];

    if (transBuffer.buffer[transStart].x<=transBuffer.buffer[transEnd].x)
    {
      // Path orientation is from left to right
      // Direct copy of the data
    
      for(int i=0; i<numPoints; i++)
      {
        x[i]=(float)transBuffer.buffer[transStart+i].x;
        y[i]=(float)transBuffer.buffer[transStart+i].y;

        if (i!=0)
          pathLenght+=sqrt(pow(x[i]-x[i-1], 2.0)+pow(y[i]-y[i-1], 2.0));
      }
    }
    else
    {
      // Path orientation is from right to left
      // Inverse copy of the data

      for(int i=0; i<numPoints; i++)
      {
        x[i]=(float)transBuffer.buffer[transEnd-i].x;
        y[i]=(float)transBuffer.buffer[transEnd-i].y;

        if (i!=0)
          pathLenght+=sqrt(pow(x[i]-x[i-1], 2.0)+pow(y[i]-y[i-1], 2.0));
      }
    }
    
    jfloatArray jArrayX=mJniEnv->NewFloatArray(numPoints);
    jfloatArray jArrayY=mJniEnv->NewFloatArray(numPoints);
    
    mJniEnv->SetFloatArrayRegion(jArrayX, 0, numPoints, x);
    mJniEnv->SetFloatArrayRegion(jArrayY, 0, numPoints, y);
        
    mJniEnv->CallVoidMethod(mPainterObject, methodId, javaText,
                            textColor, (jfloat)style.GetSize(),
                            pathLenght, jArrayX, jArrayY);

    delete x;
    delete y;

    mJniEnv->DeleteLocalRef(jArrayX);
    mJniEnv->DeleteLocalRef(jArrayY);
  }

/*
  void MapPainterCanvas::DrawSymbol(const SymbolStyle* style,
                    double x, double y)
  {
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawSymbol",
                                            "(IIFFF)V");

    if (!methodId)
      return;

    jint javaStyle=style->GetStyle();

    jint javaColor=GetColorInt(style->GetFillColor());
    
    jfloat javaSize=style->GetSize();

    mJniEnv->CallVoidMethod(mPainterObject, methodId, javaStyle, javaColor,
                            (jfloat) javaSize, (jfloat) x, (jfloat) y);
  }
*/

  void MapPainterCanvas::DrawSymbol(const Projection& projection,
                     const MapParameter& parameter, const SymbolRef& symbol,
                     double x, double y)
  {
  }

  void MapPainterCanvas::DrawIcon(const IconStyle* style,
                  double x, double y)
  {
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawIcon",
                                            "(IFF)V");

    if (!methodId)
      return;

    int iconIndex=style->GetId()-1;

    mJniEnv->CallVoidMethod(mPainterObject, methodId, iconIndex,
                           (jfloat) x, (jfloat) y);
  }

  void MapPainterCanvas::DrawPath(const Projection& projection,
                  const MapParameter& parameter,
                  const Color& color,
                  double width,
                  const std::vector<double>& dash,
                  LineStyle::CapStyle startCap,
                  LineStyle::CapStyle endCap,
                  size_t transStart, size_t transEnd)
  {
    jint javaColor=GetColorInt(color);
    
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawPath",
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

      if (startCap==LineStyle::capRound)
        roundedStartCap=JNI_TRUE;

      if (endCap==LineStyle::capRound)
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
        
    mJniEnv->CallVoidMethod(mPainterObject, methodId, javaColor, (jfloat)width,
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
    // First, check if there is any clipping area
    if (!area.clippings.empty())
    {
      // Clip areas within the area
      for (std::list<PolyData>::const_iterator c=area.clippings.begin();
          c!=area.clippings.end(); c++)
      {
        const PolyData& clipData=*c;

        int numPoints=clipData.transEnd-clipData.transStart+1;
    
        float *x=new float[numPoints];
        float *y=new float[numPoints];
    
        for(int i=0; i<numPoints; i++)
        {
        	x[i]=(float)transBuffer.buffer[clipData.transStart+i].x;
        	y[i]=(float)transBuffer.buffer[clipData.transStart+i].y;
        }
    
        jfloatArray jClipArrayX=mJniEnv->NewFloatArray(numPoints);
        jfloatArray jClipArrayY=mJniEnv->NewFloatArray(numPoints);
    
        mJniEnv->SetFloatArrayRegion(jClipArrayX, 0, numPoints, x);
        mJniEnv->SetFloatArrayRegion(jClipArrayY, 0, numPoints, y);

        jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "addClipArea",
                                            "([F[F)V");
   
        if (!methodId)
          return;
    
        mJniEnv->CallVoidMethod(mPainterObject, methodId,
                                                jClipArrayX, jClipArrayY);

        delete x;
        delete y;
    
        mJniEnv->DeleteLocalRef(jClipArrayX);
        mJniEnv->DeleteLocalRef(jClipArrayY);
      }
    }

    jint patternId=-1;

    if (area.fillStyle->HasPattern() &&
        projection.GetMagnification()>=area.fillStyle->GetPatternMinMag() &&
        HasPattern(parameter, *area.fillStyle)) {

          patternId=area.fillStyle->GetPatternId();
    }

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

    if (patternId<0)
    {
      // Draw a filled area
     
      jint fillColor=GetColorInt(area.fillStyle->GetFillColor());

      jint borderColor=GetColorInt(area.fillStyle->GetBorderColor());

      jfloat borderWidth=ConvertWidthToPixel(parameter,
                                           area.fillStyle->GetBorderWidth());

      if (borderWidth<parameter.GetLineMinWidthPixel())
      {
        // Set an invalid border width so no border will be drawn
        borderWidth=-1.0;
      }

      jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawFilledArea",
                                            "(IIF[F[F)V");
    
      if (!methodId)
        return;
    
      mJniEnv->CallVoidMethod(mPainterObject, methodId, fillColor, borderColor,
                            borderWidth, jArrayX, jArrayY);
    }
    else
    {
      jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawPatternArea",
                                            "(I[F[F)V");
    
      if (!methodId)
        return;
    
      mJniEnv->CallVoidMethod(mPainterObject, methodId, patternId-1,
                                              jArrayX, jArrayY);
    }

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
    
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawArea",
                                            "(IFFFF)V");
    
    if (!methodId)
      return;
    
    mJniEnv->CallVoidMethod(mPainterObject, methodId, color, (jfloat)x,
                                    (jfloat)y, (jfloat)width, (jfloat)height);	
  }
  
  bool MapPainterCanvas::DrawMap(const StyleConfig& styleConfig,
                 const Projection& projection,
                 const MapParameter& parameter,
                 const MapData& data,
                 JNIEnv *env,
                 jobject object)
  {
    mJniEnv=env;
    mPainterClass=env->FindClass("osm/scout/MapPainterCanvas");
    mPainterObject=object;

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
                    int projectionIndex, int mapParameterIndex,
                    int mapDataIndex)
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

  MapParameter *nativeMapParameter=gMapParameterArray->Get(mapParameterIndex);

  if (!nativeMapParameter)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): NULL MapParameter pointer");
    return JNI_FALSE;
  }
	
  MapData *nativeMapData=gMapDataArray->Get(mapDataIndex);

  if (!nativeMapData)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): NULL MapData pointer");
    return JNI_FALSE;
  }
	
  bool result=nativeMapPainter->DrawMap(*nativeStyleConfig, *nativeProjection,
                              *nativeMapParameter, *nativeMapData, env, object);

  if (result)
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): DrawMap() Ok!");
  }
  else
  {
    __android_log_print(ANDROID_LOG_DEBUG, DEBUG_TAG,
                        "jniDrawMap(): DrawMap() failed!");
  }

  return result;
}

#ifdef __cplusplus
}
#endif

