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
    // Already loaded with error
    if (style.GetIconId()==0)
    {
      return false;
    }

    int iconIndex=style.GetIconId()-1;

    if (iconIndex<mIconLoaded.size() &&
        mIconLoaded[iconIndex])
    {
      return true;
    }

    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "loadIconPNG",
                                            "(Ljava/lang/String;I)Z");

    if (!methodId)
      return false;

    for (std::list<std::string>::const_iterator path=parameter.GetIconPaths().begin();
         path!=parameter.GetIconPaths().end(); ++path)
    {
      std::string filename=*path+"/"+style.GetIconName()+".png";

      jstring iconName=mJniEnv->NewStringUTF(filename.c_str());

      bool loaded=mJniEnv->CallIntMethod(mPainterObject, methodId,
                                            iconName, iconIndex);

      mJniEnv->DeleteLocalRef(iconName);

      if (loaded)
      {
        if (iconIndex>=mIconLoaded.size())
        {
          mIconLoaded.resize(iconIndex+1, false);
        }

        mIconLoaded[iconIndex]=true;

        return true;
      }
    }

    // Error loading icon file
    style.SetIconId(0);
    return false;
  }
  
  bool MapPainterCanvas::HasPattern(const MapParameter& parameter,
                    const FillStyle& style)
  {
    // Pattern already loaded with error
    if (style.GetPatternId()==0) {
      return false;
    }

    int patternIndex=style.GetPatternId()-1;

    if (patternIndex<mPatternLoaded.size() &&
        mPatternLoaded[patternIndex])
    {
      return true;
    }

    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "loadPatternPNG",
                                            "(Ljava/lang/String;I)Z");

    if (!methodId)
      return false;

    for (std::list<std::string>::const_iterator path=parameter.GetPatternPaths().begin();
         path!=parameter.GetPatternPaths().end();
         ++path) {
      std::string filename=*path+"/"+style.GetPatternName()+".png";

      jstring patternName=mJniEnv->NewStringUTF(filename.c_str());

      bool loaded=mJniEnv->CallIntMethod(mPainterObject, methodId,
                                         patternName, patternIndex);

      mJniEnv->DeleteLocalRef(patternName);

      if (loaded)
      {
        if (patternIndex>=mPatternLoaded.size())
        {
          mPatternLoaded.resize(patternIndex+1, false);
        }

        mPatternLoaded[patternIndex]=true;

        return true;
      }
    }

    // Error loading icon file
    style.SetPatternId(0);
    return false;
  }

  void MapPainterCanvas::GetTextDimension(const Projection& projection,
                                          const MapParameter& parameter,
                                          double objectWidth,
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
    jstring javaText;
    jint    javaTextColor;
    jint    javaTextStyle;
    jfloat  javaFontSize;
    jfloat  javaX;
    jfloat  javaY;
    jobject javaBox;
    jint    javaBgColor;
    jint    javaBorderColor;

    javaText=mJniEnv->NewStringUTF(label.text.c_str());
    javaFontSize=label.fontSize;
    javaX=(jfloat)label.x;
    javaY=(jfloat)label.y;

    if (dynamic_cast<const TextStyle*>(label.style.Get())!=NULL) {

      const TextStyle* style=dynamic_cast<const TextStyle*>(label.style.Get());

      javaTextColor=GetColorInt(style->GetTextColor().GetR(),
                                style->GetTextColor().GetG(),
                                style->GetTextColor().GetB(),
                                style->GetTextColor().GetA());

      javaTextStyle=style->GetStyle();

      javaBox=NULL;
    }
    else if (dynamic_cast<const ShieldStyle*>(label.style.Get())!=NULL) {

      const ShieldStyle* style=dynamic_cast<const ShieldStyle*>(label.style.Get());

      javaTextColor=GetColorInt(style->GetTextColor().GetR(),
                                style->GetTextColor().GetG(),
                                style->GetTextColor().GetB(),
                                style->GetTextColor().GetA());

      javaTextStyle=TextStyle::normal;

      jclass rectClass=mJniEnv->FindClass("android/graphics/RectF");
      jmethodID rectMethodId=mJniEnv->GetMethodID(rectClass,"<init>","(FFFF)V");
      javaBox=mJniEnv->NewObject(rectClass, rectMethodId,
                                 (jfloat)label.bx1, (jfloat)label.by1+1,
                                 (jfloat)label.bx2+1, (jfloat)label.by2+2);

      javaBgColor=GetColorInt(style->GetBgColor());
      javaBorderColor=GetColorInt(style->GetBorderColor());
    }
    else
      return;

    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawLabel",
                       "(Ljava/lang/String;FFFIILandroid/graphics/RectF;II)V");

    if (!methodId)
      return;

    mJniEnv->CallVoidMethod(mPainterObject, methodId, javaText, javaFontSize,
                            javaX, javaY, javaTextColor, javaTextStyle,
                            javaBox, javaBgColor, javaBorderColor);

    mJniEnv->DeleteLocalRef(javaText);
  }

  void MapPainterCanvas::DrawContourLabel(const Projection& projection,
                                          const MapParameter& parameter,
                                          const PathTextStyle& style,
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

  void MapPainterCanvas::DrawPrimitivePath(const Projection& projection,
                                           const MapParameter& parameter,
                                           const DrawPrimitiveRef& p,
                                           double x, double y,
                                           double minX,
                                           double minY,
                                           double maxX,
                                           double maxY)
  {
    DrawPrimitivePath(projection,
                      parameter,
                      p,
                      x, y,
                      minX,
                      minY,
                      maxX,
                      maxY,
                      NULL, NULL, NULL);
  }

  void MapPainterCanvas::DrawPrimitivePath(const Projection& projection,
                                           const MapParameter& parameter,
                                           const DrawPrimitiveRef& p,
                                           double x, double y,
                                           double minX,
                                           double minY,
                                           double maxX,
                                           double maxY,
                                           double* onPathX, double* onPathY,
                                           double* segmentLengths)
  {
    DrawPrimitive* primitive=p.Get();
    double         width=maxX-minX;
    double         height=maxY-minY;

    if (dynamic_cast<PolygonPrimitive*>(primitive)!=NULL)
    {
      PolygonPrimitive* polygon=dynamic_cast<PolygonPrimitive*>(primitive);

      int numPoints=polygon->GetCoords().size();

      float *arrayX=new float[numPoints];
      float *arrayY=new float[numPoints];

      int i=0;

      for (std::list<Coord>::const_iterator pixel=polygon->GetCoords().begin();
           pixel!=polygon->GetCoords().end(); ++pixel)
      {
        arrayX[i]=x+ConvertWidthToPixel(parameter, pixel->x-width/2);
        arrayY[i]=y+ConvertWidthToPixel(parameter, maxY-pixel->y-height/2);

        i++;
      }

      MapPathOnPath(arrayX, arrayY, numPoints,
                    onPathX, onPathY, segmentLengths);

      SetPolygonFillPath(arrayX, arrayY, numPoints);

      delete arrayX;
      delete arrayY;
    }
    else if (dynamic_cast<RectanglePrimitive*>(primitive)!=NULL)
    {
      RectanglePrimitive* rectangle=dynamic_cast<RectanglePrimitive*>(primitive);

      float *arrayX=new float[4];
      float *arrayY=new float[4];

      // Top-left corner
      arrayX[0]=x+ConvertWidthToPixel(parameter, rectangle->GetTopLeft().x-width/2);
      arrayY[0]=y+ConvertWidthToPixel(parameter, maxY-rectangle->GetTopLeft().y-height/2);

      // Top-right corner
      arrayX[1]=arrayX[0]+ConvertWidthToPixel(parameter,rectangle->GetWidth());
      arrayY[1]=arrayY[0];

      // Bottom-right corner
      arrayX[2]=arrayX[1];
      arrayY[2]=arrayY[0]+ConvertWidthToPixel(parameter,rectangle->GetHeight());

      // Bottom-left corner
      arrayX[3]=arrayX[0];
      arrayY[3]=arrayY[2];

      MapPathOnPath(arrayX, arrayY, 4,
                    onPathX, onPathY, segmentLengths);

      SetPolygonFillPath(arrayX, arrayY, 4);

      delete arrayX;
      delete arrayY;
    }
    else if (dynamic_cast<CirclePrimitive*>(primitive)!=NULL)
    {
      CirclePrimitive* circle=dynamic_cast<CirclePrimitive*>(primitive);

      jmethodID methodId=mJniEnv->GetMethodID(mPainterClass,
                                  "setCircleFillPath", "(FFF)V");

      jfloat posX=x+ConvertWidthToPixel(parameter, circle->GetCenter().x-width/2);
      jfloat posY=y+ConvertWidthToPixel(parameter, maxY-circle->GetCenter().y-height/2);
      jfloat radius=ConvertWidthToPixel(parameter, circle->GetRadius());

      mJniEnv->CallVoidMethod(mPainterObject, methodId, posX, posY, radius);
    }
  }

  void MapPainterCanvas::DrawSymbol(const Projection& projection,
                     const MapParameter& parameter, const Symbol& symbol,
                     double x, double y)
  {
    double minX;
    double minY;
    double maxX;
    double maxY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    for (std::list<DrawPrimitiveRef>::const_iterator p=symbol.GetPrimitives().begin();
         p!=symbol.GetPrimitives().end();
         ++p) {
      FillStyleRef fillStyle=(*p)->GetFillStyle();

      DrawPrimitivePath(projection,
                        parameter,
                        *p,
                        x, y,
                        minX,
                        minY,
                        maxX,
                        maxY);

      DrawFillStyle(projection,
                    parameter,
                    *fillStyle);
    }
  }

  void MapPainterCanvas::DrawContourSymbol(const Projection& projection,
                                           const MapParameter& parameter,
                                           const Symbol& symbol,
                                           double space,
                                           size_t transStart, size_t transEnd)
  {
    double lineLength=0;

    int numPoints=transEnd-transStart+1;

    double *onPathX=new double[numPoints];
    double *onPathY=new double[numPoints];

    double *segmentLengths=new double[numPoints-1];

    for(int i=0; i<numPoints; i++)
    {
      onPathX[i]=(double)transBuffer.buffer[transStart+i].x;
      onPathY[i]=(double)transBuffer.buffer[transStart+i].y;

      if (i!=0)
        segmentLengths[i-1]=sqrt(pow(onPathX[i]-onPathX[i-1], 2.0)+
                                 pow(onPathY[i]-onPathY[i-1], 2.0));

      lineLength+=segmentLengths[i-1];
    }

    double minX;
    double minY;
    double maxX;
    double maxY;

    symbol.GetBoundingBox(minX,minY,maxX,maxY);

    double width=ConvertWidthToPixel(parameter,maxX-minX);
    double height=ConvertWidthToPixel(parameter,maxY-minY);

    for (std::list<DrawPrimitiveRef>::const_iterator p=symbol.GetPrimitives().begin();
         p!=symbol.GetPrimitives().end();
         ++p)
    {
      FillStyleRef fillStyle=(*p)->GetFillStyle();

      double offset=space/2;

      while (offset+width<lineLength)
      {
        DrawPrimitivePath(projection,
                          parameter,
                          *p,
                          offset+width/2,
                          0,
                          minX,
                          minY,
                          maxX,
                          maxY,
                          onPathX, onPathY,
                          segmentLengths);

        DrawFillStyle(projection,
                      parameter,
                      *fillStyle);

        offset+=width+space;
      }
    }
  }

  void MapPainterCanvas::SetPolygonFillPath(float* x, float* y, int numPoints)
  {
    jfloatArray jArrayX=mJniEnv->NewFloatArray(numPoints);
    jfloatArray jArrayY=mJniEnv->NewFloatArray(numPoints);
    
    mJniEnv->SetFloatArrayRegion(jArrayX, 0, numPoints, x);
    mJniEnv->SetFloatArrayRegion(jArrayY, 0, numPoints, y);

    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass,
                                  "setPolygonFillPath", "([F[F)V");

    mJniEnv->CallVoidMethod(mPainterObject, methodId, jArrayX, jArrayY);

    mJniEnv->DeleteLocalRef(jArrayX);
    mJniEnv->DeleteLocalRef(jArrayY);
  }

  void MapPainterCanvas::MapPathOnPath(float* arrayX, float* arrayY,
                                       int numPoints,
                                       double* onPathX, double* onPathY,
                                       double* segmentLengths)
  {
    if ((onPathX==NULL) || (onPathY==NULL) || (segmentLengths==NULL))
      return;
  
    for(int i=0; i<numPoints; i++)
    {
      // First, find the segment for the given point
      int s=0;

      while(arrayX[i]>segmentLengths[s])
      {
        arrayX[i]-=segmentLengths[s];
        s++;
      }

      // Relative offset in the current segment ([0..1])
      double ratio = arrayX[i] / segmentLengths[s];

      // Line polynomial
      double x = onPathX[s] * (1 - ratio) + onPathX[s+1] * ratio;
      double y = onPathY[s] * (1 - ratio) + onPathY[s+1] * ratio;

      // Line gradient
      double dx = -(onPathX[s] - onPathX[s+1]);
      double dy = -(onPathY[s] - onPathY[s+1]);

      // optimization for: ratio = the_y / sqrt (dx * dx + dy * dy)
      ratio = arrayY[i] / segmentLengths[s];
      x += -dy * ratio;
      y += dx * ratio;

      arrayX[i]=(float)x;
      arrayY[i]=(float)y;
    }
  }

  void MapPainterCanvas::DrawIcon(const IconStyle* style, double x, double y)
  {
    jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawIcon",
                                            "(IFF)V");

    if (!methodId)
      return;

    jint iconIndex=style->GetIconId()-1;

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

    // Second step, set fill path
    int numPoints=area.transEnd-area.transStart+1;
    
    float *x=new float[numPoints];
    float *y=new float[numPoints];
    
    for(int i=0; i<numPoints; i++)
    {
    	x[i]=(float)transBuffer.buffer[area.transStart+i].x;
    	y[i]=(float)transBuffer.buffer[area.transStart+i].y;
    }

    SetPolygonFillPath(x, y, numPoints);

    delete x;
    delete y;
    
    // Finally, draw fill style
    DrawFillStyle(projection, parameter, *area.fillStyle);
  }

  void MapPainterCanvas::DrawFillStyle(const Projection& projection,
                                      const MapParameter& parameter,
                                      const FillStyle& fill)
  {
    if (fill.HasPattern() &&
        projection.GetMagnification()>=fill.GetPatternMinMag() &&
        HasPattern(parameter,fill)) {

      jint patternId=fill.GetPatternId()-1;

      jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawPatternArea",
                                            "(I)V");
    
      if (!methodId)
        return;
    
      mJniEnv->CallVoidMethod(mPainterObject, methodId, patternId);
    }
    else if (fill.GetFillColor().IsVisible()) {

      jint color=GetColorInt(fill.GetFillColor());

      jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawFilledArea",
                                            "(I)V");
    
      if (!methodId)
        return;
    
      mJniEnv->CallVoidMethod(mPainterObject, methodId, color);
    }

    // Draw  border
    if (fill.GetBorderWidth()>0 &&
        fill.GetBorderColor().IsVisible() &&
        fill.GetBorderWidth()>=mMinimumLineWidth)
    {
      double borderWidth=ConvertWidthToPixel(parameter,
                                             fill.GetBorderWidth());

      if (borderWidth>=parameter.GetLineMinWidthPixel()) {

        jfloatArray javaDash=NULL;
        float *dashArray=NULL;

        std::vector<double> dash=fill.GetBorderDash();

        if (!dash.empty())
        {
          javaDash=mJniEnv->NewFloatArray(dash.size());

          dashArray=new float[dash.size()];

          for(int i=0; i<dash.size(); i++)
          {
            dashArray[i]=dash[i]*borderWidth;
          }

          mJniEnv->SetFloatArrayRegion(javaDash, 0, dash.size(), dashArray);
        }

        jmethodID methodId=mJniEnv->GetMethodID(mPainterClass, "drawAreaBorder",
                                            "(IF[F)V");

        jint javaColor=GetColorInt(fill.GetBorderColor());
    
        mJniEnv->CallVoidMethod(mPainterObject, methodId, javaColor,
                               (jfloat)borderWidth, javaDash);

      }
    }
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

    mMinimumLineWidth=parameter.GetLineMinWidthPixel()*25.4/parameter.GetDPI();

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

