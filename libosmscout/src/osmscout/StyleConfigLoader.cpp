/*
  This source is part of the libosmscout library
  Copyright (C) 2009  Tim Teulings

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

#include <osmscout/StyleConfigLoader.h>

#include <string.h>

#include <libxml/parser.h>

#include <cassert>
#include <iostream>
#include <sstream>

namespace osmscout {

  static bool StringToDouble(const char* string, double& value)
  {
    std::istringstream buffer(string);

    buffer.imbue(std::locale::classic());

    buffer >> value;

    return !buffer.fail() && !buffer.bad() && buffer.eof();
  }

  class StyleConfigParser
  {
    enum Context {
      contextUnknown,
      contextTop,
      contextNode,
      contextNodeSymbol,
      contextNodeRefLabel,
      contextNodeLabel,
      contextNodeIcon,
      contextWay,
      contextWayLine,
      contextWayRefLabel,
      contextWayLabel,
      contextArea,
      contextAreaFill,
      contextAreaSymbol,
      contextAreaLabel,
      contextAreaBorder,
      contextAreaIcon,
    };

  private:
    Context     context;
    StyleConfig &styleConfig;
    TypeId      type;

  public:
    StyleConfigParser(StyleConfig& styleConfig)
    : styleConfig(styleConfig)
    {
      context=contextUnknown;
    }

    bool GetFillStyle(const std::string& name, FillStyle::Style& style)
    {
      if (name=="plain") {
        style=FillStyle::plain;
      }
      else {
        return false;
      }

      return true;
    }

    bool GetLabelStyle(const std::string& name, LabelStyle::Style& style)
    {
      if (name=="normal") {
        style=LabelStyle::normal;
      }
      else if (name=="contour") {
        style=LabelStyle::contour;
      }
      else if (name=="plate") {
        style=LabelStyle::plate;
      }
      else if (name=="emphasize") {
        style=LabelStyle::emphasize;
      }
      else {
        return false;
      }

      return true;
    }

    bool GetLineStyle(const std::string& name, LineStyle::Style& style)
    {
      if (name=="normal") {
        style=LineStyle::normal;
      }
      else if (name=="longDash") {
        style=LineStyle::longDash;
      }
      else if (name=="dotted") {
        style=LineStyle::dotted;
      }
      else if (name=="lineDot") {
        style=LineStyle::lineDot;
      }
      else {
        return false;
      }

      return true;
    }

    bool GetSymbolStyle(const std::string& name, SymbolStyle::Style& style)
    {
      if (name=="none") {
        style=SymbolStyle::none;
      }
      else if (name=="box") {
        style=SymbolStyle::box;
      }
      else if (name=="circle") {
        style=SymbolStyle::circle;
      }
      else if (name=="triangle") {
        style=SymbolStyle::triangle;
      }
      else {
        return false;
      }

      return true;
    }

    bool GetMag(const std::string& name, Mag& mag)
    {
      if (name=="world") {
        mag=magWorld;
      }
      else if (name=="state") {
        mag=magState;
      }
      else if (name=="stateOver") {
        mag=magStateOver;
      }
      else if (name=="county") {
        mag=magCounty;
      }
      else if (name=="region") {
        mag=magRegion;
      }
      else if (name=="proximity") {
        mag=magProximity;
      }
      else if (name=="cityOver") {
        mag=magCityOver;
      }
      else if (name=="city") {
        mag=magCity;
      }
      else if (name=="suburb") {
        mag=magSuburb;
      }
      else if (name=="detail") {
        mag=magDetail;
      }
      else if (name=="close") {
        mag=magClose;
      }
      else if (name=="veryClose") {
        mag=magVeryClose;
      }
      else {
        return false;
      }

      return true;
    }

    size_t GetHexDigitValue(char c)
    {
      if (c>='0' && c<='9') {
        return c-'0';
      }
      else if (c>='a' && c<='f') {
        return 10+(c-'a');
      }

      assert(false);
    }

    bool GetColor(const std::string& color, double& r, double& g, double &b, double& a)
    {
      if (color.length()!=6 && color.length()!=8) {
        return false;
      }

      for (size_t i=0; i<color.length(); i++) {
        if (!(color[i]>='0' && color[i]<='9') && !(color[i]>='a' && color[i]<='f')) {
          return false;
        }
      }

      r=(16*GetHexDigitValue(color[0])+GetHexDigitValue(color[1]))/255.0;
      g=(16*GetHexDigitValue(color[2])+GetHexDigitValue(color[3]))/255.0;
      b=(16*GetHexDigitValue(color[4])+GetHexDigitValue(color[5]))/255.0;

      if (color.length()==8) {
        a=(16*GetHexDigitValue(color[6])+GetHexDigitValue(color[7]))/255.0;
      }
      else {
        a=1;
      }

      return true;
    }

    void StartElement(const xmlChar *name, const xmlChar **atts)
    {
      TypeConfig *typeConfig=styleConfig.GetTypeConfig();

      if (context==contextUnknown) {
        if (strcmp((const char*)name,"TravelJinni-Style")==0) {
          context=contextTop;
        }
        else {
          std::cerr << "Expected tag 'TravelJinni-Style'" << std::endl;
          return;
        }
      }
      else if (context==contextTop) {
        if (strcmp((const char*)name,"node")==0) {
          context=contextNode;
        }
        else if (strcmp((const char*)name,"way")==0) {
          context=contextWay;
        }
        else if (strcmp((const char*)name,"area")==0) {
          context=contextArea;
        }
        else {
          std::cerr << "Expected one of tags 'node', 'way' or 'area' not '" << (const char*)name << "'" << std::endl;
          return;
        }
      }
      else if (context==contextNode) {
        if (strcmp((const char*)name,"symbol")==0) {
          context=contextNodeSymbol;
        }
        else if (strcmp((const char*)name,"label")==0) {
          context=contextNodeLabel;
        }
        else if (strcmp((const char*)name,"ref")==0) {
          context=contextNodeRefLabel;
        }
        else if (strcmp((const char*)name,"icon")==0) {
          context=contextNodeIcon;
        }
        else {
          std::cerr << "Expected one of tags 'symbol' or 'label'" << std::endl;
          return;
        }
      }
      else if (context==contextWay) {
        if (strcmp((const char*)name,"line")==0) {
          context=contextWayLine;
        }
        else if (strcmp((const char*)name,"ref")==0) {
          context=contextWayRefLabel;
        }
        else if (strcmp((const char*)name,"label")==0) {
          context=contextWayLabel;
        }
        else {
          std::cerr << "Expected one of tags 'line' or 'label'" << std::endl;
          return;
        }
      }
      else if (context==contextArea) {
        if (strcmp((const char*)name,"fill")==0) {
          context=contextAreaFill;
        }
        else if (strcmp((const char*)name,"symbol")==0) {
          context=contextAreaSymbol;
        }
        else if (strcmp((const char*)name,"label")==0) {
          context=contextAreaLabel;
        }
        else if (strcmp((const char*)name,"border")==0) {
          context=contextAreaBorder;
        }
        else if (strcmp((const char*)name,"icon")==0) {
          context=contextAreaIcon;
        }
        else {
          std::cerr << "Expected one of tags 'fill', 'symbol' or 'label'" << std::endl;
          return;
        }
      }

      if (context==contextNode || context==contextWay || context==contextArea) {
        const xmlChar *keyValue=NULL;
        const xmlChar *valueValue=NULL;
        const xmlChar *prioValue=NULL;
        std::string   key;
        std::string   value;
        size_t        prio;

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"key")==0) {
              keyValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"value")==0) {
              valueValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"prio")==0) {
              prioValue=atts[i+1];
            }
          }
        }

        if (context==contextWay &&
            (keyValue==NULL || valueValue==NULL || prioValue==NULL)) {
          std::cerr << "Not all required attributes for way found" << std::endl;
          return;
        }
        else if (context==contextArea &&
            (keyValue==NULL || valueValue==NULL)) {
          std::cerr << "Not all required attributes for area found" << std::endl;
          return;
        }
        else if (keyValue==NULL || valueValue==NULL) {
          std::cerr << "Not all required attributes for node found" << std::endl;
          return;
        }

        key=(const char*)keyValue;
        value=(const char*)valueValue;


        if (context==contextWay) {
          if (sscanf((const char*)prioValue,"%u",&prio)!=1) {
            std::cerr << "Cannot parse prio: '" << prioValue << "'" << std::endl;
            return;
          }
        }

        if (context==contextNode) {
          type=typeConfig->GetNodeTypeId(typeConfig->GetTagId(key.c_str()),value.c_str());
        }
        else if (context==contextWay) {
          type=typeConfig->GetWayTypeId(typeConfig->GetTagId(key.c_str()),value.c_str());
        }
        else if (context==contextArea) {
          type=typeConfig->GetAreaTypeId(typeConfig->GetTagId(key.c_str()),value.c_str());
        }

        if (type==typeIgnore) {
          std::cerr << "Unknown type: " << key << "/" << value << std::endl;
          return;
        }

        // std::cout << "Parsed: " << key << "/" << value << " => " << type << std::endl;

        if (context==contextWay) {
          styleConfig.SetWayPrio(type,prio);
        }

        /*
        TagInfo tagInfo(value,id);

        config.AddTagInfo(tagInfo);*/
      }
      else if (context==contextWayLine ||
               context==contextAreaBorder) {
        const xmlChar *styleValue=NULL;
        const xmlChar *colorValue=NULL;
        const xmlChar *alternateColorValue=NULL;
        const xmlChar *outlineColorValue=NULL;
        const xmlChar *minPixelValue=NULL;
        const xmlChar *widthValue=NULL;
        const xmlChar *outlineValue=NULL;
        std::string   style="normal";
        std::string   color;
        double        minPixel;
        double        width;
        double        outline;

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"style")==0) {
              styleValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"color")==0) {
              colorValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"alternateColor")==0) {
              alternateColorValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"outlineColor")==0) {
              outlineColorValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"minPixel")==0) {
              minPixelValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"width")==0) {
              widthValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"outline")==0) {
              outlineValue=atts[i+1];
            }
          }

          if (styleValue!=NULL) {
            style=(const char*)styleValue;
          }

          if (style=="none") {
            std::cerr << "You cannot force style 'none'" << std::endl;
            return;
          }

          LineStyle        line;
          LineStyle::Style s;
          double           r,g,b,a;

          if (!GetLineStyle(style,s)) {
            std::cerr << "Unknown style '" << style << "' for style type 'line'" << std::endl;
            return;
          }

          line.SetStyle(s);

          if (colorValue!=NULL) {
            color=(const char*)colorValue;

            if (!GetColor(color,r,g,b,a)) {
              std::cerr << "Cannot parse color value '" << color << "'" << std::endl;
              return;
            }

            line.SetLineColor(r,g,b,a);
          }

          if (alternateColorValue!=NULL) {
            color=(const char*)alternateColorValue;

            if (!GetColor(color,r,g,b,a)) {
              std::cerr << "Cannot parse color value '" << color << "'" << std::endl;
              return;
            }

            line.SetAlternateColor(r,g,b,a);
          }

          if (outlineColorValue!=NULL) {
            color=(const char*)outlineColorValue;

            if (!GetColor(color,r,g,b,a)) {
              std::cerr << "Cannot parse color value '" << color << "'" << std::endl;
              return;
            }

            line.SetOutlineColor(r,g,b,a);
          }

          if (minPixelValue!=NULL) {
            if (!StringToDouble((const char*)minPixelValue,minPixel)) {
              std::cerr << "Numeric value '" << (const char*)minPixelValue << "' has wrong value" << std::endl;
              return;
            }

            line.SetMinPixel(minPixel);
          }

          if (widthValue!=NULL) {
            if (!StringToDouble((const char*)widthValue,width)) {
              std::cerr << "Numeric value '" << (const char*)widthValue << "' has wrong value" << std::endl;
              return;
            }

            line.SetWidth(width);
          }

          if (outlineValue!=NULL) {
            if (!StringToDouble((const char*)outlineValue,outline)) {
              std::cerr << "Numeric value '" << (const char*)outlineValue << "' has wrong value" << std::endl;
              return;
            }

            line.SetOutline(outline);
          }

          if (context==contextWayLine) {
            styleConfig.SetWayLineStyle(type,line);
          }
          else if (context==contextAreaBorder) {
            styleConfig.SetAreaBorderStyle(type,line);
          }
        }
      }
      else if (context==contextAreaFill) {
        const xmlChar *styleValue=NULL;
        const xmlChar *colorValue=NULL;
        const xmlChar *layerValue=NULL;
        const xmlChar *filterValue=NULL;
        std::string   style="plain";

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"style")==0) {
              styleValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"color")==0) {
              colorValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"layer")==0) {
              layerValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"filter")==0) {
              filterValue=atts[i+1];
            }
          }

          if (styleValue!=NULL) {
            style=(const char*)styleValue;
          }

          if (style=="none") {
            std::cerr << "You cannot force style 'none'" << std::endl;
            return;
          }

          FillStyle        fill;
          FillStyle::Style s;

          if (!GetFillStyle(style,s)) {
            std::cerr << "Unknown style '" << style << "' for style type 'fill'" << std::endl;
            return;
          }

          fill.SetStyle(s);

          if (colorValue!=NULL) {
            std::string color=(const char*)colorValue;
            double      r,g,b,a;

            if (!GetColor(color,r,g,b,a)) {
              std::cerr << "Color value '" << colorValue << "' for style value 'color' cannot be parsed" << std::endl;
              return;
            }

            fill.SetColor(r,g,b,a);
          }

          if (layerValue!=NULL) {
            int layer=0;

            if (sscanf((const char*)layerValue,"%d",&layer)!=1) {
              std::cerr << "layer value '" << colorValue << "' for style value layer cannot be parsed" << std::endl;
              return;
            }

            fill.SetLayer(layer);
          }

          if (context==contextAreaFill) {
            if (filterValue!=NULL) {
              if (strcmp((const char*)filterValue,"building")==0) {
                styleConfig.SetAreaBuildingFillStyle(type,fill);
              }
              else {
                std::cerr << "Unknown area filter '" << (const char*)filterValue << "', ignoring!" << std::endl;
              }
            }
            else {
              styleConfig.SetAreaFillStyle(type,fill);
            }
          }
        }
      }
      else if (context==contextNodeRefLabel ||
               context==contextNodeLabel ||
               context==contextWayRefLabel ||
               context==contextWayLabel ||
               context==contextAreaLabel) {
        const xmlChar *styleValue=NULL;
        const xmlChar *minMagValue=NULL;
        const xmlChar *scaleAndFadeMagValue=NULL;
        const xmlChar *maxMagValue=NULL;
        const xmlChar *sizeValue=NULL;
        const xmlChar *textColorValue=NULL;
        const xmlChar *bgColorValue=NULL;
        const xmlChar *borderColorValue=NULL;
        std::string   style="normal";

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"style")==0) {
              styleValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"minMag")==0) {
              minMagValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"scaleAndFadeMag")==0) {
              scaleAndFadeMagValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"maxMag")==0) {
              maxMagValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"size")==0) {
              sizeValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"textColor")==0) {
              textColorValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"bgColor")==0) {
              bgColorValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"borderColor")==0) {
              borderColorValue=atts[i+1];
            }
          }
        }

        if (styleValue!=NULL) {
          style=(const char*)styleValue;
        }

        if (style=="none") {
          std::cerr << "You cannot force style 'none'" << std::endl;
          return;
        }

        LabelStyle        label;
        LabelStyle::Style s;

        if (!GetLabelStyle(style,s)) {
          std::cerr << "Unknown style '" << style << "' for style type 'label'" << std::endl;
          return;
        }

        label.SetStyle(s);

        if (minMagValue!=NULL) {
          std::string minMag;
          Mag         m;

          minMag=(const char*)minMagValue;

          if (!GetMag(minMag,m)) {
            std::cerr << "Unknown minimum magnification '" << minMag << "' for style type 'label'" << std::endl;
            return;
          }

          label.SetMinMag(m);
        }

        if (scaleAndFadeMagValue!=NULL) {
          std::string scaleAndFadeMag;
          Mag         m;

          scaleAndFadeMag=(const char*)scaleAndFadeMagValue;

          if (!GetMag(scaleAndFadeMag,m)) {
            std::cerr << "Unknown scaleAndFade magnification '" << scaleAndFadeMag << "' for style type 'label'" << std::endl;
            return;
          }

          label.SetScaleAndFadeMag(m);
        }

        if (maxMagValue!=NULL) {
          std::string maxMag;
          Mag         m;

          maxMag=(const char*)maxMagValue;

          if (!GetMag(maxMag,m)) {
            std::cerr << "Unknown maximum magnification '" << maxMag << "' for style type 'label'" << std::endl;
            return;
          }

          label.SetMaxMag(m);
        }

        if (sizeValue!=NULL) {
          double size=1;

          if (!StringToDouble((const char*)sizeValue,size)) {
            std::cerr << "Cannot parse size '" << (const char*)sizeValue << "' for style type 'label'" << std::endl;
            return;
          }

          label.SetSize(size);
        }

        if (textColorValue!=NULL) {
          std::string color=(const char*)textColorValue;
          double      r,g,b,a;

          if (!GetColor(color,r,g,b,a)) {
            std::cerr << "Color value '" << textColorValue << "' for style value 'textColor' cannot be parsed" << std::endl;
            return;
          }

          label.SetTextColor(r,g,b,a);
        }

        if (bgColorValue!=NULL) {
          std::string color=(const char*)bgColorValue;
          double      r,g,b,a;

          if (!GetColor(color,r,g,b,a)) {
            std::cerr << "Color value '" << bgColorValue << "' for style value 'bgColor' cannot be parsed" << std::endl;
            return;
          }

          label.SetBgColor(r,g,b,a);
        }

        if (borderColorValue!=NULL) {
          std::string color=(const char*)borderColorValue;
          double      r,g,b,a;

          if (!GetColor(color,r,g,b,a)) {
            std::cerr << "Color value '" << borderColorValue << "' for style value 'borderColor' cannot be parsed" << std::endl;
            return;
          }

          label.SetBorderColor(r,g,b,a);
        }

        if (context==contextNodeRefLabel) {
          styleConfig.SetNodeRefLabelStyle(type,label);
        }
        else if (context==contextNodeLabel) {
          styleConfig.SetNodeLabelStyle(type,label);
        }
        else if (context==contextWayRefLabel) {
          styleConfig.SetWayRefLabelStyle(type,label);
        }
        else if (context==contextWayLabel) {
          styleConfig.SetWayNameLabelStyle(type,label);
        }
        if (context==contextAreaLabel) {
          styleConfig.SetAreaLabelStyle(type,label);
        }
      }
      else if (context==contextNodeSymbol || context==contextAreaSymbol) {
        const xmlChar *styleValue=NULL;
        const xmlChar *minMagValue=NULL;
        const xmlChar *sizeValue=NULL;
        const xmlChar *fillColorValue=NULL;
        std::string   style="none";

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"style")==0) {
              styleValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"minMag")==0) {
              minMagValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"size")==0) {
              sizeValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"fillColor")==0) {
              fillColorValue=atts[i+1];
            }
          }
        }

        if (styleValue!=NULL) {
          style=(const char*)styleValue;
        }

        if (style=="none") {
          std::cerr << "You cannot force style 'none'" << std::endl;
          return;
        }

        SymbolStyle        symbol;
        SymbolStyle::Style s;

        if (!GetSymbolStyle(style,s)) {
          std::cerr << "Unknown style '" << style << "' for style type 'symbol'" << std::endl;
          return;
        }

        symbol.SetStyle(s);

        if (minMagValue!=NULL) {
          std::string minMag;
          Mag         m;

          minMag=(const char*)minMagValue;

          if (!GetMag(minMag,m)) {
            std::cerr << "Unknown minimum magnification '" << minMag << "' for style type 'symbol'" << std::endl;
            return;
          }

          symbol.SetMinMag(m);
        }

        if (sizeValue!=NULL) {
          double size=1;

          if (!StringToDouble((const char*)sizeValue,size)) {
            std::cerr << "Cannot parse size '" << (const char*)sizeValue << "' for style type 'symbol'" << std::endl;
            return;
          }

          symbol.SetSize(size);
        }

        if (fillColorValue!=NULL) {
          std::string color=(const char*)fillColorValue;
          double      r,g,b,a;

          if (!GetColor(color,r,g,b,a)) {
            std::cerr << "Color value '" << fillColorValue << "' for style value 'fillColor' cannot be parsed" << std::endl;
            return;
          }

          symbol.SetFillColor(r,g,b,a);
        }

        if (context==contextNodeSymbol) {
          styleConfig.SetNodeSymbolStyle(type,symbol);
        }
        else if (context==contextAreaSymbol) {
          styleConfig.SetAreaSymbolStyle(type,symbol);
        }
      }
      else if (context==contextNodeIcon || context==contextAreaIcon) {
        const xmlChar   *iconNameValue=NULL;
        const xmlChar   *minMagValue=NULL;
        std::string     iconName;

        if (atts!=NULL) {
          for (size_t i=0; atts[i]!=NULL && atts[i+1]!=NULL; i+=2) {
            if (strcmp((const char*)atts[i],"name")==0) {
              iconNameValue=atts[i+1];
            }
            else if (strcmp((const char*)atts[i],"minMag")==0) {
              minMagValue=atts[i+1];
            }
          }
        }

        if (iconNameValue==NULL) {
          std::cerr << "Not all required attributes found" << std::endl;
          return;
        }

        iconName=(const char*)iconNameValue;

        IconStyle  iconStyle;

        iconStyle.SetIconName(iconName);

        if (minMagValue!=NULL) {
          std::string minMag;
          Mag         m;

          minMag=(const char*)minMagValue;

          if (!GetMag(minMag,m)) {
            std::cerr << "Unknown minimum magnification '" << minMag << "' for style type 'icon'" << std::endl;
            return;
          }

          iconStyle.SetMinMag(m);
        }

        if (context==contextNodeIcon) {
          styleConfig.SetNodeIconStyle(type,iconStyle);
        }
        else if (context==contextAreaIcon) {
          styleConfig.SetAreaIconStyle(type,iconStyle);
        }
      }
    }

    void EndElement(const xmlChar *name)
    {
      if (context==contextTop) {
        if (strcmp((const char*)name,"TravelJinni-Type")==0) {
          context=contextUnknown;
        }
      }
      else if (context==contextNode) {
        if (strcmp((const char*)name,"node")==0) {
          context=contextTop;
        }
      }
      else if (context==contextWay) {
        if (strcmp((const char*)name,"way")==0) {
          context=contextTop;
        }
      }
      else if (context==contextArea) {
        if (strcmp((const char*)name,"area")==0) {
          context=contextTop;
        }
      }
      else if (context==contextNodeSymbol) {
        if (strcmp((const char*)name,"symbol")==0) {
          context=contextNode;
        }
      }
      else if (context==contextNodeRefLabel) {
        if (strcmp((const char*)name,"ref")==0) {
          context=contextNode;
        }
      }
      else if (context==contextNodeLabel) {
        if (strcmp((const char*)name,"label")==0) {
          context=contextNode;
        }
      }
      else if (context==contextNodeIcon) {
        if (strcmp((const char*)name,"icon")==0) {
          context=contextNode;
        }
      }
      else if (context==contextWayLine) {
        if (strcmp((const char*)name,"line")==0) {
          context=contextWay;
        }
      }
      else if (context==contextWayRefLabel) {
        if (strcmp((const char*)name,"ref")==0) {
          context=contextWay;
        }
      }
      else if (context==contextWayLabel) {
        if (strcmp((const char*)name,"label")==0) {
          context=contextWay;
        }
      }
      else if (context==contextAreaFill) {
        if (strcmp((const char*)name,"fill")==0) {
          context=contextArea;
        }
      }
      else if (context==contextAreaSymbol) {
        if (strcmp((const char*)name,"symbol")==0) {
          context=contextArea;
        }
      }
      else if (context==contextAreaLabel) {
        if (strcmp((const char*)name,"label")==0) {
          context=contextArea;
        }
      }
      else if (context==contextAreaBorder) {
        if (strcmp((const char*)name,"border")==0) {
          context=contextArea;
        }
      }
      else if (context==contextAreaIcon) {
        if (strcmp((const char*)name,"icon")==0) {
          context=contextArea;
        }
      }
      else {
        std::cerr << "Unknown tag '" << (const char*)name << "'" << std::endl;
      }
    }
  };

  static xmlEntityPtr GetEntity(void* /*data*/, const xmlChar *name)
  {
    return xmlGetPredefinedEntity(name);
  }

  static void StartElement(void *data, const xmlChar *name, const xmlChar **atts)
  {
    StyleConfigParser* parser=static_cast<StyleConfigParser*>(data);

    parser->StartElement(name,atts);
  }

  static void EndElement(void *data, const xmlChar *name)
  {
    StyleConfigParser* parser=static_cast<StyleConfigParser*>(data);

    parser->EndElement(name);
  }

  static void StructuredErrorHandler(void *data, xmlErrorPtr error)
  {
    std::cerr << "XML error, line " << error->line << ": " << error->message << std::endl;
  }

  bool LoadStyleConfig(const char* styleFile,
                       StyleConfig& styleConfig)
  {
    StyleConfigParser parser(styleConfig);
    xmlSAXHandler     saxParser;

    memset(&saxParser,0,sizeof(xmlSAXHandler));
    saxParser.initialized=XML_SAX2_MAGIC;
    saxParser.getEntity=GetEntity;
    saxParser.startElement=StartElement;
    saxParser.endElement=EndElement;
    saxParser.serror=StructuredErrorHandler;

    xmlSAXUserParseFile(&saxParser,&parser,styleFile);

    styleConfig.Postprocess();

    return true;
  }
}

