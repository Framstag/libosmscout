/*
This source is part of the libosmscout library
Copyright (C) 2014  Tim Teulings

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

#include <osmscout/feature/ColorFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void ColorFeatureValue::Read(FileScanner& scanner)
  {
    color=scanner.ReadColor();
  }

  void ColorFeatureValue::Write(FileWriter& writer)
  {
    writer.Write(color);
  }

  ColorFeatureValue& ColorFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const ColorFeatureValue&>(other);

      color=otherValue.color;
    }

    return *this;
  }

  bool ColorFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const ColorFeatureValue&>(other);

    return color==otherValue.color;
  }

  const char* const ColorFeature::NAME = "Color";
  const char* const ColorFeature::NUMBER_LABEL = "color";
  const size_t      ColorFeature::NUMBER_LABEL_INDEX = 0;

  ColorFeature::ColorFeature()
  {
    RegisterLabel(NUMBER_LABEL_INDEX,NUMBER_LABEL);
  }

  void ColorFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagColor=tagRegistry.RegisterTag("colour");
    tagSymbol=tagRegistry.RegisterTag("osmc:symbol");
  }

  std::string ColorFeature::GetName() const
  {
    return NAME;
  }

  size_t ColorFeature::GetValueAlignment() const
  {
    return alignof(ColorFeatureValue);
  }

  size_t ColorFeature::GetValueSize() const
  {
    return sizeof(ColorFeatureValue);
  }

  FeatureValue* ColorFeature::AllocateValue(void* buffer)
  {
    return new (buffer) ColorFeatureValue();
  }

  void ColorFeature::Parse(TagErrorReporter& errorReporter,
                            const TagRegistry& /*tagRegistry*/,
                            const FeatureInstance& feature,
                            const ObjectOSMRef& object,
                            const TagMap& tags,
                            FeatureValueBuffer& buffer) const
  {
    using namespace std::string_literals;

    std::string colorString;
    if (auto color=tags.find(tagColor);
        color!=tags.end() && !color->second.empty()) {

      colorString=color->second;
    } else if (auto symbol=tags.find(tagSymbol);
               symbol!=tags.end() && !symbol->second.empty()) {

      colorString=GetFirstInStringList(symbol->second, ":"s);
    }

    if (!colorString.empty()){
      colorString=UTF8StringToLower(colorString);
      Color color;
      if (!Color::FromHexString(colorString, color) &&
          !Color::FromW3CKeywordString(colorString, color)) {
        errorReporter.ReportTag(object,tags,"Not a valid color value ("s + colorString + ")"s);
        return;
      }

      auto* value = static_cast<ColorFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));
      value->SetColor(color);
    }
  }
}
