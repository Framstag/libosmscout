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

#include <osmscout/feature/LanesFeature.h>

#include <osmscout/util/String.h>

namespace osmscout {

  void LanesFeatureValue::Read(FileScanner& scanner)
  {
    lanes=scanner.ReadUInt8();

    auto StrToTurns = [](const std::string &str) {
      std::vector<LaneTurn> res;
      for (char ch: str) {
        if (ch > char(LaneTurn::Unknown)) {
          res.push_back(LaneTurn::Unknown);
        } else {
          res.push_back(LaneTurn(ch));
        }
      }
      return res;
    };

    if ((lanes & 0x01u)!=0) {
      turnForward=StrToTurns(scanner.ReadString());
      turnBackward=StrToTurns(scanner.ReadString());
      destinationForward=scanner.ReadString();
      destinationBackward=scanner.ReadString();
    }
  }

  void LanesFeatureValue::Write(FileWriter& writer)
  {
    if (turnForward.empty() &&
        turnBackward.empty() &&
        destinationForward.empty() &&
        destinationBackward.empty()) {
      lanes=lanes & uint8_t(~0x01u);
    }
    else {
      lanes=lanes | 0x01u;
    }

    writer.Write(lanes);

    auto TurnsToStr = [](const std::vector<LaneTurn> &turns) {
      std::string res;
      res.reserve(turns.size());
      for (LaneTurn t: turns) {
        res.push_back(char(t));
      }
      return res;
    };

    if ((lanes & 0x01u)!=0) {
      writer.Write(TurnsToStr(turnForward));
      writer.Write(TurnsToStr(turnBackward));
      writer.Write(destinationForward);
      writer.Write(destinationBackward);
    }
  }

  LanesFeatureValue& LanesFeatureValue::operator=(const FeatureValue& other)
  {
    if (this!=&other) {
      const auto& otherValue=static_cast<const LanesFeatureValue&>(other);

      lanes=otherValue.lanes;
      turnForward=otherValue.turnForward;
      turnBackward=otherValue.turnBackward;
      destinationForward=otherValue.destinationForward;
      destinationBackward=otherValue.destinationBackward;
    }

    return *this;
  }

  bool LanesFeatureValue::operator==(const FeatureValue& other) const
  {
    const auto& otherValue=static_cast<const LanesFeatureValue&>(other);

    return lanes==otherValue.lanes &&
           turnForward==otherValue.turnForward &&
           turnBackward==otherValue.turnBackward &&
           destinationForward==otherValue.destinationForward &&
           destinationBackward==otherValue.destinationBackward;
  }

  uint8_t LanesFeatureValue::GetLanes() const
  {
    return GetForwardLanes() + GetBackwardLanes();
  }

  const char* const LanesFeature::NAME             = "Lanes";
  const char* const LanesFeature::NAME_LABEL       = "label";
  const size_t      LanesFeature::NAME_LABEL_INDEX = 0;

  LanesFeature::LanesFeature()
  {
    RegisterLabel(NAME_LABEL_INDEX,
                  NAME_LABEL);
  }

  void LanesFeature::Initialize(TagRegistry& tagRegistry)
  {
    tagOneway=tagRegistry.RegisterTag("oneway");
    tagLanes=tagRegistry.RegisterTag("lanes");
    tagLanesForward=tagRegistry.RegisterTag("lanes:forward");
    tagLanesBackward=tagRegistry.RegisterTag("lanes:backward");
    tagTurnLanes=tagRegistry.RegisterTag("turn:lanes");
    tagTurnLanesForward=tagRegistry.RegisterTag("turn:lanes:forward");
    tagTurnLanesBackward=tagRegistry.RegisterTag("turn:lanes:backward");
    tagDestinationLanes=tagRegistry.RegisterTag("destination:lanes");
    tagDestinationLanesForward=tagRegistry.RegisterTag("destination:lanes:forward");
    tagDestinationLanesBackward=tagRegistry.RegisterTag("destination:lanes:backward");
  }

  std::string LanesFeature::GetName() const
  {
    return NAME;
  }

  size_t LanesFeature::GetValueAlignment() const
  {
    return alignof(LanesFeatureValue);
  }

  size_t LanesFeature::GetValueSize() const
  {
    return sizeof(LanesFeatureValue);
  }

  FeatureValue* LanesFeature::AllocateValue(void* buffer)
  {
    return new (buffer) LanesFeatureValue();
  }

  void LanesFeature::Parse(TagErrorReporter& errorReporter,
                           const TagRegistry& /*tagRegistry*/,
                           const FeatureInstance& feature,
                           const ObjectOSMRef& object,
                           const TagMap& tags,
                           FeatureValueBuffer& buffer) const
  {
    bool        oneway=false;
    bool        additionalInfos=false;
    uint8_t     lanes=0;
    uint8_t     lanesForward=0;
    uint8_t     lanesBackward=0;
    auto        onewayTag=tags.find(tagOneway);
    auto        lanesTag=tags.find(tagLanes);
    auto        lanesForwardTag=tags.find(tagLanesForward);
    auto        lanesBackwardTag=tags.find(tagLanesBackward);
    auto        turnLanesTag=tags.find(tagTurnLanes);
    auto        turnLanesForwardTag=tags.find(tagTurnLanesForward);
    auto        turnLanesBackwardTag=tags.find(tagTurnLanesBackward);
    auto        destinationLanesTag=tags.find(tagDestinationLanes);
    auto        destinationLanesForwardTag=tags.find(tagDestinationLanesForward);
    auto        destinationLanesBackwardTag=tags.find(tagDestinationLanesBackward);
    std::string turnForward;
    std::string turnBackward;
    std::string destinationForward;
    std::string destinationBackward;

    if (onewayTag!=tags.end()) {
      // TODO: What happens with -1?
      oneway=onewayTag->second!="no" && onewayTag->second!="false" && onewayTag->second!="0";
    }

    if (lanesTag!=tags.end() &&
      !StringToNumber(lanesTag->second,lanes)) {
      errorReporter.ReportTag(object,tags,std::string("lanes tag value '")+lanesTag->second+"' is not numeric!");

      return;
    }

    if (lanesForwardTag!=tags.end() &&
        !StringToNumber(lanesForwardTag->second,lanesForward)) {
      errorReporter.ReportTag(object,tags,std::string("lanes:forward tag value '")+lanesForwardTag->second+"' is not numeric!");

      return;
    }

    if (lanesBackwardTag!=tags.end() &&
        !StringToNumber(lanesBackwardTag->second,lanesBackward)) {
      errorReporter.ReportTag(object,tags,std::string("lanes:backward tag value '")+lanesBackwardTag->second+"' is not numeric!");

      return;
    }

    /* Too many warnings :-/
    if (!oneway && lanesTag!=tags.end() && lanes%2 != 0) {
      errorReporter.ReportTag(object,tags,std::string("No oneway, but lanes tag is set with uneven value"));
    }*/

    if (!oneway &&
        turnLanesTag!=tags.end()) {
      errorReporter.ReportTag(object,tags,std::string("No oneway, but turn:lanes tag is set"));
    }

    if (!oneway &&
        destinationLanesTag!=tags.end()) {
      errorReporter.ReportTag(object,tags,std::string("No oneway, but destination:lanes tag is set"));
    }

    if (turnLanesTag!=tags.end()) {
      turnForward=turnLanesTag->second;
      additionalInfos=true;
    }

    if (turnLanesForwardTag!=tags.end()) {
      turnForward=turnLanesForwardTag->second;
      additionalInfos=true;
    }

    if (turnLanesBackwardTag!=tags.end()) {
      turnBackward=turnLanesBackwardTag->second;
      additionalInfos=true;
    }

    if (destinationLanesTag!=tags.end()) {
      destinationForward=destinationLanesTag->second;
      additionalInfos=true;
    }

    if (destinationLanesForwardTag!=tags.end()) {
      destinationForward=destinationLanesForwardTag->second;
      additionalInfos=true;
    }

    if (destinationLanesBackwardTag!=tags.end()) {
      destinationBackward=destinationLanesBackwardTag->second;
      additionalInfos=true;
    }

    if (lanesForwardTag!=tags.end() &&
        lanesBackwardTag!=tags.end() &&
        lanesTag!=tags.end()) {
      if (lanesForward+lanesBackward!=lanes) {
        errorReporter.ReportTag(object,tags,std::string("lanes tag value '")+lanesTag->second+"' is not equal sum of lanes:forward and lanes:backward");
      }
    }
    else if (lanesForwardTag!=tags.end() &&
             lanesTag!=tags.end()) {
      lanesBackward=lanes-lanesForward;
    }
    else if (lanesBackwardTag!=tags.end() &&
             lanesTag!=tags.end()) {
      lanesForward=lanes-lanesBackward;
    }
    else if (lanesTag!=tags.end()) {
      if (oneway) {
        lanesForward=lanes;
        lanesBackward=0;
      }
      else {
        lanesForward=lanes/(uint8_t)2;
        lanesBackward=lanes/(uint8_t)2;
      }
    }
    else {
      return;
    }

    if (!additionalInfos) {
      if (oneway) {
        if (lanes==feature.GetType()->GetOnewayLanes()) {
          return;
        }
      }
      else {
        if (lanes==feature.GetType()->GetLanes()) {
          return;
        }
      }
    }

    auto* value=static_cast<LanesFeatureValue*>(buffer.AllocateValue(feature.GetIndex()));

    if (lanes==1) {
      // One (most of the time implicit) lane for both directions together
      value->SetLanes(0,0);
    }
    else {
      value->SetLanes(lanesForward,lanesBackward);
    }

    auto ParseLaneTurns = [&](const std::string &turnStr) {
      auto turns=SplitString(turnStr, "|");
      std::vector<LaneTurn> result;
      for (const std::string &turn: turns) {
        if (turn=="none" || turn=="") {
          result.push_back(LaneTurn::None);
        } else if (turn=="left") {
          result.push_back(LaneTurn::Left);
        } else if (turn=="merge_to_left") {
          result.push_back(LaneTurn::MergeToLeft);
        } else if (turn=="slight_left") {
          result.push_back(LaneTurn::SlightLeft);
        } else if (turn=="sharp_left") {
          result.push_back(LaneTurn::SharpLeft);
        } else if (turn=="through;left" || turn=="left;through") {
          result.push_back(LaneTurn::Through_Left);
        } else if (turn=="through;slight_left" || turn=="slight_left;through") {
          result.push_back(LaneTurn::Through_SlightLeft);
        } else if (turn=="through;sharp_left" || turn=="sharp_left;through") {
          result.push_back(LaneTurn::Through_SharpLeft);
        } else if (turn=="through") {
          result.push_back(LaneTurn::Through);
        } else if (turn=="through;right" || turn=="right;through") {
          result.push_back(LaneTurn::Through_Right);
        } else if (turn=="through;slight_right" || turn=="slight_right;through") {
          result.push_back(LaneTurn::Through_SlightRight);
        } else if (turn=="through;sharp_right" || turn=="sharp_right;through") {
          result.push_back(LaneTurn::Through_SharpRight);
        } else if (turn=="right") {
          result.push_back(LaneTurn::Right);
        } else if (turn=="merge_to_right") {
          result.push_back(LaneTurn::MergeToRight);
        } else if (turn=="slight_right") {
          result.push_back(LaneTurn::SlightRight);
        } else if (turn=="sharp_right") {
          result.push_back(LaneTurn::SharpRight);
        } else {
          errorReporter.ReportTag(object,tags,std::string("Lane turn '")+turn+"' is unknown!");
          result.push_back(LaneTurn::Unknown);
        }
      }
      return result;
    };

    if (additionalInfos) {
      value->SetTurnLanes(ParseLaneTurns(turnForward),ParseLaneTurns(turnBackward));
      value->SetDestinationLanes(destinationForward,destinationBackward);
    }
  }
}
